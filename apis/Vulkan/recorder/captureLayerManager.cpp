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
#include "encoderLayerAuto.h"
#include "logVkErrorLayerAuto.h"
#include "hudLayer.h"

namespace gits {
namespace vulkan {

void CaptureLayerManager::LoadLayers(CaptureManager& captureManager,
                                     stream::OrderingRecorder& recorder,
                                     PluginService& pluginService) {
  auto& cfg = Configurator::Get();

  std::unique_ptr<Layer> logVkErrorLayer = std::make_unique<LogVkErrorLayer>();
  std::unique_ptr<Layer> encoderLayer;
  std::unique_ptr<Layer> hudLayer;

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
    m_CaptureCustomizationLayerGroup =
        std::make_unique<CaptureCustomizationLayerGroup>(captureManager, recorder);
    encoderLayer = std::make_unique<EncoderLayer>(recorder);
  }

  auto enablePreLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      m_PreLayers.push_back(layer.get());
    }
  };

  if (m_CaptureCustomizationLayerGroup) {
    const auto& layers = m_CaptureCustomizationLayerGroup->GetPreLayers();
    m_PreLayers.insert(m_PreLayers.end(), layers.begin(), layers.end());
  }
  if (traceCfg.enabled && traceCfg.print.preCalls) {
    enablePreLayer(traceLayer);
  }
  if (screenshotsCfg.enabled) {
    m_PreLayers.push_back(screenshotLayer);
  }

  if (Configurator::IsHudEnabledForApi(gits::ApiBool::VK)) {
    hudLayer = std::make_unique<HudLayer>(captureManager.GetDispatchTablesHolder());
    enablePreLayer(hudLayer);
  }

  auto enablePostLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      m_PostLayers.push_back(layer.get());
    }
  };

  if (Configurator::IsHudEnabledForApi(gits::ApiBool::VK)) {
    enablePostLayer(hudLayer);
  }

  enablePostLayer(logVkErrorLayer);
  if (m_CaptureCustomizationLayerGroup) {
    const auto& layers = m_CaptureCustomizationLayerGroup->GetPostLayers();
    m_PostLayers.insert(m_PostLayers.end(), layers.begin(), layers.end());
  }
  if (traceCfg.enabled && traceCfg.print.postCalls) {
    enablePostLayer(traceLayer);
  }
  if (screenshotsCfg.enabled) {
    m_PostLayers.push_back(screenshotLayer);
  }

  // The encoder must run last
  enablePostLayer(encoderLayer);

  auto retainLayer = [this](std::unique_ptr<Layer>&& layer) {
    if (layer) {
      m_LayersOwner.push_back(std::move(layer));
    }
  };

  retainLayer(std::move(logVkErrorLayer));
  retainLayer(std::move(traceLayer));

  if (Configurator::IsHudEnabledForApi(gits::ApiBool::VK)) {
    retainLayer(std::move(hudLayer));
  }

  retainLayer(std::move(encoderLayer));

  for (const auto& plugin : pluginService.GetPlugins()) {
    Layer* layer = static_cast<Layer*>(plugin.Impl->getImpl());
    if (layer) {
      m_PreLayers.push_back(layer);
      m_PostLayers.push_back(layer);
    }
  }
}

void CaptureLayerManager::Shutdown() {
  m_PreLayers.clear();
  m_PostLayers.clear();
  m_LayersOwner.clear();
  m_CaptureCustomizationLayerGroup.reset();
  m_ResourceDumpingLayerGroup.reset();
  m_TraceLayerGroup.reset();
}

} // namespace vulkan
} // namespace gits
