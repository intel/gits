// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureLayerManager.h"
#include "captureManager.h"
#include "configurator.h"
#include "pluginService.h"
#include "testLayerAuto.h"
#include "encoderLayerAuto.h"
#include "captureCustomizationLayer.h"

namespace gits {
namespace vulkan {

void CaptureLayerManager::LoadLayers(CaptureManager& captureManager,
                                     stream::OrderingRecorder& recorder,
                                     PluginService& pluginService) {
  auto& cfg = Configurator::Get();

  //std::unique_ptr<Layer> testLayer = std::make_unique<TestLayerAuto>();
  std::unique_ptr<Layer> encoderLayer;
  std::unique_ptr<Layer> captureCustomizationLayer;

  const auto& traceCfg = Configurator::Get().common.shared.trace;
  const auto& screenshotsCfg = Configurator::Get().common.shared.screenshots;

  std::unique_ptr<Layer> traceLayer;
  if (traceCfg.enabled) {
    m_TraceLayerGroup = std::make_unique<TraceLayerGroup>();
    traceLayer = m_TraceLayerGroup->GetTraceLayer();
  }

  Layer* screenshotLayer = nullptr;
  if (screenshotsCfg.enabled) {
    m_ResourceDumpingLayerGroup =
        std::make_unique<ResourceDumpingLayerGroup>(captureManager.GetDispatchTablesHolder());
    m_ResourceDumpingLayerGroup->loadLayers();
    screenshotLayer = m_ResourceDumpingLayerGroup->getScreenshotLayer();
  }

  if (cfg.common.recorder.enabled) {
    captureCustomizationLayer =
        std::make_unique<CaptureCustomizationLayer>(captureManager, recorder);
    encoderLayer = std::make_unique<EncoderLayer>(recorder);
  }

  auto enablePreLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      m_PreLayers.push_back(layer.get());
    }
  };

  //enablePreLayer(testLayer);
  enablePreLayer(captureCustomizationLayer);
  if (traceCfg.enabled && traceCfg.print.preCalls) {
    enablePreLayer(traceLayer);
  }
  if (screenshotsCfg.enabled) {
    m_PreLayers.push_back(screenshotLayer);
  }

  auto enablePostLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      m_PostLayers.push_back(layer.get());
    }
  };

  enablePostLayer(captureCustomizationLayer);
  enablePostLayer(encoderLayer);
  //enablePostLayer(testLayer);
  if (traceCfg.enabled && traceCfg.print.postCalls) {
    enablePostLayer(traceLayer);
  }
  if (screenshotsCfg.enabled) {
    m_PostLayers.push_back(screenshotLayer);
  }

  auto retainLayer = [this](std::unique_ptr<Layer>&& layer) {
    if (layer) {
      m_LayersOwner.push_back(std::move(layer));
    }
  };

  retainLayer(std::move(captureCustomizationLayer));
  retainLayer(std::move(encoderLayer));
  //retainLayer(std::move(testLayer));
  retainLayer(std::move(traceLayer));

  for (const auto& plugin : pluginService.GetPlugins()) {
    Layer* layer = static_cast<Layer*>(plugin.Impl->getImpl());
    if (layer) {
      m_PreLayers.push_back(layer);
      m_PostLayers.push_back(layer);
    }
  }
}

} // namespace vulkan
} // namespace gits
