// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureManager.h"
#include "iunknownWrapper.h"
#include "wrappersAuto.h"
#include "kernelWrappers.h"
#include "intelExtensionsWrappers.h"
#include "nvapiWrappers.h"
#include "nvapi_interface.h"
#include "d3d11on12Wrappers.h"
#include "configurator.h"
#include "log.h"

#include <detours.h>
#include <setupapi.h>
#include <ntddvdeo.h>
#include <filesystem>

namespace gits {
namespace DirectX {

CaptureManager* CaptureManager::m_Instance = nullptr;

thread_local unsigned CaptureManager::m_LocalStackDepth = 0;

CaptureManager& CaptureManager::get() {
  if (!m_Instance) {
    m_Instance = new CaptureManager();
    m_Instance->interceptKernelFunctions();
    if (Configurator::Get().directx.recorder.captureDirectStorage) {
      m_Instance->interceptDirectStorageFunctions();
    }
    if (Configurator::Get().directx.recorder.captureDirectML) {
      m_Instance->interceptDirectMLFunctions();
    }
    if (Configurator::Get().directx.recorder.captureNvAPI) {
      m_Instance->interceptNvAPIFunctions();
    }
    m_Instance->interceptD3D11On12Functions();
  }

  return *m_Instance;
}

CaptureManager::~CaptureManager() {
  if (m_KernelDll) {
    FreeLibrary(m_KernelDll);
  }
  if (m_DmlDll) {
    FreeLibrary(m_DmlDll);
  }
  if (m_DstorageDll) {
    FreeLibrary(m_DstorageDll);
  }
  if (m_XessDll) {
    FreeLibrary(m_XessDll);
  }
  if (m_XellDll) {
    FreeLibrary(m_XellDll);
  }
  if (m_XefgDll) {
    FreeLibrary(m_XefgDll);
  }
  if (m_IntelExtensionLoaded) {
    INTC_UnloadExtensionsLibrary();
  }
  if (m_NvapiDll) {
    FreeLibrary(m_NvapiDll);
  }
  if (m_D3D11Dll) {
    FreeLibrary(m_D3D11Dll);
  }
}

void CaptureManager::exchangeDXGIDispatchTables(const DXGIDispatchTable& systemTable,
                                                DXGIDispatchTable& wrapperTable) {
  m_DxgiDispatchTableSystem = systemTable;
  wrapperTable = m_DxgiDispatchTableWrapper;
}

void CaptureManager::exchangeD3D12DispatchTables(const D3D12DispatchTable& systemTable,
                                                 D3D12DispatchTable& wrapperTable) {
  m_D3D12DispatchTableSystem = systemTable;
  wrapperTable = m_D3D12DispatchTableWrapper;
}

CaptureManager::CaptureManager() {
  m_DxgiDispatchTableWrapper.CreateDXGIFactory = CreateDXGIFactoryWrapper;
  m_DxgiDispatchTableWrapper.CreateDXGIFactory1 = CreateDXGIFactory1Wrapper;
  m_DxgiDispatchTableWrapper.CreateDXGIFactory2 = CreateDXGIFactory2Wrapper;
  m_DxgiDispatchTableWrapper.DXGIDeclareAdapterRemovalSupport =
      DXGIDeclareAdapterRemovalSupportWrapper;
  m_DxgiDispatchTableWrapper.DXGIGetDebugInterface1 = DXGIGetDebugInterface1Wrapper;

  m_D3D12DispatchTableWrapper.D3D12CreateDevice = D3D12CreateDeviceWrapper;
  m_D3D12DispatchTableWrapper.D3D12GetDebugInterface = D3D12GetDebugInterfaceWrapper;
  m_D3D12DispatchTableWrapper.D3D12CreateRootSignatureDeserializer =
      D3D12CreateRootSignatureDeserializerWrapper;
  m_D3D12DispatchTableWrapper.D3D12CreateVersionedRootSignatureDeserializer =
      D3D12CreateVersionedRootSignatureDeserializerWrapper;
  m_D3D12DispatchTableWrapper.D3D12EnableExperimentalFeatures =
      D3D12EnableExperimentalFeaturesWrapper;
  m_D3D12DispatchTableWrapper.D3D12GetInterface = D3D12GetInterfaceWrapper;
  m_D3D12DispatchTableWrapper.D3D12SerializeRootSignature = D3D12SerializeRootSignatureWrapper;
  m_D3D12DispatchTableWrapper.D3D12SerializeVersionedRootSignature =
      D3D12SerializeVersionedRootSignatureWrapper;

  m_Recorder.reset(new stream::OrderingRecorder());

  m_MapTrackingService.reset(new MapTrackingService(*m_Recorder));
  m_FenceService.reset(new FenceService(*m_Recorder));

  m_PluginService.loadPlugins();

  m_LayerManager.LoadLayers(*this, *m_Recorder.get(), m_GpuAddressService, m_PluginService);
}

void CaptureManager::interceptDirectMLFunctions() {
  // Load DirectML.dll from the current directory or System32
  // Most DirectML applications include the DirectML runtime
  m_DmlDll = LoadLibrary("DirectML.dll");
  GITS_ASSERT(m_DmlDll);

  m_DmlDispatchTable.DMLCreateDevice =
      reinterpret_cast<decltype(DMLCreateDevice)*>(GetProcAddress(m_DmlDll, "DMLCreateDevice"));
  m_DmlDispatchTable.DMLCreateDevice1 =
      reinterpret_cast<decltype(DMLCreateDevice1)*>(GetProcAddress(m_DmlDll, "DMLCreateDevice1"));

  LONG ret = DetourTransactionBegin();
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourUpdateThread(GetCurrentThread());
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_DmlDispatchTable.DMLCreateDevice, DMLCreateDeviceWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&m_DmlDispatchTable.DMLCreateDevice1, DMLCreateDevice1Wrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourTransactionCommit();
  GITS_ASSERT(ret == NO_ERROR);
}

void CaptureManager::interceptDirectStorageFunctions() {
  // Load directstorage.dll from the current directory
  // If not available the game does not use directstorage.dll so no need to do anything
  m_DstorageDll = LoadLibrary("dstorage.dll");
  GITS_ASSERT(m_DstorageDll);

  m_DstorageDispatchTable.DStorageSetConfiguration =
      reinterpret_cast<decltype(DStorageSetConfiguration)*>(
          GetProcAddress(m_DstorageDll, "DStorageSetConfiguration"));
  m_DstorageDispatchTable.DStorageSetConfiguration1 =
      reinterpret_cast<decltype(DStorageSetConfiguration1)*>(
          GetProcAddress(m_DstorageDll, "DStorageSetConfiguration1"));
  m_DstorageDispatchTable.DStorageGetFactory = reinterpret_cast<decltype(DStorageGetFactory)*>(
      GetProcAddress(m_DstorageDll, "DStorageGetFactory"));
  m_DstorageDispatchTable.DStorageCreateCompressionCodec =
      reinterpret_cast<decltype(DStorageCreateCompressionCodec)*>(
          GetProcAddress(m_DstorageDll, "DStorageCreateCompressionCodec"));

  LONG ret = DetourTransactionBegin();
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourUpdateThread(GetCurrentThread());
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_DstorageDispatchTable.DStorageSetConfiguration,
                     DStorageSetConfigurationWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&m_DstorageDispatchTable.DStorageSetConfiguration1,
                     DStorageSetConfiguration1Wrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&m_DstorageDispatchTable.DStorageGetFactory, DStorageGetFactoryWrapper);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&m_DstorageDispatchTable.DStorageCreateCompressionCodec,
                     DStorageCreateCompressionCodecWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourTransactionCommit();
  GITS_ASSERT(ret == NO_ERROR);
}

std::pair<unsigned, unsigned> CaptureManager::createCommandKeyRange(unsigned rangeSize) {
  std::pair<unsigned, unsigned> range;
  range.first = m_CommandUniqueKey.fetch_add(rangeSize, std::memory_order_relaxed) + 1;
  range.second = range.first + rangeSize - 1;
  return range;
}

void CaptureManager::addWrapper(IUnknownWrapper* wrapper) {
  std::lock_guard<std::mutex> lock(m_WrappersMutex);
  m_Wrappers[wrapper->getRootIUnknown()] = wrapper;
}
void CaptureManager::removeWrapper(IUnknownWrapper* wrapper) {
  std::lock_guard<std::mutex> lock(m_WrappersMutex);
  m_Wrappers.erase(wrapper->getRootIUnknown());
}
IUnknownWrapper* CaptureManager::findWrapper(IUnknown* object) {
  std::lock_guard<std::mutex> lock(m_WrappersMutex);
  auto it = m_Wrappers.find(IUnknownWrapper::getRootIUnknown(object));
  return it != m_Wrappers.end() ? it->second : nullptr;
}

void CaptureManager::interceptKernelFunctions() {

  if (m_KernelDll) {
    return;
  }

  m_KernelDll = LoadLibrary("C:\\Windows\\System32\\kernel32.dll");
  GITS_ASSERT(m_KernelDll);

  m_Kernel32DispatchTableSystem.WaitForSingleObject =
      reinterpret_cast<decltype(WaitForSingleObject)*>(
          GetProcAddress(m_KernelDll, "WaitForSingleObject"));

  m_Kernel32DispatchTableSystem.WaitForSingleObjectEx =
      reinterpret_cast<decltype(WaitForSingleObjectEx)*>(
          GetProcAddress(m_KernelDll, "WaitForSingleObjectEx"));

  m_Kernel32DispatchTableSystem.WaitForMultipleObjects =
      reinterpret_cast<decltype(WaitForMultipleObjects)*>(
          GetProcAddress(m_KernelDll, "WaitForMultipleObjects"));

  m_Kernel32DispatchTableSystem.WaitForMultipleObjectsEx =
      reinterpret_cast<decltype(WaitForMultipleObjectsEx)*>(
          GetProcAddress(m_KernelDll, "WaitForMultipleObjectsEx"));

  m_Kernel32DispatchTableSystem.SignalObjectAndWait =
      reinterpret_cast<decltype(SignalObjectAndWait)*>(
          GetProcAddress(m_KernelDll, "SignalObjectAndWait"));

  m_Kernel32DispatchTableSystem.LoadLibraryA =
      reinterpret_cast<decltype(LoadLibraryA)*>(GetProcAddress(m_KernelDll, "LoadLibraryA"));

  m_Kernel32DispatchTableSystem.LoadLibraryW =
      reinterpret_cast<decltype(LoadLibraryW)*>(GetProcAddress(m_KernelDll, "LoadLibraryW"));

  m_Kernel32DispatchTableSystem.LoadLibraryExA =
      reinterpret_cast<decltype(LoadLibraryExA)*>(GetProcAddress(m_KernelDll, "LoadLibraryExA"));

  m_Kernel32DispatchTableSystem.LoadLibraryExW =
      reinterpret_cast<decltype(LoadLibraryExW)*>(GetProcAddress(m_KernelDll, "LoadLibraryExW"));

  LONG ret = DetourTransactionBegin();
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourUpdateThread(GetCurrentThread());
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_Kernel32DispatchTableSystem.WaitForSingleObject, WaitForSingleObject);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&m_Kernel32DispatchTableSystem.WaitForSingleObjectEx, WaitForSingleObjectEx);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&m_Kernel32DispatchTableSystem.WaitForMultipleObjects, WaitForMultipleObjects);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&m_Kernel32DispatchTableSystem.WaitForMultipleObjectsEx,
                     WaitForMultipleObjectsEx);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&m_Kernel32DispatchTableSystem.SignalObjectAndWait, SignalObjectAndWait);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&m_Kernel32DispatchTableSystem.LoadLibraryA, MyLoadLibraryA);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&m_Kernel32DispatchTableSystem.LoadLibraryW, MyLoadLibraryW);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&m_Kernel32DispatchTableSystem.LoadLibraryExA, MyLoadLibraryExA);
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourAttach(&m_Kernel32DispatchTableSystem.LoadLibraryExW, MyLoadLibraryExW);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourTransactionCommit();
  GITS_ASSERT(ret == NO_ERROR);
}

