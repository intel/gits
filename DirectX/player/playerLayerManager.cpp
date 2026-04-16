// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerLayerManager.h"
#include "configurator.h"
#include "configurationLib.h"
#include "replayCustomizationLayer.h"
#include "gpuPatchLayer.h"
#include "multithreadedObjectCreationLayer.h"
#include "multithreadedObjectAwaitLayer.h"
#include "dstorage/directStorageLayer.h"
#include "printStatusLayer.h"
#include "dllOverrideUseLayer.h"
#include "debugInfoLayerAuto.h"
#include "debugHelperLayer.h"
#include "logDxErrorLayerAuto.h"
#include "imguiHudLayer.h"
#include "ccodeLayerAuto.h"
#include "log.h"

namespace gits {
namespace DirectX {

void PlayerLayerManager::loadLayers(PlayerManager& playerManager, PluginService& pluginService) {
  auto& cfg = Configurator::Get().directx;

  // Load layers from LayerGroups
  traceLayerGroup_.loadLayers();
  subcaptureLayerGroup_.loadLayers();
  executionSerializationLayerGroup_.loadLayers();
  resourceDumpingLayerGroup_.loadLayers();
  skipCallsLayerGroup_.loadLayers();
  portabilityLayerGroup_.loadLayers();
  addressPinningLayerGroup_.loadLayers();

  // Get layers from LayerGroups
  Layer* skipCallsOnConfigLayer = skipCallsLayerGroup_.getLayer("SkipCallsOnConfig");
  Layer* skipCallsOnResultLayer = skipCallsLayerGroup_.getLayer("SkipCallsOnResult");
  Layer* traceLayer = traceLayerGroup_.getLayer("Trace");
  Layer* showExecutionLayer = traceLayerGroup_.getLayer("ShowExecution");
  Layer* portabilityLayer = portabilityLayerGroup_.getLayer("Portability");
  Layer* stateTrackingLayer = subcaptureLayerGroup_.getLayer("StateTracking");
  Layer* recordingLayer = subcaptureLayerGroup_.getLayer("Recording");
  Layer* commandPreservationLayer = subcaptureLayerGroup_.getLayer("CommandPreservation");
  Layer* analyzerLayer = subcaptureLayerGroup_.getLayer("Analyzer");
  Layer* executionSerializationLayer =
      executionSerializationLayerGroup_.getLayer("ExecutionSerialization");
  Layer* screenshotsLayer = resourceDumpingLayerGroup_.getLayer("Screenshots");
  Layer* resourceDumpLayer = resourceDumpingLayerGroup_.getLayer("ResourceDump");
  Layer* renderTargetsDumpLayer = resourceDumpingLayerGroup_.getLayer("RenderTargetsDump");
  Layer* dispatchOutputsDumpLayer = resourceDumpingLayerGroup_.getLayer("DispatchOutputsDump");
  Layer* accelerationStructuresDumpLayer =
      resourceDumpingLayerGroup_.getLayer("AccelerationStructuresDump");
  Layer* rootSignatureDumpLayer = resourceDumpingLayerGroup_.getLayer("RootSignatureDump");
  Layer* addressPinningLayer = nullptr;
  if (cfg.player.execute) {
    addressPinningLayer = addressPinningLayerGroup_.getLayer("AddressPinningUse");
    if (!addressPinningLayer) {
      addressPinningLayer = addressPinningLayerGroup_.getLayer("AddressPinningStore");
    }
  }

  // Create individual layers
  std::unique_ptr<Layer> replayCustomizationLayer;
  std::unique_ptr<Layer> gpuPatchLayer;
  std::unique_ptr<Layer> multithreadedObjectCreationLayer;
  std::unique_ptr<Layer> multithreadedObjectAwaitLayer;
  std::unique_ptr<Layer> directStorageLayer;
  std::unique_ptr<Layer> debugInfoLayer;
  std::unique_ptr<Layer> debugHelperLayer;
  std::unique_ptr<Layer> logDxErrorLayer = std::make_unique<LogDxErrorLayer>();
  std::unique_ptr<Layer> imGuiHUDLayer;
  std::unique_ptr<Layer> printStatusLayer = std::make_unique<PrintStatusLayer>();
  std::unique_ptr<Layer> dllOverrideUseLayer;
  std::unique_ptr<Layer> ccodeLayer;

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
    if (cfg.player.multithreadedShaderCompilation && !cfg.features.subcapture.enabled &&
        !cfg.player.cCode.enabled) {
      multithreadedObjectCreationLayer =
          std::make_unique<MultithreadedObjectCreationLayer>(playerManager);
      multithreadedObjectAwaitLayer =
          std::make_unique<MultithreadedObjectAwaitLayer>(playerManager);
    }
    if (Configurator::IsHudEnabledForApi(ApiBool::DX)) {
      imGuiHUDLayer = std::make_unique<ImGuiHUDLayer>();
    }
    dllOverrideUseLayer = std::make_unique<DllOverrideUseLayer>(playerManager);
    if (cfg.player.cCode.enabled) {
      ccodeLayer = std::make_unique<CCodeLayer>();
    }
  }

