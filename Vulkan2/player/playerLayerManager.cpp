// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerLayerManager.h"
#include "pluginService.h"
#include "replayCustomizationLayer.h"
#include "log.h"

namespace gits {
namespace vulkan {

void PlayerLayerManager::LoadLayers(PlayerManager& playerManager, PluginService& pluginService) {

  std::unique_ptr<Layer> replayCustomizationLayer =
      std::make_unique<ReplayCustomizationLayer>(playerManager);

  auto enablePreLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      m_PreLayers.push_back(layer.get());
    }
  };

  enablePreLayer(replayCustomizationLayer);

  auto enablePostLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      m_PostLayers.push_back(layer.get());
    }
  };

  enablePostLayer(replayCustomizationLayer);

  auto retainLayer = [this](std::unique_ptr<Layer>&& layer) {
    if (layer) {
      m_LayersOwner.push_back(std::move(layer));
    }
  };

  retainLayer(std::move(replayCustomizationLayer));

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
