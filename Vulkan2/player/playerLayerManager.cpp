// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerLayerManager.h"
#include "replayCustomizationLayer.h"

namespace gits {
namespace vulkan {

void PlayerLayerManager::LoadLayers(PlayerManager& playerManager) {

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
}

} // namespace vulkan
} // namespace gits