  // Enable Pre layers
  // Insertion order determines execution order
  auto enablePreLayer = [this](Layer* layer) {
    if (layer) {
      preLayers_.push_back(layer);
    }
  };
  enablePreLayer(skipCallsOnConfigLayer);
  enablePreLayer(skipCallsOnResultLayer);
  enablePreLayer(multithreadedObjectAwaitLayer.get());
  enablePreLayer(debugHelperLayer.get());
  enablePreLayer(traceLayer);
  enablePreLayer(portabilityLayer);
  enablePreLayer(commandPreservationLayer);
  enablePreLayer(dllOverrideUseLayer.get());
  enablePreLayer(stateTrackingLayer);
  enablePreLayer(executionSerializationLayer);
  enablePreLayer(analyzerLayer);
  enablePreLayer(gpuPatchLayer.get());
  enablePreLayer(addressPinningLayer);
  enablePreLayer(replayCustomizationLayer.get());
  enablePreLayer(screenshotsLayer);
  enablePreLayer(debugInfoLayer.get());
  enablePreLayer(recordingLayer);
  enablePreLayer(logDxErrorLayer.get());
  enablePreLayer(multithreadedObjectCreationLayer.get());
  enablePreLayer(directStorageLayer.get());
  enablePreLayer(accelerationStructuresDumpLayer);
  enablePreLayer(printStatusLayer.get());
  enablePreLayer(imGuiHUDLayer.get());
  enablePreLayer(ccodeLayer.get());

  // Enable Post layers
  // Insertion order determines execution order
  auto enablePostLayer = [this](Layer* layer) {
    if (layer) {
      postLayers_.push_back(layer);
    }
  };
  enablePostLayer(debugInfoLayer.get());
  enablePostLayer(portabilityLayer);
  enablePostLayer(logDxErrorLayer.get());
  enablePostLayer(directStorageLayer.get());
  enablePostLayer(addressPinningLayer);
  enablePostLayer(replayCustomizationLayer.get());
  enablePostLayer(gpuPatchLayer.get());
  enablePostLayer(traceLayer);
  enablePostLayer(showExecutionLayer);
  enablePostLayer(debugHelperLayer.get());
  enablePostLayer(commandPreservationLayer);
  enablePostLayer(stateTrackingLayer);
  enablePostLayer(analyzerLayer);
  enablePostLayer(screenshotsLayer);
  enablePostLayer(resourceDumpLayer);
  enablePostLayer(renderTargetsDumpLayer);
  enablePostLayer(dispatchOutputsDumpLayer);
  enablePostLayer(accelerationStructuresDumpLayer);
  enablePostLayer(rootSignatureDumpLayer);
  enablePostLayer(recordingLayer);
  enablePostLayer(executionSerializationLayer);
  enablePostLayer(printStatusLayer.get());
  enablePostLayer(imGuiHUDLayer.get());
  enablePostLayer(ccodeLayer.get());

  // Enable plugin layers
  for (const auto& plugin : pluginService.getPlugins()) {
    Layer* layer = static_cast<Layer*>(plugin.impl->getImpl());
    enablePreLayer(layer);
    enablePostLayer(layer);
  }

  LOG_INFO << "PlayerManager: Initialized with " << preLayers_.size() << " pre-layers and "
           << postLayers_.size() << " post-layers";

  // Let layersOwner_ take the ownership of layers
  auto retainLayer = [this](std::unique_ptr<Layer>&& layer) {
    if (layer) {
      layersOwner_.push_back(std::move(layer));
    }
  };
  retainLayer(std::move(replayCustomizationLayer));
  retainLayer(std::move(gpuPatchLayer));
  retainLayer(std::move(multithreadedObjectCreationLayer));
  retainLayer(std::move(multithreadedObjectAwaitLayer));
  retainLayer(std::move(debugInfoLayer));
  retainLayer(std::move(debugHelperLayer));
  retainLayer(std::move(logDxErrorLayer));
  retainLayer(std::move(directStorageLayer));
  retainLayer(std::move(imGuiHUDLayer));
  retainLayer(std::move(printStatusLayer));
  retainLayer(std::move(dllOverrideUseLayer));
  retainLayer(std::move(ccodeLayer));
}

} // namespace DirectX
} // namespace gits
