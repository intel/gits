// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

namespace gits {
namespace vulkan {

class PlayerManager;

class MultithreadedObjectCreationLayer : public Layer {
public:
  MultithreadedObjectCreationLayer(PlayerManager& manager)
      : Layer("MultithreadedObjectCreation"), m_Manager(manager) {}

  void Pre(vkCreateGraphicsPipelinesCommand& command) override;
  void Pre(vkCreateComputePipelinesCommand& command) override;

private:
  PlayerManager& m_Manager;
};

} // namespace vulkan
} // namespace gits
