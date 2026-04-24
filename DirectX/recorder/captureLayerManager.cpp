// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureLayerManager.h"
#include "configurator.h"
#include "configurationLib.h"
#include "interceptorCustomizationLayer.h"
#include "encoderLayerAuto.h"
#include "captureCustomizationLayer.h"
#include "captureSynchronizationLayer.h"
#include "gpuPatchLayer.h"
#include "debugInfoLayerAuto.h"
#include "globalSynchronizationLayerAuto.h"
#include "logDxErrorLayerAuto.h"
#include "dllOverrideStoreLayer.h"
#include "imguiHudLayer.h"

namespace gits {
namespace DirectX {

void CaptureLayerManager::LoadLayers(CaptureManager& captureManager,
                                     stream::OrderingRecorder& gitsRecorder,
                                     GpuAddressService& gpuAddressService,
                                     PluginService& pluginService) {
  auto& cfg = Configurator::Get();

  // Load layers from LayerGroups
  m_TraceLayerGroup.LoadLayers();
  m_ResourceDumpingLayerGroup.LoadLayers();
  m_PortabilityLayerGroup.LoadLayers();
  m_AddressPinningLayerGroup.LoadLayers();

  // Get layers from LayerGroups
  Layer* traceLayer = m_TraceLayerGroup.GetLayer("Trace");
  Layer* screenshotsLayer = m_ResourceDumpingLayerGroup.GetLayer("Screenshots");
  Layer* portabilityLayer = nullptr;
  Layer* addressPinningLayer = nullptr;

  // Create individual layers
  std::unique_ptr<Layer> interceptorCustomizationLayer =
      std::make_unique<InterceptorCustomizationLayer>();
  std::unique_ptr<Layer> logDxErrorLayer = std::make_unique<LogDxErrorLayer>();
  std::unique_ptr<Layer> debugInfoLayer;
  std::unique_ptr<Layer> captureCustomizationLayer;
  std::unique_ptr<Layer> captureSynchronizationLayer;
  std::unique_ptr<Layer> encoderLayer;
  std::unique_ptr<Layer> gpuPatchLayer;
  std::unique_ptr<Layer> imGuiHUDLayer;
  std::unique_ptr<Layer> dllOverrideStoreLayer;
  std::unique_ptr<Layer> globalSynchronizationLayer;

  if (cfg.directx.recorder.debugLayer) {
    debugInfoLayer = std::make_unique<DebugInfoLayer>();
  }

  if (cfg.directx.recorder.forceGlobalSynchronization) {
    globalSynchronizationLayer = std::make_unique<GlobalSynchronizationLayer>();
  }

  if (cfg.common.recorder.enabled) {
    captureCustomizationLayer =
        std::make_unique<CaptureCustomizationLayer>(captureManager, gitsRecorder);
    captureSynchronizationLayer = std::make_unique<CaptureSynchronizationLayer>(captureManager);
    encoderLayer = std::make_unique<EncoderLayer>(gitsRecorder);
    gpuPatchLayer = std::make_unique<GpuPatchLayer>(gpuAddressService);
    portabilityLayer = m_PortabilityLayerGroup.GetLayer("Portability");
    addressPinningLayer = m_AddressPinningLayerGroup.GetLayer("AddressPinningStore");
    if (!addressPinningLayer) {
      addressPinningLayer = m_AddressPinningLayerGroup.GetLayer("AddressPinningUse");
    }
    dllOverrideStoreLayer = std::make_unique<DllOverrideStoreLayer>(captureManager, gitsRecorder);
  }

  if (Configurator::IsHudEnabledForApi(ApiBool::DX)) {
    imGuiHUDLayer = std::make_unique<ImGuiHUDLayer>();
  }

  // Enable Pre layers
  // Insertion order determines execution order
  auto enablePreLayer = [this](Layer* layer) {
    if (layer) {
      m_PreLayers.push_back(layer);
    }
  };
  enablePreLayer(globalSynchronizationLayer.get());
  enablePreLayer(traceLayer);
  enablePreLayer(interceptorCustomizationLayer.get());
  enablePreLayer(dllOverrideStoreLayer.get());
  enablePreLayer(captureCustomizationLayer.get());
  enablePreLayer(debugInfoLayer.get());
  enablePreLayer(captureSynchronizationLayer.get());
  enablePreLayer(screenshotsLayer);
  enablePreLayer(portabilityLayer);
  enablePreLayer(imGuiHUDLayer.get());

  // Enable Post layers
  // Insertion order determines execution order
  auto enablePostLayer = [this](Layer* layer) {
    if (layer) {
      m_PostLayers.push_back(layer);
    }
  };
  enablePostLayer(logDxErrorLayer.get());
  enablePostLayer(interceptorCustomizationLayer.get());
  enablePostLayer(dllOverrideStoreLayer.get());
  enablePostLayer(captureCustomizationLayer.get());
  enablePostLayer(portabilityLayer);
  enablePostLayer(debugInfoLayer.get());
  enablePostLayer(encoderLayer.get());
  enablePostLayer(gpuPatchLayer.get());
  enablePostLayer(addressPinningLayer);
  enablePostLayer(captureSynchronizationLayer.get());
  enablePostLayer(traceLayer);
  enablePostLayer(screenshotsLayer);
  enablePostLayer(imGuiHUDLayer.get());

  // Enable plugin layers
  for (const auto& plugin : pluginService.getPlugins()) {
    Layer* layer = static_cast<Layer*>(plugin.impl->getImpl());
    enablePreLayer(layer);
    enablePostLayer(layer);
  }

  // keep as last
  enablePostLayer(globalSynchronizationLayer.get());

  // Retain ownership of layers
  auto retainLayer = [this](std::unique_ptr<Layer>&& layer) {
    if (layer) {
      m_LayersOwner.push_back(std::move(layer));
    }
  };
  retainLayer(std::move(interceptorCustomizationLayer));
  retainLayer(std::move(captureCustomizationLayer));
  retainLayer(std::move(captureSynchronizationLayer));
  retainLayer(std::move(encoderLayer));
  retainLayer(std::move(gpuPatchLayer));
  retainLayer(std::move(debugInfoLayer));
  retainLayer(std::move(logDxErrorLayer));
  retainLayer(std::move(imGuiHUDLayer));
  retainLayer(std::move(dllOverrideStoreLayer));
  retainLayer(std::move(globalSynchronizationLayer));
}

} // namespace DirectX
} // namespace gits
