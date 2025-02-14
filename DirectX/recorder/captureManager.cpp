// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureManager.h"
#include "iunknownWrapper.h"
#include "wrappersAuto.h"
#include "interceptorCustomizationLayer.h"
#include "encoderLayerAuto.h"
#include "captureCustomizationLayer.h"
#include "kernelWrappers.h"
#include "intelExtensionsWrappers.h"
#include "captureSynchronizationLayer.h"
#include "gpuPatchLayer.h"
#include "portabilityLayer.h"
#include "layers/dstorage_resources/directStorageResourcesLayer.h"

#include "gits.h"
#include "directXApi.h"

#include <detours.h>
#include <setupapi.h>
#include <ntddvdeo.h>

namespace gits {
namespace DirectX {

CaptureManager* CaptureManager::instance_ = nullptr;

thread_local unsigned CaptureManager::localStackDepth_ = 0;

CaptureManager& CaptureManager::get() {
  if (!instance_) {
    instance_ = new CaptureManager();
    instance_->interceptKernelFunctions();
    if (Config::Get().directx.capture.captureDirectStorage) {
      instance_->interceptDirectStorageFunctions();
    }
    if (Config::Get().directx.capture.captureDirectML) {
      instance_->interceptDirectMLFunctions();
    }
  }

  return *instance_;
}

CaptureManager::~CaptureManager() {
  if (kernelDll_) {
    FreeLibrary(kernelDll_);
  }
  if (dmlDll_) {
    FreeLibrary(dmlDll_);
  }
  if (dStorageDll_) {
    FreeLibrary(dStorageDll_);
  }
  if (xessDll_) {
    FreeLibrary(xessDll_);
  }
  if (intelExtensionLoaded_) {
    INTC_UnloadExtensionsLibrary();
  }
}

void CaptureManager::exchangeDXGIDispatchTables(const DXGIDispatchTable& systemTable,
                                                DXGIDispatchTable& wrapperTable) {
  dxgiDispatchTableSystem_ = systemTable;
  wrapperTable = dxgiDispatchTableWrapper_;
}

void CaptureManager::exchangeD3D12DispatchTables(const D3D12DispatchTable& systemTable,
                                                 D3D12DispatchTable& wrapperTable) {
  d3d12DispatchTableSystem_ = systemTable;
  wrapperTable = d3d12DispatchTableWrapper_;
}

CaptureManager::CaptureManager() {

  gits::CGits::Instance().apis.UseApi3dIface(
      std::shared_ptr<gits::ApisIface::Api3d>(new DirectXApi()));

  dxgiDispatchTableWrapper_.CreateDXGIFactory = CreateDXGIFactoryWrapper;
  dxgiDispatchTableWrapper_.CreateDXGIFactory1 = CreateDXGIFactory1Wrapper;
  dxgiDispatchTableWrapper_.CreateDXGIFactory2 = CreateDXGIFactory2Wrapper;
  dxgiDispatchTableWrapper_.DXGIDeclareAdapterRemovalSupport =
      DXGIDeclareAdapterRemovalSupportWrapper;
  dxgiDispatchTableWrapper_.DXGIGetDebugInterface1 = DXGIGetDebugInterface1Wrapper;

  d3d12DispatchTableWrapper_.D3D12CreateDevice = D3D12CreateDeviceWrapper;
  d3d12DispatchTableWrapper_.D3D12GetDebugInterface = D3D12GetDebugInterfaceWrapper;
  d3d12DispatchTableWrapper_.D3D12CreateRootSignatureDeserializer =
      D3D12CreateRootSignatureDeserializerWrapper;
  d3d12DispatchTableWrapper_.D3D12CreateVersionedRootSignatureDeserializer =
      D3D12CreateVersionedRootSignatureDeserializerWrapper;
  d3d12DispatchTableWrapper_.D3D12EnableExperimentalFeatures =
      D3D12EnableExperimentalFeaturesWrapper;
  d3d12DispatchTableWrapper_.D3D12GetInterface = D3D12GetInterfaceWrapper;
  d3d12DispatchTableWrapper_.D3D12SerializeRootSignature = D3D12SerializeRootSignatureWrapper;
  d3d12DispatchTableWrapper_.D3D12SerializeVersionedRootSignature =
      D3D12SerializeVersionedRootSignatureWrapper;

  recorder_.reset(new GitsRecorder());

  mapTrackingService_.reset(new MapTrackingService(*recorder_));
  fenceService_.reset(new FenceService(*recorder_));

  createLayers();
}

void CaptureManager::createLayers() {
  auto& cfg = Config::Get();

  // Create layers used by Recorder
  std::unique_ptr<Layer> interceptorCustomizationLayer =
      std::make_unique<InterceptorCustomizationLayer>();
  std::unique_ptr<Layer> traceLayer = traceFactory_.getTraceLayer();
  std::unique_ptr<Layer> debugInfoLayer = traceFactory_.getDebugInfoLayer();
  std::unique_ptr<Layer> debugHelperLayer = traceFactory_.getDebugHelperLayer();
  std::unique_ptr<Layer> logDxErrorLayer = traceFactory_.getLogDxErrorLayer();
  std::unique_ptr<Layer> screenshotsLayer = resourceDumpingFactory_.getScreenshotsLayer();
  std::unique_ptr<Layer> directStorageResourcesLayer;
  std::unique_ptr<Layer> captureCustomizationLayer;
  std::unique_ptr<Layer> captureSynchronizationLayer;
  std::unique_ptr<Layer> encoderLayer;
  std::unique_ptr<Layer> gpuPatchLayer;
  std::unique_ptr<Layer> portabilityLayer;

  if (cfg.directx.capture.record) {
    captureCustomizationLayer = std::make_unique<CaptureCustomizationLayer>(*this, *recorder_);
    captureSynchronizationLayer = std::make_unique<CaptureSynchronizationLayer>(*this);
    encoderLayer = std::make_unique<EncoderLayer>(*recorder_);
    gpuPatchLayer = std::make_unique<GpuPatchLayer>(gpuAddressService_);
    if (cfg.directx.capture.captureDirectStorage) {
      directStorageResourcesLayer = std::make_unique<DirectStorageResourcesLayer>();
    }
    if (cfg.directx.features.portability.enabled) {
      portabilityLayer = std::make_unique<PortabilityLayer>();
    }
  }

  // Enable Pre layers
  // Insertion order determines execution order
  auto enablePreLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      preLayers_.push_back(layer.get());
    }
  };
  enablePreLayer(traceLayer);
  enablePreLayer(interceptorCustomizationLayer);
  enablePreLayer(captureCustomizationLayer);
  enablePreLayer(debugInfoLayer);
  enablePreLayer(captureSynchronizationLayer);
  enablePreLayer(screenshotsLayer);
  enablePreLayer(portabilityLayer);
  enablePreLayer(directStorageResourcesLayer);

