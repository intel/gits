// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerLayerManager.h"
#include "replayCustomizationLayer.h"
#include "stateTrackingLayer.h"
#include "recordingLayerAuto.h"
#include "gpuPatchLayer.h"
#include "multithreadedObjectCreationLayer.h"
#include "multithreadedObjectAwaitLayer.h"
#include "portabilityLayer.h"
#include "accelerationStructuresDumpLayer.h"
#include "dstorage/directStorageLayer.h"
#include "printStatusLayer.h"
#include "dllOverrideUseLayer.h"
#include "debugInfoLayerAuto.h"
#include "debugHelperLayer.h"
#include "logDxErrorLayerAuto.h"
#include "imguiHudLayer.h"
#include "gits.h"

namespace gits {
namespace DirectX {

void PlayerLayerManager::loadLayers(PlayerManager& playerManager, PluginService& pluginService) {
  auto& cfg = Configurator::Get().directx;

  // Create layers used by Player
  std::unique_ptr<Layer> replayCustomizationLayer;
  std::unique_ptr<Layer> gpuPatchLayer;
  std::unique_ptr<Layer> multithreadedObjectCreationLayer;
  std::unique_ptr<Layer> multithreadedObjectAwaitLayer;
  std::unique_ptr<Layer> portabilityLayer = portabilityFactory_.getPortabilityLayer();
  std::unique_ptr<Layer> directStorageLayer;
  std::unique_ptr<Layer> traceLayer = traceFactory_.getTraceLayer();
  std::unique_ptr<Layer> showExecutionLayer = traceFactory_.getShowExecutionLayer();
  std::unique_ptr<Layer> debugInfoLayer;
  std::unique_ptr<Layer> debugHelperLayer;
  std::unique_ptr<Layer> logDxErrorLayer = std::make_unique<LogDxErrorLayer>();
  std::unique_ptr<Layer> stateTrackingLayer = subcaptureFactory_.getStateTrackingLayer();
  std::unique_ptr<Layer> recordingLayer = subcaptureFactory_.getRecordingLayer();
  std::unique_ptr<Layer> commandPreservationLayer =
      subcaptureFactory_.getCommandPreservationLayer();
  std::unique_ptr<Layer> analyzerLayer = subcaptureFactory_.getAnalyzerOldLayer();
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
  std::unique_ptr<Layer> printStatusLayer = std::make_unique<PrintStatusLayer>();
  std::unique_ptr<Layer> addressPinningLayer;
  std::unique_ptr<Layer> dllOverrideUseLayer;

  if (cfg.player.execute) {
    replayCustomizationLayer = std::make_unique<ReplayCustomizationLayer>(playerManager);
    if (cfg.player.patchGpuBuffers) {
      gpuPatchLayer = std::make_unique<GpuPatchLayer>(playerManager);
    }
    directStorageLayer = std::make_unique<DirectStorageLayer>();
    debugHelperLayer = std::make_unique<DebugHelperLayer>();
    if (cfg.player.debugLayer) {
      debugInfoLayer = std::make_unique<DebugInfoLayer>();
    }
    if (cfg.player.multithreadedShaderCompilation && !cfg.features.subcapture.enabled) {
      multithreadedObjectCreationLayer =
          std::make_unique<MultithreadedObjectCreationLayer>(playerManager);
      multithreadedObjectAwaitLayer =
          std::make_unique<MultithreadedObjectAwaitLayer>(playerManager);
    }
    addressPinningLayer = addressPinningFactory_.getAddressPinningLayer();
    dllOverrideUseLayer = std::make_unique<DllOverrideUseLayer>(playerManager);
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
  enablePreLayer(multithreadedObjectAwaitLayer);
  enablePreLayer(debugHelperLayer);
  enablePreLayer(traceLayer);
  enablePreLayer(portabilityLayer);
  enablePreLayer(commandPreservationLayer);
  enablePreLayer(dllOverrideUseLayer);
  enablePreLayer(stateTrackingLayer);
  enablePreLayer(executionSerializationLayer);
  enablePreLayer(analyzerLayer);
  enablePreLayer(gpuPatchLayer);
  enablePreLayer(addressPinningLayer);
  enablePreLayer(replayCustomizationLayer);
  enablePreLayer(screenshotsLayer);
  enablePreLayer(debugInfoLayer);
  enablePreLayer(recordingLayer);
  enablePreLayer(logDxErrorLayer);
  enablePreLayer(multithreadedObjectCreationLayer);
  enablePreLayer(directStorageLayer);
  enablePreLayer(accelerationStructuresDumpLayer);
  enablePreLayer(printStatusLayer);
  if (Configurator::IsHudEnabledForApi(ApiBool::DX)) {
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
  enablePostLayer(addressPinningLayer);
  enablePostLayer(replayCustomizationLayer);
  enablePostLayer(gpuPatchLayer);
  enablePostLayer(traceLayer);
  enablePostLayer(showExecutionLayer);
  enablePostLayer(debugHelperLayer);
  enablePostLayer(commandPreservationLayer);
  enablePostLayer(stateTrackingLayer);
  enablePostLayer(analyzerLayer);
  enablePostLayer(screenshotsLayer);
  enablePostLayer(resourceDumpLayer);
  enablePostLayer(renderTargetsDumpLayer);
  enablePostLayer(accelerationStructuresDumpLayer);
  enablePostLayer(rootSignatureDumpLayer);
  enablePostLayer(recordingLayer);
  enablePostLayer(executionSerializationLayer);
  enablePostLayer(printStatusLayer);
  if (Configurator::IsHudEnabledForApi(ApiBool::DX)) {
    enablePostLayer(imGuiHUDLayer);
  }

  for (const auto& plugin : pluginService.getPlugins()) {
    Layer* layer = static_cast<Layer*>(plugin.impl->getImpl());
    // The enable[Pre|Post]Layer lambdas take unique_ptr<Layer>& instead of
    // Layer* to avoid littering their each use with a .get() call. This means
    // we can't use them here, so let's add those layers manually.
    if (layer) {
      preLayers_.push_back(layer);
      postLayers_.push_back(layer);
    }
  }

  PLOGI << "PlayerManager: Initialized with " << preLayers_.size() << " pre-layers and "
        << postLayers_.size() << " post-layers";

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
  retainLayer(std::move(commandPreservationLayer));
  retainLayer(std::move(analyzerLayer));
  retainLayer(std::move(executionSerializationLayer));
  retainLayer(std::move(screenshotsLayer));
  retainLayer(std::move(resourceDumpLayer));
  retainLayer(std::move(renderTargetsDumpLayer));
  retainLayer(std::move(accelerationStructuresDumpLayer));
  retainLayer(std::move(rootSignatureDumpLayer));
  retainLayer(std::move(directStorageLayer));
  retainLayer(std::move(imGuiHUDLayer));
  retainLayer(std::move(printStatusLayer));
  retainLayer(std::move(addressPinningLayer));
  retainLayer(std::move(dllOverrideUseLayer));
}

} // namespace DirectX
} // namespace gits