void CaptureManager::interceptXessFunctions() {

  if (m_XessDll || m_LoadingXessDll || !Configurator::Get().directx.recorder.captureXess) {
    return;
  }

  m_LoadingXessDll = true;
  m_XessDll = LoadLibrary("libxess.dll");
  m_LoadingXessDll = false;
  if (!m_XessDll) {
    return;
  }

  {
    m_XessDispatchTable.xessGetVersion =
        reinterpret_cast<decltype(xessGetVersion)*>(GetProcAddress(m_XessDll, "xessGetVersion"));

    m_XessDispatchTable.xessGetIntelXeFXVersion =
        reinterpret_cast<decltype(xessGetIntelXeFXVersion)*>(
            GetProcAddress(m_XessDll, "xessGetIntelXeFXVersion"));

    m_XessDispatchTable.xessGetProperties = reinterpret_cast<decltype(xessGetProperties)*>(
        GetProcAddress(m_XessDll, "xessGetProperties"));

    m_XessDispatchTable.xessGetInputResolution =
        reinterpret_cast<decltype(xessGetInputResolution)*>(
            GetProcAddress(m_XessDll, "xessGetInputResolution"));

    m_XessDispatchTable.xessGetOptimalInputResolution =
        reinterpret_cast<decltype(xessGetOptimalInputResolution)*>(
            GetProcAddress(m_XessDll, "xessGetOptimalInputResolution"));

    m_XessDispatchTable.xessGetJitterScale = reinterpret_cast<decltype(xessGetJitterScale)*>(
        GetProcAddress(m_XessDll, "xessGetJitterScale"));

    m_XessDispatchTable.xessGetVelocityScale = reinterpret_cast<decltype(xessGetVelocityScale)*>(
        GetProcAddress(m_XessDll, "xessGetVelocityScale"));

    m_XessDispatchTable.xessDestroyContext = reinterpret_cast<decltype(xessDestroyContext)*>(
        GetProcAddress(m_XessDll, "xessDestroyContext"));

    m_XessDispatchTable.xessSetJitterScale = reinterpret_cast<decltype(xessSetJitterScale)*>(
        GetProcAddress(m_XessDll, "xessSetJitterScale"));

    m_XessDispatchTable.xessSetVelocityScale = reinterpret_cast<decltype(xessSetVelocityScale)*>(
        GetProcAddress(m_XessDll, "xessSetVelocityScale"));

    m_XessDispatchTable.xessSetExposureMultiplier =
        reinterpret_cast<decltype(xessSetExposureMultiplier)*>(
            GetProcAddress(m_XessDll, "xessSetExposureMultiplier"));

    m_XessDispatchTable.xessGetExposureMultiplier =
        reinterpret_cast<decltype(xessGetExposureMultiplier)*>(
            GetProcAddress(m_XessDll, "xessGetExposureMultiplier"));

    m_XessDispatchTable.xessIsOptimalDriver = reinterpret_cast<decltype(xessIsOptimalDriver)*>(
        GetProcAddress(m_XessDll, "xessIsOptimalDriver"));

    m_XessDispatchTable.xessForceLegacyScaleFactors =
        reinterpret_cast<decltype(xessForceLegacyScaleFactors)*>(
            GetProcAddress(m_XessDll, "xessForceLegacyScaleFactors"));

    m_XessDispatchTable.xessSetLoggingCallback =
        reinterpret_cast<decltype(xessSetLoggingCallback)*>(
            GetProcAddress(m_XessDll, "xessSetLoggingCallback"));

    m_XessDispatchTable.xessD3D12CreateContext =
        reinterpret_cast<decltype(xessD3D12CreateContext)*>(
            GetProcAddress(m_XessDll, "xessD3D12CreateContext"));

    m_XessDispatchTable.xessD3D12BuildPipelines =
        reinterpret_cast<decltype(xessD3D12BuildPipelines)*>(
            GetProcAddress(m_XessDll, "xessD3D12BuildPipelines"));

    m_XessDispatchTable.xessD3D12Init =
        reinterpret_cast<decltype(xessD3D12Init)*>(GetProcAddress(m_XessDll, "xessD3D12Init"));

    m_XessDispatchTable.xessD3D12GetInitParams =
        reinterpret_cast<decltype(xessD3D12GetInitParams)*>(
            GetProcAddress(m_XessDll, "xessD3D12GetInitParams"));

    m_XessDispatchTable.xessD3D12Execute = reinterpret_cast<decltype(xessD3D12Execute)*>(
        GetProcAddress(m_XessDll, "xessD3D12Execute"));

    m_XessDispatchTable.xessSetMaxResponsiveMaskValue =
        reinterpret_cast<decltype(xessSetMaxResponsiveMaskValue)*>(
            GetProcAddress(m_XessDll, "xessSetMaxResponsiveMaskValue"));

    m_XessDispatchTable.xessGetMaxResponsiveMaskValue =
        reinterpret_cast<decltype(xessGetMaxResponsiveMaskValue)*>(
            GetProcAddress(m_XessDll, "xessGetMaxResponsiveMaskValue"));

    m_XessDispatchTable.xessGetPipelineBuildStatus =
        reinterpret_cast<decltype(xessGetPipelineBuildStatus)*>(
            GetProcAddress(m_XessDll, "xessGetPipelineBuildStatus"));
  }

  xess_version_t xessVersion{};
  xess_result_t res = m_XessDispatchTable.xessGetVersion(&xessVersion);
  GITS_ASSERT(res == XESS_RESULT_SUCCESS);

  LOG_INFO << "Loaded XeSS (libxess.dll) version: " << xessVersion.major << "." << xessVersion.minor
           << "." << xessVersion.patch;

  LONG ret = DetourTransactionBegin();
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourUpdateThread(GetCurrentThread());
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_XessDispatchTable.xessGetVersion, xessGetVersionWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  if (m_XessDispatchTable.xessGetIntelXeFXVersion) {
    ret =
        DetourAttach(&m_XessDispatchTable.xessGetIntelXeFXVersion, xessGetIntelXeFXVersionWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessGetProperties) {
    ret = DetourAttach(&m_XessDispatchTable.xessGetProperties, xessGetPropertiesWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessGetInputResolution) {
    ret = DetourAttach(&m_XessDispatchTable.xessGetInputResolution, xessGetInputResolutionWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessDestroyContext) {
    ret = DetourAttach(&m_XessDispatchTable.xessDestroyContext, xessDestroyContextWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessSetJitterScale) {
    ret = DetourAttach(&m_XessDispatchTable.xessSetJitterScale, xessSetJitterScaleWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessSetVelocityScale) {
    ret = DetourAttach(&m_XessDispatchTable.xessSetVelocityScale, xessSetVelocityScaleWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessGetJitterScale) {
    ret = DetourAttach(&m_XessDispatchTable.xessGetJitterScale, xessGetJitterScaleWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessGetVelocityScale) {
    ret = DetourAttach(&m_XessDispatchTable.xessGetVelocityScale, xessGetVelocityScaleWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessIsOptimalDriver) {
    ret = DetourAttach(&m_XessDispatchTable.xessIsOptimalDriver, xessIsOptimalDriverWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessGetOptimalInputResolution) {
    ret = DetourAttach(&m_XessDispatchTable.xessGetOptimalInputResolution,
                       xessGetOptimalInputResolutionWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessSetExposureMultiplier) {
    ret = DetourAttach(&m_XessDispatchTable.xessSetExposureMultiplier,
                       xessSetExposureMultiplierWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessGetExposureMultiplier) {
    ret = DetourAttach(&m_XessDispatchTable.xessGetExposureMultiplier,
                       xessGetExposureMultiplierWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessForceLegacyScaleFactors) {
    ret = DetourAttach(&m_XessDispatchTable.xessForceLegacyScaleFactors,
                       xessForceLegacyScaleFactorsWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessSetLoggingCallback) {
    ret = DetourAttach(&m_XessDispatchTable.xessSetLoggingCallback, xessSetLoggingCallbackWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessD3D12CreateContext) {
    ret = DetourAttach(&m_XessDispatchTable.xessD3D12CreateContext, xessD3D12CreateContextWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessD3D12Init) {
    ret = DetourAttach(&m_XessDispatchTable.xessD3D12Init, xessD3D12InitWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessD3D12BuildPipelines) {
    ret =
        DetourAttach(&m_XessDispatchTable.xessD3D12BuildPipelines, xessD3D12BuildPipelinesWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessD3D12GetInitParams) {
    ret = DetourAttach(&m_XessDispatchTable.xessD3D12GetInitParams, xessD3D12GetInitParamsWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessD3D12Execute) {
    ret = DetourAttach(&m_XessDispatchTable.xessD3D12Execute, xessD3D12ExecuteWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessSetMaxResponsiveMaskValue) {
    ret = DetourAttach(&m_XessDispatchTable.xessSetMaxResponsiveMaskValue,
                       xessSetMaxResponsiveMaskValueWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessGetMaxResponsiveMaskValue) {
    ret = DetourAttach(&m_XessDispatchTable.xessGetMaxResponsiveMaskValue,
                       xessGetMaxResponsiveMaskValueWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XessDispatchTable.xessGetPipelineBuildStatus) {
    ret = DetourAttach(&m_XessDispatchTable.xessGetPipelineBuildStatus,
                       xessGetPipelineBuildStatusWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }

  ret = DetourTransactionCommit();
  GITS_ASSERT(ret == NO_ERROR);
}

void CaptureManager::interceptXellFunctions() {
  if (m_XellDll || m_LoadingXellDll || !Configurator::Get().directx.recorder.captureXell) {
    return;
  }

  m_LoadingXellDll = true;
  m_XellDll = LoadLibrary("libxell.dll");
  m_LoadingXellDll = false;
  if (!m_XellDll) {
    return;
  }

  m_XellDispatchTable.xellDestroyContext = reinterpret_cast<decltype(xellDestroyContext)*>(
      GetProcAddress(m_XellDll, "xellDestroyContext"));
  m_XellDispatchTable.xellSetSleepMode =
      reinterpret_cast<decltype(xellSetSleepMode)*>(GetProcAddress(m_XellDll, "xellSetSleepMode"));
  m_XellDispatchTable.xellGetSleepMode =
      reinterpret_cast<decltype(xellGetSleepMode)*>(GetProcAddress(m_XellDll, "xellGetSleepMode"));
  m_XellDispatchTable.xellSleep =
      reinterpret_cast<decltype(xellSleep)*>(GetProcAddress(m_XellDll, "xellSleep"));
  m_XellDispatchTable.xellAddMarkerData = reinterpret_cast<decltype(xellAddMarkerData)*>(
      GetProcAddress(m_XellDll, "xellAddMarkerData"));
  m_XellDispatchTable.xellGetVersion =
      reinterpret_cast<decltype(xellGetVersion)*>(GetProcAddress(m_XellDll, "xellGetVersion"));
  m_XellDispatchTable.xellSetLoggingCallback = reinterpret_cast<decltype(xellSetLoggingCallback)*>(
      GetProcAddress(m_XellDll, "xellSetLoggingCallback"));
  m_XellDispatchTable.xellGetFramesReports = reinterpret_cast<decltype(xellGetFramesReports)*>(
      GetProcAddress(m_XellDll, "xellGetFramesReports"));
  m_XellDispatchTable.xellD3D12CreateContext = reinterpret_cast<decltype(xellD3D12CreateContext)*>(
      GetProcAddress(m_XellDll, "xellD3D12CreateContext"));

  xell_version_t xellVersion{};
  xell_result_t res = m_XellDispatchTable.xellGetVersion(&xellVersion);
  GITS_ASSERT(res == XELL_RESULT_SUCCESS);

  LOG_INFO << "Loaded XeLL (libxell.dll) version: " << xellVersion.major << "." << xellVersion.minor
           << "." << xellVersion.patch;

  LONG ret = DetourTransactionBegin();
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourUpdateThread(GetCurrentThread());
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_XellDispatchTable.xellGetVersion, xellGetVersionWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  if (m_XellDispatchTable.xellDestroyContext) {
    ret = DetourAttach(&m_XellDispatchTable.xellDestroyContext, xellDestroyContextWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XellDispatchTable.xellSetSleepMode) {
    ret = DetourAttach(&m_XellDispatchTable.xellSetSleepMode, xellSetSleepModeWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XellDispatchTable.xellGetSleepMode) {
    ret = DetourAttach(&m_XellDispatchTable.xellGetSleepMode, xellGetSleepModeWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XellDispatchTable.xellSleep) {
    ret = DetourAttach(&m_XellDispatchTable.xellSleep, xellSleepWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XellDispatchTable.xellAddMarkerData) {
    ret = DetourAttach(&m_XellDispatchTable.xellAddMarkerData, xellAddMarkerDataWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XellDispatchTable.xellSetLoggingCallback) {
    ret = DetourAttach(&m_XellDispatchTable.xellSetLoggingCallback, xellSetLoggingCallbackWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XellDispatchTable.xellGetFramesReports) {
    ret = DetourAttach(&m_XellDispatchTable.xellGetFramesReports, xellGetFramesReportsWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XellDispatchTable.xellD3D12CreateContext) {
    ret = DetourAttach(&m_XellDispatchTable.xellD3D12CreateContext, xellD3D12CreateContextWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }

  ret = DetourTransactionCommit();
  GITS_ASSERT(ret == NO_ERROR);
}

void CaptureManager::interceptXefgFunctions() {
  if (m_XefgDll || m_LoadingXefgDll || !Configurator::Get().directx.recorder.captureXefg) {
    return;
  }

  m_LoadingXefgDll = true;
  m_XefgDll = LoadLibrary("libxess_fg.dll");
  m_LoadingXefgDll = false;
  if (!m_XefgDll) {
    return;
  }

  m_XefgDispatchTable.xefgSwapChainGetVersion =
      reinterpret_cast<decltype(xefgSwapChainGetVersion)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainGetVersion"));
  m_XefgDispatchTable.xefgSwapChainGetProperties =
      reinterpret_cast<decltype(xefgSwapChainGetProperties)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainGetProperties"));
  m_XefgDispatchTable.xefgSwapChainTagFrameConstants =
      reinterpret_cast<decltype(xefgSwapChainTagFrameConstants)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainTagFrameConstants"));
  m_XefgDispatchTable.xefgSwapChainSetEnabled =
      reinterpret_cast<decltype(xefgSwapChainSetEnabled)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainSetEnabled"));
  m_XefgDispatchTable.xefgSwapChainSetPresentId =
      reinterpret_cast<decltype(xefgSwapChainSetPresentId)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainSetPresentId"));
  m_XefgDispatchTable.xefgSwapChainGetLastPresentStatus =
      reinterpret_cast<decltype(xefgSwapChainGetLastPresentStatus)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainGetLastPresentStatus"));
  m_XefgDispatchTable.xefgSwapChainSetLoggingCallback =
      reinterpret_cast<decltype(xefgSwapChainSetLoggingCallback)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainSetLoggingCallback"));
  m_XefgDispatchTable.xefgSwapChainDestroy = reinterpret_cast<decltype(xefgSwapChainDestroy)*>(
      GetProcAddress(m_XefgDll, "xefgSwapChainDestroy"));
  m_XefgDispatchTable.xefgSwapChainSetLatencyReduction =
      reinterpret_cast<decltype(xefgSwapChainSetLatencyReduction)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainSetLatencyReduction"));
  m_XefgDispatchTable.xefgSwapChainSetSceneChangeThreshold =
      reinterpret_cast<decltype(xefgSwapChainSetSceneChangeThreshold)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainSetSceneChangeThreshold"));
  m_XefgDispatchTable.xefgSwapChainGetPipelineBuildStatus =
      reinterpret_cast<decltype(xefgSwapChainGetPipelineBuildStatus)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainGetPipelineBuildStatus"));
  m_XefgDispatchTable.xefgSwapChainD3D12CreateContext =
      reinterpret_cast<decltype(xefgSwapChainD3D12CreateContext)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12CreateContext"));
  m_XefgDispatchTable.xefgSwapChainD3D12BuildPipelines =
      reinterpret_cast<decltype(xefgSwapChainD3D12BuildPipelines)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12BuildPipelines"));
  m_XefgDispatchTable.xefgSwapChainD3D12InitFromSwapChain =
      reinterpret_cast<decltype(xefgSwapChainD3D12InitFromSwapChain)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12InitFromSwapChain"));
  m_XefgDispatchTable.xefgSwapChainD3D12InitFromSwapChainDesc =
      reinterpret_cast<decltype(xefgSwapChainD3D12InitFromSwapChainDesc)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12InitFromSwapChainDesc"));
  m_XefgDispatchTable.xefgSwapChainD3D12GetSwapChainPtr =
      reinterpret_cast<decltype(xefgSwapChainD3D12GetSwapChainPtr)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12GetSwapChainPtr"));
  m_XefgDispatchTable.xefgSwapChainD3D12TagFrameResource =
      reinterpret_cast<decltype(xefgSwapChainD3D12TagFrameResource)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12TagFrameResource"));
  m_XefgDispatchTable.xefgSwapChainD3D12SetDescriptorHeap =
      reinterpret_cast<decltype(xefgSwapChainD3D12SetDescriptorHeap)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainD3D12SetDescriptorHeap"));
  m_XefgDispatchTable.xefgSwapChainEnableDebugFeature =
      reinterpret_cast<decltype(xefgSwapChainEnableDebugFeature)*>(
          GetProcAddress(m_XefgDll, "xefgSwapChainEnableDebugFeature"));

  xefg_swapchain_version_t xefgVersion{};
  xefg_swapchain_result_t result = m_XefgDispatchTable.xefgSwapChainGetVersion(&xefgVersion);
  GITS_ASSERT(result == XEFG_SWAPCHAIN_RESULT_SUCCESS);

  LOG_INFO << "Loaded XeSS FG (libxess_fg.dll) version: " << xefgVersion.major << "."
           << xefgVersion.minor << "." << xefgVersion.patch;

  LONG ret = DetourTransactionBegin();
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourUpdateThread(GetCurrentThread());
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainGetVersion, xefgSwapChainGetVersionWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  if (m_XefgDispatchTable.xefgSwapChainGetProperties) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainGetProperties,
                       xefgSwapChainGetPropertiesWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainTagFrameConstants) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainTagFrameConstants,
                       xefgSwapChainTagFrameConstantsWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainSetEnabled) {
    ret =
        DetourAttach(&m_XefgDispatchTable.xefgSwapChainSetEnabled, xefgSwapChainSetEnabledWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainSetPresentId) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainSetPresentId,
                       xefgSwapChainSetPresentIdWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainGetLastPresentStatus) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainGetLastPresentStatus,
                       xefgSwapChainGetLastPresentStatusWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainSetLoggingCallback) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainSetLoggingCallback,
                       xefgSwapChainSetLoggingCallbackWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainDestroy) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainDestroy, xefgSwapChainDestroyWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainSetLatencyReduction) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainSetLatencyReduction,
                       xefgSwapChainSetLatencyReductionWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainSetSceneChangeThreshold) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainSetSceneChangeThreshold,
                       xefgSwapChainSetSceneChangeThresholdWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainGetPipelineBuildStatus) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainGetPipelineBuildStatus,
                       xefgSwapChainGetPipelineBuildStatusWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainD3D12CreateContext) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainD3D12CreateContext,
                       xefgSwapChainD3D12CreateContextWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainD3D12BuildPipelines) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainD3D12BuildPipelines,
                       xefgSwapChainD3D12BuildPipelinesWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainD3D12InitFromSwapChain) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainD3D12InitFromSwapChain,
                       xefgSwapChainD3D12InitFromSwapChainWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainD3D12InitFromSwapChainDesc) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainD3D12InitFromSwapChainDesc,
                       xefgSwapChainD3D12InitFromSwapChainDescWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainD3D12GetSwapChainPtr) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainD3D12GetSwapChainPtr,
                       xefgSwapChainD3D12GetSwapChainPtrWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainD3D12TagFrameResource) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainD3D12TagFrameResource,
                       xefgSwapChainD3D12TagFrameResourceWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainD3D12SetDescriptorHeap) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainD3D12SetDescriptorHeap,
                       xefgSwapChainD3D12SetDescriptorHeapWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }
  if (m_XefgDispatchTable.xefgSwapChainEnableDebugFeature) {
    ret = DetourAttach(&m_XefgDispatchTable.xefgSwapChainEnableDebugFeature,
                       xefgSwapChainEnableDebugFeatureWrapper);
    GITS_ASSERT(ret == NO_ERROR);
  }

  ret = DetourTransactionCommit();
  GITS_ASSERT(ret == NO_ERROR);
}

void CaptureManager::loadIntelExtension(const uint32_t& vendorID, const uint32_t& deviceID) {
  if (m_IntelExtensionLoaded || !Configurator::Get().directx.recorder.captureIntelExtensions) {
    return;
  }

  HRESULT result = INTC_LoadExtensionsLibrary(false, vendorID, deviceID);
  if (FAILED(result)) {
    return;
  }
  LOGI << "IntelExtensions - Loaded Intel Extensions for device 0x" << std::hex << deviceID;

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
  if (SUCCEEDED(result)) {
    LOGI << "IntelExtensions - Registered INTC_D3D12_API_CALLBACKS";
  } else {
    LOG_ERROR << "IntelExtensions - INTC_D3D12_RegisterApplicationCallbacks failed!";
  }

  INTCParamValue captureMode{0};
  captureMode.id = INTC_DEVICE_PARAM_CAPTURE_MODE;
  INTCDeviceParams params{0};
  params.NumParamValues = 1;
  params.pParamValues = &captureMode;
  result = INTC_D3D12_SetDeviceParams(&params);
  if (SUCCEEDED(result)) {
    LOGI << "IntelExtensions - Set INTC_DEVICE_PARAM_CAPTURE_MODE";
  } else {
    LOG_ERROR << "IntelExtensions - Failed to set INTC_DEVICE_PARAM_CAPTURE_MODE";
  }

  m_IntelExtensionLoaded = true;
}

void CaptureManager::interceptNvAPIFunctions() {
  {
    NvAPI_Status status = NvAPI_Initialize();
    if (status != NVAPI_OK) {
      return;
    }
    m_NvapiDll = LoadLibrary("nvapi64.dll");
    if (!m_NvapiDll) {
      return;
    }
    status = NvAPI_Unload();
    GITS_ASSERT(status == NVAPI_OK);
  }

  LOG_INFO << "Loaded NvAPI";

  for (const auto& iface : nvapi_interface_table) {
    m_NvapiFunctionIds[iface.func] = iface.id;
  }

  {
    m_NvapiDispatchTable.nvapi_QueryInterface =
        reinterpret_cast<decltype(nvapi_QueryInterfaceWrapper)*>(
            GetProcAddress(m_NvapiDll, "nvapi_QueryInterface"));

    m_NvapiDispatchTable.NvAPI_Initialize =
        (decltype(NvAPI_Initialize)*)m_NvapiDispatchTable.nvapi_QueryInterface(
            m_NvapiFunctionIds.at("NvAPI_Initialize"));

    m_NvapiDispatchTable.NvAPI_Unload =
        (decltype(NvAPI_Unload)*)m_NvapiDispatchTable.nvapi_QueryInterface(
            m_NvapiFunctionIds.at("NvAPI_Unload"));

    m_NvapiDispatchTable.NvAPI_D3D12_SetCreatePipelineStateOptions =
        (decltype(NvAPI_D3D12_SetCreatePipelineStateOptions)*)
            m_NvapiDispatchTable.nvapi_QueryInterface(
                m_NvapiFunctionIds.at("NvAPI_D3D12_SetCreatePipelineStateOptions"));

    m_NvapiDispatchTable.NvAPI_D3D12_SetNvShaderExtnSlotSpace =
        (decltype(NvAPI_D3D12_SetNvShaderExtnSlotSpace)*)m_NvapiDispatchTable.nvapi_QueryInterface(
            m_NvapiFunctionIds.at("NvAPI_D3D12_SetNvShaderExtnSlotSpace"));

    m_NvapiDispatchTable.NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread =
        (decltype(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread)*)
            m_NvapiDispatchTable.nvapi_QueryInterface(
                m_NvapiFunctionIds.at("NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread"));

    m_NvapiDispatchTable.NvAPI_D3D12_BuildRaytracingAccelerationStructureEx =
        (decltype(NvAPI_D3D12_BuildRaytracingAccelerationStructureEx)*)
            m_NvapiDispatchTable.nvapi_QueryInterface(
                m_NvapiFunctionIds.at("NvAPI_D3D12_BuildRaytracingAccelerationStructureEx"));

    m_NvapiDispatchTable.NvAPI_D3D12_BuildRaytracingOpacityMicromapArray =
        (decltype(NvAPI_D3D12_BuildRaytracingOpacityMicromapArray)*)
            m_NvapiDispatchTable.nvapi_QueryInterface(
                m_NvapiFunctionIds.at("NvAPI_D3D12_BuildRaytracingOpacityMicromapArray"));

    m_NvapiDispatchTable.NvAPI_D3D12_RelocateRaytracingOpacityMicromapArray =
        (decltype(NvAPI_D3D12_RelocateRaytracingOpacityMicromapArray)*)
            m_NvapiDispatchTable.nvapi_QueryInterface(
                m_NvapiFunctionIds.at("NvAPI_D3D12_RelocateRaytracingOpacityMicromapArray"));

    m_NvapiDispatchTable.NvAPI_D3D12_EmitRaytracingOpacityMicromapArrayPostbuildInfo =
        (decltype(NvAPI_D3D12_EmitRaytracingOpacityMicromapArrayPostbuildInfo)*)
            m_NvapiDispatchTable.nvapi_QueryInterface(m_NvapiFunctionIds.at(
                "NvAPI_D3D12_EmitRaytracingOpacityMicromapArrayPostbuildInfo"));

    m_NvapiDispatchTable.NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation =
        (decltype(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation)*)
            m_NvapiDispatchTable.nvapi_QueryInterface(m_NvapiFunctionIds.at(
                "NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation"));

    m_NvapiDispatchTable.NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirect =
        (decltype(NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirect)*)
            m_NvapiDispatchTable.nvapi_QueryInterface(
                m_NvapiFunctionIds.at("NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirect"));
  }

  LONG ret = DetourTransactionBegin();
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourUpdateThread(GetCurrentThread());
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_NvapiDispatchTable.nvapi_QueryInterface, nvapi_QueryInterfaceWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_NvapiDispatchTable.NvAPI_Initialize, NvAPI_InitializeWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_NvapiDispatchTable.NvAPI_Unload, NvAPI_UnloadWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_NvapiDispatchTable.NvAPI_D3D12_SetNvShaderExtnSlotSpace,
                     NvAPI_D3D12_SetNvShaderExtnSlotSpaceWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_NvapiDispatchTable.NvAPI_D3D12_SetCreatePipelineStateOptions,
                     NvAPI_D3D12_SetCreatePipelineStateOptionsWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_NvapiDispatchTable.NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread,
                     NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_NvapiDispatchTable.NvAPI_D3D12_BuildRaytracingAccelerationStructureEx,
                     NvAPI_D3D12_BuildRaytracingAccelerationStructureExWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_NvapiDispatchTable.NvAPI_D3D12_BuildRaytracingOpacityMicromapArray,
                     NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_NvapiDispatchTable.NvAPI_D3D12_RelocateRaytracingOpacityMicromapArray,
                     NvAPI_D3D12_RelocateRaytracingOpacityMicromapArrayWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(
      &m_NvapiDispatchTable.NvAPI_D3D12_EmitRaytracingOpacityMicromapArrayPostbuildInfo,
      NvAPI_D3D12_EmitRaytracingOpacityMicromapArrayPostbuildInfoWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret =
      DetourAttach(&m_NvapiDispatchTable.NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation,
                   NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_NvapiDispatchTable.NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirect,
                     NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirectWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourTransactionCommit();
  GITS_ASSERT(ret == NO_ERROR);
}

void CaptureManager::interceptD3D11On12Functions() {
  if (m_D3D11Dll) {
    return;
  }

  m_D3D11Dll = LoadLibrary("d3d11.dll");
  if (!m_D3D11Dll) {
    return;
  }

  {
    m_D3D11On12DispatchTable.D3D11On12CreateDevice =
        reinterpret_cast<decltype(D3D11On12CreateDevice)*>(
            GetProcAddress(m_D3D11Dll, "D3D11On12CreateDevice"));
  }

  LONG ret = DetourTransactionBegin();
  GITS_ASSERT(ret == NO_ERROR);
  ret = DetourUpdateThread(GetCurrentThread());
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourAttach(&m_D3D11On12DispatchTable.D3D11On12CreateDevice, D3D11On12CreateDeviceWrapper);
  GITS_ASSERT(ret == NO_ERROR);

  ret = DetourTransactionCommit();
  GITS_ASSERT(ret == NO_ERROR);
}

} // namespace DirectX
} // namespace gits