  // Enable Post layers
  // Insertion order determines execution order
  auto enablePostLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      postLayers_.push_back(layer.get());
    }
  };
  enablePostLayer(logDxErrorLayer);
  enablePostLayer(interceptorCustomizationLayer);
  enablePostLayer(captureCustomizationLayer);
  enablePostLayer(portabilityLayer);
  enablePostLayer(debugInfoLayer);
  enablePostLayer(encoderLayer);
  enablePostLayer(gpuPatchLayer);
  enablePostLayer(captureSynchronizationLayer);
  enablePostLayer(traceLayer);
  enablePostLayer(debugHelperLayer);
  enablePostLayer(screenshotsLayer);
  enablePostLayer(directStorageResourcesLayer);

  // Retain ownership of layers
  auto retainLayer = [this](std::unique_ptr<Layer>&& layer) {
    if (layer) {
      layersOwner_.push_back(std::move(layer));
    }
  };
  retainLayer(std::move(interceptorCustomizationLayer));
  retainLayer(std::move(captureCustomizationLayer));
  retainLayer(std::move(captureSynchronizationLayer));
  retainLayer(std::move(encoderLayer));
  retainLayer(std::move(gpuPatchLayer));
  retainLayer(std::move(traceLayer));
  retainLayer(std::move(debugInfoLayer));
  retainLayer(std::move(debugHelperLayer));
  retainLayer(std::move(logDxErrorLayer));
  retainLayer(std::move(screenshotsLayer));
  retainLayer(std::move(directStorageResourcesLayer));
  retainLayer(std::move(portabilityLayer));

  pluginService_.loadPlugins();
  for (const auto& plugin : pluginService_.getPlugins()) {
    auto* layer = static_cast<Layer*>(plugin.impl->getImpl());
    // The enable[Pre|Post]Layer lambdas take unique_ptr<Layer>& instead of
    // Layer* to avoid littering their each use with a .get() call. This means
    // we can't use them here, so let's add those layers manually.
    if (layer) {
      preLayers_.push_back(layer);
      postLayers_.push_back(layer);
    }
  }
}

