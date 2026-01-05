// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureLayerManager.h"
#include "interceptorCustomizationLayer.h"
#include "encoderLayerAuto.h"
#include "captureCustomizationLayer.h"
#include "captureSynchronizationLayer.h"
#include "gpuPatchLayer.h"
#include "portabilityLayer.h"
#include "debugInfoLayerAuto.h"
#include "globalSynchronizationLayerAuto.h"
#include "logDxErrorLayerAuto.h"
#include "dllOverrideStoreLayer.h"
#include "imguiHudLayer.h"

namespace gits {
namespace DirectX {

void CaptureLayerManager::loadLayers(CaptureManager& captureManager,
                                     GitsRecorder& gitsRecorder,
                                     GpuAddressService& gpuAddressService,
                                     PluginService& pluginService) {
  auto& cfg = Configurator::Get();

  std::unique_ptr<Layer> interceptorCustomizationLayer =
      std::make_unique<InterceptorCustomizationLayer>();
  std::unique_ptr<Layer> logDxErrorLayer = std::make_unique<LogDxErrorLayer>();
  std::unique_ptr<Layer> traceLayer = traceFactory_.getTraceLayer();
  std::unique_ptr<Layer> screenshotsLayer = resourceDumpingFactory_.getScreenshotsLayer();
  std::unique_ptr<Layer> debugInfoLayer;
  std::unique_ptr<Layer> captureCustomizationLayer;
  std::unique_ptr<Layer> captureSynchronizationLayer;
  std::unique_ptr<Layer> encoderLayer;
  std::unique_ptr<Layer> gpuPatchLayer;
  std::unique_ptr<Layer> portabilityLayer;
  std::unique_ptr<Layer> imGuiHUDLayer = std::make_unique<ImGuiHUDLayer>();
  std::unique_ptr<Layer> addressPinningLayer;
  std::unique_ptr<Layer> dllOverrideStoreLayer;
  std::unique_ptr<Layer> globalSynchronizationLayer;

  if (cfg.directx.capture.debugLayer) {
    debugInfoLayer = std::make_unique<DebugInfoLayer>();
  }

  if (cfg.directx.capture.forceGlobalSynchronization) {
    globalSynchronizationLayer = std::make_unique<GlobalSynchronizationLayer>();
  }

  if (cfg.common.recorder.enabled) {
    captureCustomizationLayer =
        std::make_unique<CaptureCustomizationLayer>(captureManager, gitsRecorder);
    captureSynchronizationLayer = std::make_unique<CaptureSynchronizationLayer>(captureManager);
    encoderLayer = std::make_unique<EncoderLayer>(gitsRecorder);
    gpuPatchLayer = std::make_unique<GpuPatchLayer>(gpuAddressService);
    portabilityLayer = portabilityFactory_.getPortabilityLayer();
    addressPinningLayer = addressPinningFactory_.getAddressPinningLayer();
    dllOverrideStoreLayer = std::make_unique<DllOverrideStoreLayer>(captureManager, gitsRecorder);
  }

  // Enable Pre layers
  // Insertion order determines execution order
  auto enablePreLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      preLayers_.push_back(layer.get());
    }
  };
  enablePreLayer(globalSynchronizationLayer); // keep as first
  enablePreLayer(traceLayer);
  enablePreLayer(interceptorCustomizationLayer);
  enablePreLayer(dllOverrideStoreLayer);
  enablePreLayer(captureCustomizationLayer);
  enablePreLayer(debugInfoLayer);
  enablePreLayer(captureSynchronizationLayer);
  enablePreLayer(screenshotsLayer);
  enablePreLayer(portabilityLayer);
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
  enablePostLayer(logDxErrorLayer);
  enablePostLayer(interceptorCustomizationLayer);
  enablePostLayer(dllOverrideStoreLayer);
  enablePostLayer(captureCustomizationLayer);
  enablePostLayer(portabilityLayer);
  enablePostLayer(debugInfoLayer);
  enablePostLayer(encoderLayer);
  enablePostLayer(gpuPatchLayer);
  enablePostLayer(addressPinningLayer);
  enablePostLayer(captureSynchronizationLayer);
  enablePostLayer(traceLayer);
  enablePostLayer(screenshotsLayer);
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

  // keep as last
  enablePostLayer(globalSynchronizationLayer);

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
  retainLayer(std::move(logDxErrorLayer));
  retainLayer(std::move(screenshotsLayer));
  retainLayer(std::move(portabilityLayer));
  retainLayer(std::move(imGuiHUDLayer));
  retainLayer(std::move(addressPinningLayer));
  retainLayer(std::move(dllOverrideStoreLayer));
  retainLayer(std::move(globalSynchronizationLayer));
}

} // namespace DirectX
} // namespace gits
