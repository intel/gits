// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerManager.h"

#include "messageBus.h"

namespace gits::opengl2 {

PlayerManager& PlayerManager::Get() {
  static PlayerManager instance;
  return instance;
}

PlayerManager::PlayerManager() {
  m_LayerManager.LoadLayers();
  gits::MessageBus::get().subscribe({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                                    [this](Topic, const MessagePtr&) { Shutdown(); });
}

PlayerManager::~PlayerManager() {
  Shutdown();
}

void PlayerManager::Shutdown() {
  if (m_Shutdown) {
    return;
  }
  m_Shutdown = true;
  m_LayerManager.Shutdown();
}

} // namespace gits::opengl2