void CaptureManager::interceptDirectMLFunctions() {
  // Load DirectML.dll from the current directory or System32
  // Most DirectML applications include the DirectML runtime
  dmlDll_ = LoadLibrary("DirectML.dll");
  GITS_ASSERT(dmlDll_);

  dmlDispatchTable_.DMLCreateDevice =
      reinterpret_cast<decltype(DMLCreateDevice)*>(GetProcAddress(dmlDll_, "DMLCreateDevice"));
  dmlDispatchTable_.DMLCreateDevice1 =
      reinterpret_cast<decltype(DMLCreateDevice1)*>(GetProcAddress(dmlDll_, "DMLCreateDevice1"));

  LONG ret = DetourTransactionBegin();
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourUpdateThread(GetCurrentThread());
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&dmlDispatchTable_.DMLCreateDevice, DMLCreateDeviceWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&dmlDispatchTable_.DMLCreateDevice1, DMLCreateDevice1Wrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourTransactionCommit();
  GITS_ASSERT(ret == NO_ERROR);
}

void CaptureManager::interceptDirectStorageFunctions() {
  // Load directstorage.dll from the current directory
  // If not available the game does not use directstorage.dll so no need to do anything
  dStorageDll_ = LoadLibrary("dstorage.dll");
  GITS_ASSERT(dStorageDll_);

  dstorageDispatchTable_.DStorageSetConfiguration =
      reinterpret_cast<decltype(DStorageSetConfiguration)*>(
          GetProcAddress(dStorageDll_, "DStorageSetConfiguration"));
  dstorageDispatchTable_.DStorageSetConfiguration1 =
      reinterpret_cast<decltype(DStorageSetConfiguration1)*>(
          GetProcAddress(dStorageDll_, "DStorageSetConfiguration1"));
  dstorageDispatchTable_.DStorageGetFactory = reinterpret_cast<decltype(DStorageGetFactory)*>(
      GetProcAddress(dStorageDll_, "DStorageGetFactory"));
  dstorageDispatchTable_.DStorageCreateCompressionCodec =
      reinterpret_cast<decltype(DStorageCreateCompressionCodec)*>(
          GetProcAddress(dStorageDll_, "DStorageCreateCompressionCodec"));

  LONG ret = DetourTransactionBegin();
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourUpdateThread(GetCurrentThread());
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&dstorageDispatchTable_.DStorageSetConfiguration,
                     DStorageSetConfigurationWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&dstorageDispatchTable_.DStorageSetConfiguration1,
                     DStorageSetConfiguration1Wrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&dstorageDispatchTable_.DStorageGetFactory, DStorageGetFactoryWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&dstorageDispatchTable_.DStorageCreateCompressionCodec,
                     DStorageCreateCompressionCodecWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourTransactionCommit();
  GITS_ASSERT(ret == NO_ERROR);
}

