// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerManager.h"
#include "replayCustomizationLayer.h"
#include "gits.h"
#include "stateTrackingLayer.h"
#include "recordingLayerAuto.h"
#include "IPlugin.h"
#include "gpuPatchLayer.h"
#include "multithreadedObjectCreationLayer.h"
#include "multithreadedObjectAwaitLayer.h"
#include "portabilityLayer.h"
#include "accelerationStructuresDumpLayer.h"
#include "dstorage/directStorageLayer.h"
#include "debugInfoLayerAuto.h"
#include "debugHelperLayer.h"
#include "logDxErrorLayerAuto.h"
#include "imguiHudLayer.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

PlayerManager* PlayerManager::instance_ = nullptr;

PlayerManager& PlayerManager::get() {
  if (!instance_) {
    instance_ = new PlayerManager();
    gits::CGits::Instance().RegisterEndPlaybackEvent(PlayerManager::destroy);
  }
  return *instance_;
}

PlayerManager::~PlayerManager() {
  try {
    Log(INFO) << "PlayerManager: Playback completed. Cleaning up...";
    multithreadedObjectCreationService_.shutdown();
    pluginService_.reset();
  } catch (...) {
    topmost_exception_handler("PlayerManager::~PlayerManager");
  }

  if (d3d12CoreDll_) {
    FreeLibrary(d3d12CoreDll_);
  }
  if (dmlDll_) {
    FreeLibrary(dmlDll_);
  }
  if (dmlDebugDll_) {
    FreeLibrary(dmlDebugDll_);
  }
  if (dStorageDll_) {
    FreeLibrary(dStorageDll_);
  }
  if (dStorageCoreDll_) {
    FreeLibrary(dStorageCoreDll_);
  }
}

