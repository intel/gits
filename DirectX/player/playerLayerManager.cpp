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

void PlayerLayerManager::LoadLayers(PlayerManager& playerManager, PluginService& pluginService) {
  auto& cfg = Configurator::Get().directx;

  auto registerResourceCallback = [&playerManager](unsigned resourceKey, ID3D12Resource* resource) {
    if (!resource) {
      return;
    }
    playerManager.AddObject(resourceKey, resource);
    playerManager.GetGpuAddressService().CreateResource(resourceKey, resource);
  };

  // Load layers from LayerGroups
  m_TraceLayerGroup.LoadLayers();
  m_SubcaptureLayerGroup.LoadLayers();
  m_ExecutionSerializationLayerGroup.LoadLayers();
  m_ResourceDumpingLayerGroup.LoadLayers();
  m_SkipCallsLayerGroup.LoadLayers();
  m_PortabilityLayerGroup.LoadLayers(registerResourceCallback);
  m_AddressPinningLayerGroup.LoadLayers();

  // Get layers from LayerGroups
  Layer* skipCallsOnConfigLayer = m_SkipCallsLayerGroup.GetLayer("SkipCallsOnConfig");
  Layer* skipCallsOnResultLayer = m_SkipCallsLayerGroup.GetLayer("SkipCallsOnResult");
  Layer* traceLayer = m_TraceLayerGroup.GetLayer("Trace");
  Layer* showExecutionLayer = m_TraceLayerGroup.GetLayer("ShowExecution");
  Layer* portabilityLayer = m_PortabilityLayerGroup.GetLayer("Portability");
  Layer* stateTrackingLayer = m_SubcaptureLayerGroup.GetLayer("StateTracking");
  Layer* recordingLayer = m_SubcaptureLayerGroup.GetLayer("Recording");
  Layer* commandPreservationLayer = m_SubcaptureLayerGroup.GetLayer("CommandPreservation");
  Layer* analyzerLayer = m_SubcaptureLayerGroup.GetLayer("Analyzer");
  Layer* executionSerializationLayer =
      m_ExecutionSerializationLayerGroup.GetLayer("ExecutionSerialization");
  Layer* screenshotsLayer = m_ResourceDumpingLayerGroup.GetLayer("Screenshots");
  Layer* resourceDumpLayer = m_ResourceDumpingLayerGroup.GetLayer("ResourceDump");
  Layer* renderTargetsDumpLayer = m_ResourceDumpingLayerGroup.GetLayer("RenderTargetsDump");
  Layer* dispatchOutputsDumpLayer = m_ResourceDumpingLayerGroup.GetLayer("DispatchOutputsDump");
  Layer* accelerationStructuresDumpLayer =
      m_ResourceDumpingLayerGroup.GetLayer("AccelerationStructuresDump");
  Layer* rootSignatureDumpLayer = m_ResourceDumpingLayerGroup.GetLayer("RootSignatureDump");
  Layer* addressPinningLayer = nullptr;
  if (cfg.player.execute) {
    addressPinningLayer = m_AddressPinningLayerGroup.GetLayer("AddressPinningUse");
    if (!addressPinningLayer) {
      addressPinningLayer = m_AddressPinningLayerGroup.GetLayer("AddressPinningStore");
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
    if (Configurator::IsHudEnabledForApi(ApiBool::DX)) {
      imGuiHUDLayer = std::make_unique<ImGuiHUDLayer>();
    }
    dllOverrideUseLayer = std::make_unique<DllOverrideUseLayer>(playerManager);
    if (cfg.player.cCode.enabled) {
      ccodeLayer = std::make_unique<CCodeLayer>();
      const_cast<gits::Configuration&>(Configurator::Get())
          .directx.player.multithreadedShaderCompilation = false;
    }
    if (cfg.player.multithreadedShaderCompilation) {
      multithreadedObjectCreationLayer =
          std::make_unique<MultithreadedObjectCreationLayer>(playerManager);
      multithreadedObjectAwaitLayer =
          std::make_unique<MultithreadedObjectAwaitLayer>(playerManager);
    }
  }

  // Enable Pre layers
  // Insertion order determines execution order
  auto enablePreLayer = [this](Layer* layer) {
    if (layer) {
      m_PreLayers.push_back(layer);
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
      m_PostLayers.push_back(layer);
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

  LOG_INFO << "PlayerManager: Initialized with " << m_PreLayers.size() << " pre-layers and "
           << m_PostLayers.size() << " post-layers";

  // Let layersOwner_ take the ownership of layers
  auto retainLayer = [this](std::unique_ptr<Layer>&& layer) {
    if (layer) {
      m_LayersOwner.push_back(std::move(layer));
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