void CaptureManager::addWrapper(IUnknownWrapper* wrapper) {
  std::lock_guard<std::mutex> lock(wrappersMutex_);
  wrappers_[wrapper->getRootIUnknown()] = wrapper;
}
void CaptureManager::removeWrapper(IUnknownWrapper* wrapper) {
  std::lock_guard<std::mutex> lock(wrappersMutex_);
  wrappers_.erase(wrapper->getRootIUnknown());
}
IUnknownWrapper* CaptureManager::findWrapper(IUnknown* object) {
  std::lock_guard<std::mutex> lock(wrappersMutex_);
  auto it = wrappers_.find(IUnknownWrapper::getRootIUnknown(object));
  return it != wrappers_.end() ? it->second : nullptr;
}

void CaptureManager::interceptKernelFunctions() {

  if (kernelDll_) {
    return;
  }

  kernelDll_ = LoadLibrary("C:\\Windows\\System32\\kernel32.dll");
  GITS_ASSERT(kernelDll_);

  kernel32DispatchTableSystem_.WaitForSingleObject =
      reinterpret_cast<decltype(WaitForSingleObject)*>(
          GetProcAddress(kernelDll_, "WaitForSingleObject"));

  kernel32DispatchTableSystem_.WaitForSingleObjectEx =
      reinterpret_cast<decltype(WaitForSingleObjectEx)*>(
          GetProcAddress(kernelDll_, "WaitForSingleObjectEx"));

  kernel32DispatchTableSystem_.WaitForMultipleObjects =
      reinterpret_cast<decltype(WaitForMultipleObjects)*>(
          GetProcAddress(kernelDll_, "WaitForMultipleObjects"));

  kernel32DispatchTableSystem_.WaitForMultipleObjectsEx =
      reinterpret_cast<decltype(WaitForMultipleObjectsEx)*>(
          GetProcAddress(kernelDll_, "WaitForMultipleObjectsEx"));

  kernel32DispatchTableSystem_.SignalObjectAndWait =
      reinterpret_cast<decltype(SignalObjectAndWait)*>(
          GetProcAddress(kernelDll_, "SignalObjectAndWait"));

  kernel32DispatchTableSystem_.LoadLibraryA =
      reinterpret_cast<decltype(LoadLibraryA)*>(GetProcAddress(kernelDll_, "LoadLibraryA"));

  kernel32DispatchTableSystem_.LoadLibraryW =
      reinterpret_cast<decltype(LoadLibraryW)*>(GetProcAddress(kernelDll_, "LoadLibraryW"));

  kernel32DispatchTableSystem_.LoadLibraryExA =
      reinterpret_cast<decltype(LoadLibraryExA)*>(GetProcAddress(kernelDll_, "LoadLibraryExA"));

  kernel32DispatchTableSystem_.LoadLibraryExW =
      reinterpret_cast<decltype(LoadLibraryExW)*>(GetProcAddress(kernelDll_, "LoadLibraryExW"));

  LONG ret = DetourTransactionBegin();
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourUpdateThread(GetCurrentThread());
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&kernel32DispatchTableSystem_.WaitForSingleObject, WaitForSingleObject);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&kernel32DispatchTableSystem_.WaitForSingleObjectEx, WaitForSingleObjectEx);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&kernel32DispatchTableSystem_.WaitForMultipleObjects, WaitForMultipleObjects);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&kernel32DispatchTableSystem_.WaitForMultipleObjectsEx,
                     WaitForMultipleObjectsEx);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&kernel32DispatchTableSystem_.SignalObjectAndWait, SignalObjectAndWait);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&kernel32DispatchTableSystem_.LoadLibraryA, MyLoadLibraryA);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&kernel32DispatchTableSystem_.LoadLibraryW, MyLoadLibraryW);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&kernel32DispatchTableSystem_.LoadLibraryExA, MyLoadLibraryExA);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&kernel32DispatchTableSystem_.LoadLibraryExW, MyLoadLibraryExW);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourTransactionCommit();
  GITS_ASSERT(ret == NO_ERROR);
}

