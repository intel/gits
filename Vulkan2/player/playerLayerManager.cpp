// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerLayerManager.h"
#include "playerManager.h"
#include "dispatchTablesHolder.h"
#include "pluginService.h"
#include "replayCustomizationLayer.h"
#include "log.h"

namespace gits {
namespace vulkan {

void PlayerLayerManager::LoadLayers(PlayerManager& playerManager, PluginService& pluginService) {

  std::unique_ptr<Layer> replayCustomizationLayer =
      std::make_unique<ReplayCustomizationLayer>(playerManager);

  const auto& traceCfg = Configurator::Get().common.shared.trace;
  const auto& screenshotsCfg = Configurator::Get().common.shared.screenshots;

  std::unique_ptr<Layer> traceLayer;
  if (traceCfg.enabled) {
    m_TraceLayerGroup = std::make_unique<TraceLayerGroup>();
    traceLayer = m_TraceLayerGroup->GetTraceLayer();
  }

  Layer* screenshotLayer = nullptr;
  m_ResourceDumpingLayerGroup =
      std::make_unique<ResourceDumpingLayerGroup>(playerManager.GetDispatchTablesHolder());
  if (screenshotsCfg.enabled) {
    m_ResourceDumpingLayerGroup->loadLayers();
    screenshotLayer = m_ResourceDumpingLayerGroup->getScreenshotLayer();
  }

  auto enablePreLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      m_PreLayers.push_back(layer.get());
    }
  };

  enablePreLayer(replayCustomizationLayer);
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

  enablePostLayer(replayCustomizationLayer);
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

  retainLayer(std::move(replayCustomizationLayer));
  retainLayer(std::move(traceLayer));

  for (const auto& plugin : pluginService.GetPlugins()) {
    Layer* layer = static_cast<Layer*>(plugin.Impl->getImpl());
    if (layer) {
      m_PreLayers.push_back(layer);
      m_PostLayers.push_back(layer);
    }
  }

  PLOGI << "PlayerManager: Initialized with " << m_PreLayers.size() << " pre-layers and "
        << m_PostLayers.size() << " post-layers";
}

} // namespace vulkan
} // namespace gits
