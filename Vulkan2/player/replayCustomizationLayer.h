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

class ReplayCustomizationLayer : public Layer {
public:
  ReplayCustomizationLayer(PlayerManager& manager)
      : Layer("ReplayCustomization"), m_Manager(manager) {}

  void Post(vkCreateInstanceCommand& command) override;
  void Post(vkCreateDeviceCommand& command) override;
#ifdef VK_USE_PLATFORM_WIN32_KHR
  void Pre(vkCreateWin32SurfaceKHRCommand& command) override;
#endif
  void Post(vkAllocateMemoryCommand& command) override;
  void Post(vkMapMemoryCommand& command) override;
  void Pre(vkUnmapMemoryCommand& command) override;

private:
  PlayerManager& m_Manager;
};

} // namespace vulkan
} // namespace gits