void CaptureManager::interceptXessFunctions() {

  if (xessDll_ || loadingXessDll_ || !Config::Get().directx.capture.captureXess) {
    return;
  }

  loadingXessDll_ = true;
  xessDll_ = LoadLibrary("libxess.dll");
  loadingXessDll_ = false;
  if (!xessDll_) {
    return;
  }

  {
    xessDispatchTable_.xessGetVersion =
        reinterpret_cast<decltype(xessGetVersion)*>(GetProcAddress(xessDll_, "xessGetVersion"));

    xessDispatchTable_.xessGetIntelXeFXVersion =
        reinterpret_cast<decltype(xessGetIntelXeFXVersion)*>(
            GetProcAddress(xessDll_, "xessGetIntelXeFXVersion"));

    xessDispatchTable_.xessGetProperties = reinterpret_cast<decltype(xessGetProperties)*>(
        GetProcAddress(xessDll_, "xessGetProperties"));

    xessDispatchTable_.xessGetInputResolution = reinterpret_cast<decltype(xessGetInputResolution)*>(
        GetProcAddress(xessDll_, "xessGetInputResolution"));

    xessDispatchTable_.xessGetOptimalInputResolution =
        reinterpret_cast<decltype(xessGetOptimalInputResolution)*>(
            GetProcAddress(xessDll_, "xessGetOptimalInputResolution"));

    xessDispatchTable_.xessGetJitterScale = reinterpret_cast<decltype(xessGetJitterScale)*>(
        GetProcAddress(xessDll_, "xessGetJitterScale"));

    xessDispatchTable_.xessGetVelocityScale = reinterpret_cast<decltype(xessGetVelocityScale)*>(
        GetProcAddress(xessDll_, "xessGetVelocityScale"));

    xessDispatchTable_.xessDestroyContext = reinterpret_cast<decltype(xessDestroyContext)*>(
        GetProcAddress(xessDll_, "xessDestroyContext"));

    xessDispatchTable_.xessSetJitterScale = reinterpret_cast<decltype(xessSetJitterScale)*>(
        GetProcAddress(xessDll_, "xessSetJitterScale"));

    xessDispatchTable_.xessSetVelocityScale = reinterpret_cast<decltype(xessSetVelocityScale)*>(
        GetProcAddress(xessDll_, "xessSetVelocityScale"));

    xessDispatchTable_.xessSetExposureMultiplier =
        reinterpret_cast<decltype(xessSetExposureMultiplier)*>(
            GetProcAddress(xessDll_, "xessSetExposureMultiplier"));

    xessDispatchTable_.xessGetExposureMultiplier =
        reinterpret_cast<decltype(xessGetExposureMultiplier)*>(
            GetProcAddress(xessDll_, "xessGetExposureMultiplier"));

    xessDispatchTable_.xessIsOptimalDriver = reinterpret_cast<decltype(xessIsOptimalDriver)*>(
        GetProcAddress(xessDll_, "xessIsOptimalDriver"));

    xessDispatchTable_.xessForceLegacyScaleFactors =
        reinterpret_cast<decltype(xessForceLegacyScaleFactors)*>(
            GetProcAddress(xessDll_, "xessForceLegacyScaleFactors"));

    xessDispatchTable_.xessSetLoggingCallback = reinterpret_cast<decltype(xessSetLoggingCallback)*>(
        GetProcAddress(xessDll_, "xessSetLoggingCallback"));

    xessDispatchTable_.xessD3D12CreateContext = reinterpret_cast<decltype(xessD3D12CreateContext)*>(
        GetProcAddress(xessDll_, "xessD3D12CreateContext"));

    xessDispatchTable_.xessD3D12BuildPipelines =
        reinterpret_cast<decltype(xessD3D12BuildPipelines)*>(
            GetProcAddress(xessDll_, "xessD3D12BuildPipelines"));

    xessDispatchTable_.xessD3D12Init =
        reinterpret_cast<decltype(xessD3D12Init)*>(GetProcAddress(xessDll_, "xessD3D12Init"));

    xessDispatchTable_.xessD3D12GetInitParams = reinterpret_cast<decltype(xessD3D12GetInitParams)*>(
        GetProcAddress(xessDll_, "xessD3D12GetInitParams"));

    xessDispatchTable_.xessD3D12Execute =
        reinterpret_cast<decltype(xessD3D12Execute)*>(GetProcAddress(xessDll_, "xessD3D12Execute"));
  }

  xess_version_t xessVersion{};
  xess_result_t res = xessDispatchTable_.xessGetVersion(&xessVersion);
  GITS_ASSERT(res == XESS_RESULT_SUCCESS);
  if (xessVersion.major > 1) {
    Log(WARN)
        << "XeSS 2.0 (or newer) is loaded. GITS will not record XeSS 2.0 (or newer) API calls.";
  }

  Log(INFO) << "Loaded XeSS (libxess.dll) version: " << xessVersion.major << "."
            << xessVersion.minor << "." << xessVersion.patch;

  LONG ret = DetourTransactionBegin();
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourUpdateThread(GetCurrentThread());
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&xessDispatchTable_.xessGetVersion, xessGetVersionWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&xessDispatchTable_.xessGetIntelXeFXVersion, xessGetIntelXeFXVersionWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&xessDispatchTable_.xessGetProperties, xessGetPropertiesWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&xessDispatchTable_.xessGetInputResolution, xessGetInputResolutionWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&xessDispatchTable_.xessDestroyContext, xessDestroyContextWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&xessDispatchTable_.xessSetJitterScale, xessSetJitterScaleWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&xessDispatchTable_.xessSetVelocityScale, xessSetVelocityScaleWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  if (xessVersion.minor >= 1) {
    ret = DetourAttach(&xessDispatchTable_.xessGetJitterScale, xessGetJitterScaleWrapper);
    GITS_ASSERT(ret == NO_ERROR);
    ret = DetourAttach(&xessDispatchTable_.xessGetVelocityScale, xessGetVelocityScaleWrapper);
    GITS_ASSERT(ret == NO_ERROR);
    ret = DetourAttach(&xessDispatchTable_.xessIsOptimalDriver, xessIsOptimalDriverWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (xessVersion.minor >= 2) {
    ret = DetourAttach(&xessDispatchTable_.xessGetOptimalInputResolution,
                       xessGetOptimalInputResolutionWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (xessVersion.minor >= 3) {
    ret = DetourAttach(&xessDispatchTable_.xessSetExposureMultiplier,
                       xessSetExposureMultiplierWrapper);
    GITS_ASSERT(ret == NO_ERROR);
    ret = DetourAttach(&xessDispatchTable_.xessGetExposureMultiplier,
                       xessGetExposureMultiplierWrapper);
    GITS_ASSERT(ret == NO_ERROR);

    ret = DetourAttach(&xessDispatchTable_.xessForceLegacyScaleFactors,
                       xessForceLegacyScaleFactorsWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  ret = DetourAttach(&xessDispatchTable_.xessD3D12CreateContext, xessD3D12CreateContextWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&xessDispatchTable_.xessD3D12Init, xessD3D12InitWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&xessDispatchTable_.xessD3D12BuildPipelines, xessD3D12BuildPipelinesWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&xessDispatchTable_.xessD3D12GetInitParams, xessD3D12GetInitParamsWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&xessDispatchTable_.xessD3D12Execute, xessD3D12ExecuteWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourTransactionCommit();
  GITS_ASSERT(ret == NO_ERROR);
}

void CaptureManager::loadIntelExtension(const uint32_t& vendorID, const uint32_t& deviceID) {
  if (intelExtensionLoaded_ || !Config::Get().directx.capture.captureIntelExtensions) {
    return;
  }

  HRESULT result = INTC_LoadExtensionsLibrary(false, vendorID, deviceID);
  if (FAILED(result)) {
    return;
  }

  INTC_D3D12_API_CALLBACKS callbacks{0};
  callbacks.INTC_D3D12_GetSupportedVersions = INTC_D3D12_GetSupportedVersionsWrapper;
  callbacks.INTC_D3D12_CreateDeviceExtensionContext =
      INTC_D3D12_CreateDeviceExtensionContextWrapper;
  callbacks.INTC_D3D12_CreateDeviceExtensionContext1 =
      INTC_D3D12_CreateDeviceExtensionContext1Wrapper;
  callbacks.INTC_DestroyDeviceExtensionContext = INTC_DestroyDeviceExtensionContextWrapper;
  callbacks.INTC_D3D12_CreateCommandQueue = INTC_D3D12_CreateCommandQueueWrapper;
  callbacks.INTC_D3D12_CreateComputePipelineState = INTC_D3D12_CreateComputePipelineStateWrapper;
  callbacks.INTC_D3D12_CreateReservedResource = INTC_D3D12_CreateReservedResourceWrapper;
  callbacks.INTC_D3D12_CreateCommittedResource = INTC_D3D12_CreateCommittedResourceWrapper;
  callbacks.INTC_D3D12_CreateCommittedResource1 = INTC_D3D12_CreateCommittedResource1Wrapper;
  callbacks.INTC_D3D12_CreateHeap = INTC_D3D12_CreateHeapWrapper;
  callbacks.INTC_D3D12_CreatePlacedResource = INTC_D3D12_CreatePlacedResourceWrapper;
  callbacks.INTC_D3D12_CreateHostRTASResource = INTC_D3D12_CreateHostRTASResourceWrapper;
  callbacks.INTC_D3D12_BuildRaytracingAccelerationStructure_Host =
      INTC_D3D12_BuildRaytracingAccelerationStructure_HostWrapper;
  callbacks.INTC_D3D12_CopyRaytracingAccelerationStructure_Host =
      INTC_D3D12_CopyRaytracingAccelerationStructure_HostWrapper;
  callbacks.INTC_D3D12_EmitRaytracingAccelerationStructurePostbuildInfo_Host =
      INTC_D3D12_EmitRaytracingAccelerationStructurePostbuildInfo_HostWrapper;
  callbacks.INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo_Host =
      INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo_HostWrapper;
  callbacks.INTC_D3D12_TransferHostRTAS = INTC_D3D12_TransferHostRTASWrapper;
  callbacks.INTC_D3D12_SetDriverEventMetadata = INTC_D3D12_SetDriverEventMetadataWrapper;
  callbacks.INTC_D3D12_QueryCpuVisibleVidmem = INTC_D3D12_QueryCpuVisibleVidmemWrapper;
  callbacks.INTC_D3D12_CreateStateObject = INTC_D3D12_CreateStateObjectWrapper;
  callbacks.INTC_D3D12_BuildRaytracingAccelerationStructure =
      INTC_D3D12_BuildRaytracingAccelerationStructureWrapper;
  callbacks.INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo =
      INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfoWrapper;
  callbacks.INTC_D3D12_SetFeatureSupport = INTC_D3D12_SetFeatureSupportWrapper;
  callbacks.INTC_D3D12_GetResourceAllocationInfo = INTC_D3D12_GetResourceAllocationInfoWrapper;
  callbacks.INTC_D3D12_CheckFeatureSupport = INTC_D3D12_CheckFeatureSupportWrapper;
  callbacks.INTC_D3D12_AddShaderBinariesPath = INTC_D3D12_AddShaderBinariesPathWrapper;
  callbacks.INTC_D3D12_RemoveShaderBinariesPath = INTC_D3D12_RemoveShaderBinariesPathWrapper;
  callbacks.INTC_D3D12_SetApplicationInfo = INTC_D3D12_SetApplicationInfoWrapper;

  result = INTC_D3D12_RegisterApplicationCallbacks(&callbacks);
  if (FAILED(result)) {
    Log(ERR) << "IntelExtensions: INTC_D3D12_RegisterApplicationCallbacks failed.";
  }

  intelExtensionLoaded_ = true;
}

} // namespace DirectX
} // namespace gits
