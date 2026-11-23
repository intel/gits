// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "playerLayerManager.h"

#include <vector>

namespace gits::opengl {

class Layer;

class PlayerManager {
public:
  static PlayerManager& Get();

  // Idempotent. Invalidates layer views. Called by TOPIC_PROGRAM_EXIT handler and destructor.
  void Shutdown();

  bool ExecuteCommands() const {
    return true;
  }

  const std::vector<Layer*>& GetPreLayers() const {
    return m_LayerManager.GetPreLayers();
  }
  const std::vector<Layer*>& GetPostLayers() const {
    return m_LayerManager.GetPostLayers();
  }

private:
  PlayerManager();
  ~PlayerManager();
  PlayerManager(const PlayerManager&) = delete;
  PlayerManager& operator=(const PlayerManager&) = delete;

  PlayerLayerManager m_LayerManager;
  bool m_Shutdown{false};
};

} // namespace gits::opengl
