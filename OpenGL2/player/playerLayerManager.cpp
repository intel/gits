// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerLayerManager.h"

#include "layerAuto.h"

namespace gits::opengl2 {

PlayerLayerManager::PlayerLayerManager() = default;

PlayerLayerManager::~PlayerLayerManager() = default;

void PlayerLayerManager::LoadLayers() {
  // Concrete player layers are constructed and ordered here as their features are enabled.
}

void PlayerLayerManager::Shutdown() {
  // Raw pointers are views into m_LayersOwner and must be invalidated before ownership ends.
  m_PreLayers.clear();
  m_PostLayers.clear();
  m_LayersOwner.clear();
}

} // namespace gits::opengl2