PlayerManager::PlayerManager() {
  auto& cfg = Configurator::Get();
  executeCommands_ = cfg.directx.player.execute;
  multithreadedShaderCompilation_ =
      cfg.directx.player.multithreadedShaderCompilation && !cfg.directx.features.subcapture.enabled;

  // Create layers used by Player
  std::unique_ptr<Layer> replayCustomizationLayer;
  std::unique_ptr<Layer> gpuPatchLayer;
  std::unique_ptr<Layer> multithreadedObjectCreationLayer;
  std::unique_ptr<Layer> multithreadedObjectAwaitLayer;
  std::unique_ptr<Layer> portabilityLayer;
  std::unique_ptr<Layer> directStorageLayer;
  std::unique_ptr<Layer> traceLayer = traceFactory_.getTraceLayer();
  std::unique_ptr<Layer> showExecutionLayer = traceFactory_.getShowExecutionLayer();
  std::unique_ptr<Layer> debugInfoLayer;
  std::unique_ptr<Layer> debugHelperLayer;
  std::unique_ptr<Layer> logDxErrorLayer = std::make_unique<LogDxErrorLayer>();
  std::unique_ptr<Layer> stateTrackingLayer = subcaptureFactory_.getStateTrackingLayer();
  std::unique_ptr<Layer> recordingLayer = subcaptureFactory_.getRecordingLayer();
  std::unique_ptr<Layer> analyzerLayer = subcaptureFactory_.getAnalyzerLayer();
  std::unique_ptr<Layer> directStorageResourcesLayer =
      subcaptureFactory_.getDirectStorageResourcesLayer();
  std::unique_ptr<Layer> executionSerializationLayer =
      executionSerializationFactory_.getExecutionSerializationLayer();
  std::unique_ptr<Layer> screenshotsLayer = resourceDumpingFactory_.getScreenshotsLayer();
  std::unique_ptr<Layer> resourceDumpLayer = resourceDumpingFactory_.getResourceDumpLayer();
  std::unique_ptr<Layer> renderTargetsDumpLayer =
      resourceDumpingFactory_.getRenderTargetsDumpLayer();
  std::unique_ptr<Layer> accelerationStructuresDumpLayer =
      resourceDumpingFactory_.getAccelerationStructuresDumpLayer();
  std::unique_ptr<Layer> rootSignatureDumpLayer =
      resourceDumpingFactory_.getRootSignatureDumpLayer();
  std::unique_ptr<Layer> skipCallsOnConfigLayer = skipCallsFactory_.getSkipCallsOnConfigLayer();
  std::unique_ptr<Layer> skipCallsOnResultLayer = skipCallsFactory_.getSkipCallsOnResultLayer();
  std::unique_ptr<Layer> imGuiHUDLayer = std::make_unique<ImGuiHUDLayer>();

  if (executeCommands_) {
    replayCustomizationLayer = std::make_unique<ReplayCustomizationLayer>(*this);
    gpuPatchLayer = std::make_unique<GpuPatchLayer>(*this);
    directStorageLayer = std::make_unique<DirectStorageLayer>();
    debugHelperLayer = std::make_unique<DebugHelperLayer>();
    if (cfg.directx.player.debugLayer) {
      debugInfoLayer = std::make_unique<DebugInfoLayer>();
    }
    if (multithreadedShaderCompilation_) {
      multithreadedObjectCreationLayer = std::make_unique<MultithreadedObjectCreationLayer>(*this);
      multithreadedObjectAwaitLayer = std::make_unique<MultithreadedObjectAwaitLayer>(*this);
    }
    portabilityLayer = portabilityFactory_.getPortabilityLayer();
  }

  // Enable Pre layers
  // Insertion order determines execution order
  auto enablePreLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      preLayers_.push_back(layer.get());
    }
  };
  enablePreLayer(skipCallsOnConfigLayer);
  enablePreLayer(skipCallsOnResultLayer);
  enablePreLayer(portabilityLayer);
  enablePreLayer(multithreadedObjectAwaitLayer);
  enablePreLayer(debugHelperLayer);
  enablePreLayer(traceLayer);
  enablePreLayer(stateTrackingLayer);
  enablePreLayer(executionSerializationLayer);
  enablePreLayer(gpuPatchLayer);
  enablePreLayer(replayCustomizationLayer);
  enablePreLayer(screenshotsLayer);
  enablePreLayer(debugInfoLayer);
  enablePreLayer(recordingLayer);
  enablePreLayer(logDxErrorLayer);
  enablePreLayer(multithreadedObjectCreationLayer);
  enablePreLayer(directStorageLayer);
  enablePreLayer(directStorageResourcesLayer);
  enablePreLayer(accelerationStructuresDumpLayer);
  if (cfg.common.shared.hud.enabled) {
    enablePreLayer(imGuiHUDLayer);
  }

  // Enable Post layers
  // Insertion order determines execution order
  auto enablePostLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      postLayers_.push_back(layer.get());
    }
  };
  enablePostLayer(debugInfoLayer);
  enablePostLayer(portabilityLayer);
  enablePostLayer(logDxErrorLayer);
  enablePostLayer(directStorageLayer);
  enablePostLayer(directStorageResourcesLayer);
  enablePostLayer(replayCustomizationLayer);
  enablePostLayer(gpuPatchLayer);
  enablePostLayer(traceLayer);
  enablePostLayer(showExecutionLayer);
  enablePostLayer(debugHelperLayer);
  enablePostLayer(stateTrackingLayer);
  enablePostLayer(analyzerLayer);
  enablePostLayer(screenshotsLayer);
  enablePostLayer(resourceDumpLayer);
  enablePostLayer(renderTargetsDumpLayer);
  enablePostLayer(accelerationStructuresDumpLayer);
  enablePostLayer(rootSignatureDumpLayer);
  enablePostLayer(recordingLayer);
  enablePostLayer(executionSerializationLayer);
  if (cfg.common.shared.hud.enabled) {
    enablePostLayer(imGuiHUDLayer);
  }

  // Let layersOwner_ take the ownership of layers
  auto retainLayer = [this](std::unique_ptr<Layer>&& layer) {
    if (layer) {
      layersOwner_.push_back(std::move(layer));
    }
  };
  retainLayer(std::move(skipCallsOnConfigLayer));
  retainLayer(std::move(skipCallsOnResultLayer));
  retainLayer(std::move(replayCustomizationLayer));
  retainLayer(std::move(portabilityLayer));
  retainLayer(std::move(multithreadedObjectCreationLayer));
  retainLayer(std::move(multithreadedObjectAwaitLayer));
  retainLayer(std::move(traceLayer));
  retainLayer(std::move(showExecutionLayer));
  retainLayer(std::move(debugInfoLayer));
  retainLayer(std::move(debugHelperLayer));
  retainLayer(std::move(logDxErrorLayer));
  retainLayer(std::move(stateTrackingLayer));
  retainLayer(std::move(gpuPatchLayer));
  retainLayer(std::move(recordingLayer));
  retainLayer(std::move(analyzerLayer));
  retainLayer(std::move(executionSerializationLayer));
  retainLayer(std::move(screenshotsLayer));
  retainLayer(std::move(resourceDumpLayer));
  retainLayer(std::move(renderTargetsDumpLayer));
  retainLayer(std::move(accelerationStructuresDumpLayer));
  retainLayer(std::move(rootSignatureDumpLayer));
  retainLayer(std::move(directStorageLayer));
  retainLayer(std::move(directStorageResourcesLayer));
  retainLayer(std::move(imGuiHUDLayer));

  // Load DirectX runtimes
  loadAgilitySdk();
  loadDirectML();
  loadDirectStorage();

  // Load services
  getAdapterService().loadAdapters();
  getXessService().loadXess();
  getIntelExtensionsService().loadIntelExtensions(getAdapterService().getAdapter());
  getIntelExtensionsService().setApplicationInfo();

  pluginService_ = std::make_unique<PluginService>();
  pluginService_->loadPlugins();
  for (const auto& plugin : pluginService_->getPlugins()) {
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

void PlayerManager::addObject(unsigned objectKey, IUnknown* object) {
  objects_[objectKey] = object;
}

void PlayerManager::removeObject(unsigned objectKey) {
  objects_.erase(objectKey);
}

IUnknown* PlayerManager::findObject(unsigned objectKey) {
  auto it = objects_.find(objectKey);
  if (it == objects_.end()) {
    return nullptr;
  }
  return it->second;
}

void PlayerManager::loadAgilitySdk() {
  d3d12CoreDll_ = LoadLibrary(".\\D3D12\\D3D12Core.dll");
  if (!d3d12CoreDll_) {
    Log(ERR) << "Agility SDK - Failed to load (D3D12\\D3D12Core.dll)";
    return;
  }
  UINT sdkVersion = *reinterpret_cast<UINT*>(GetProcAddress(d3d12CoreDll_, "D3D12SDKVersion"));

  Microsoft::WRL::ComPtr<ID3D12SDKConfiguration> sdkConfiguration;
  HRESULT hr = D3D12GetInterface(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(&sdkConfiguration));
  if (hr != S_OK) {
    Log(ERR) << "Agility SDK - D3D12GetInterface(ID3D12SDKConfiguration) failed";
    return;
  }

  hr = sdkConfiguration->SetSDKVersion(sdkVersion, ".\\D3D12\\");
  if (hr != S_OK) {
    Log(ERR) << "Agility SDK - SetSDKVersion call failed. This method can be used only in "
             << "Windows Developer Mode. Check settings !";
    return;
  }

  Log(INFO) << "Agility SDK - Loaded SDK version (" << sdkVersion << ")";
}

void PlayerManager::loadDirectML() {
  dmlDll_ = LoadLibrary(".\\D3D12\\DirectML.dll");
  if (!dmlDll_) {
    Log(ERR) << "Failed to load DirectML runtime (D3D12\\DirectML.dll)";
    return;
  }
  Log(INFO) << "Loaded DirectML runtime (D3D12\\DirectML.dll)";

  if (Configurator::Get().directx.player.debugLayer) {
    dmlDebugDll_ = LoadLibrary(".\\D3D12\\DirectML.Debug.dll");
    if (!dmlDebugDll_) {
      Log(ERR) << "Failed to load DirectML debug runtime (D3D12\\DirectML.Debug.dll)";
      return;
    }
    Log(INFO) << "Loaded DirectML debug runtime (D3D12\\DirectML.Debug.dll)";
  }
}

void PlayerManager::loadDirectStorage() {
  dStorageDll_ = LoadLibrary(".\\D3D12\\dstorage.dll");
  if (!dStorageDll_) {
    Log(ERR) << "DirectStorage - Failed to load (D3D12\\dstorage.dll)";
    return;
  }
  dStorageCoreDll_ = LoadLibrary(".\\D3D12\\dstoragecore.dll");
  if (!dStorageCoreDll_) {
    Log(ERR) << "DirectStorage - Failed to load (D3D12\\dstoragecore.dll)";
    return;
  }
  Log(INFO) << "Loaded DirectStorage runtime (D3D12\\dstorage.dll and D3D12\\dstoragecore.dll)";
}

} // namespace DirectX
} // namespace gits
