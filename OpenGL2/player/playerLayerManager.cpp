// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerLayerManager.h"

#include "layerAuto.h"
#include "traceLayerAuto.h"
#include "traceLayerGroup.h"

namespace gits::opengl2 {

PlayerLayerManager::PlayerLayerManager() = default;

PlayerLayerManager::~PlayerLayerManager() = default;

void PlayerLayerManager::LoadLayers() {
  m_TraceLayerGroup = std::make_unique<TraceLayerGroup>();
  if (m_TraceLayerGroup->IsEnabled()) {
    auto layer = m_TraceLayerGroup->CreateLayer();
    m_PreLayers.push_back(layer.get());
    m_LayersOwner.push_back(std::move(layer));
  }
}

void PlayerLayerManager::Shutdown() {
  // Raw pointers are views into m_LayersOwner and must be invalidated before ownership ends.
  m_PreLayers.clear();
  m_PostLayers.clear();
  m_LayersOwner.clear();
  m_TraceLayerGroup.reset();
}

} // namespace gits::opengl2
