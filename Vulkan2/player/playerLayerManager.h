// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"
#include "layerAuto.h"

#include <vector>
#include <memory>

namespace gits {
namespace vulkan {

class PlayerManager;

class PlayerLayerManager : public gits::noncopyable {
public:
  void LoadLayers(PlayerManager& playerManager);

  std::vector<Layer*>& GetPreLayers() {
    return m_PreLayers;
  }

  std::vector<Layer*>& GetPostLayers() {
    return m_PostLayers;
  }

private:
  std::vector<Layer*> m_PreLayers;
  std::vector<Layer*> m_PostLayers;
  std::vector<std::unique_ptr<Layer>> m_LayersOwner;
};

} // namespace vulkan
} // namespace gits
