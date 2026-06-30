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
  void Post(vkMapMemory2Command& command) override;
  void Post(vkMapMemory2KHRCommand& command) override;
  void Pre(vkUnmapMemoryCommand& command) override;
  void Pre(vkUnmapMemory2Command& command) override;
  void Pre(vkUnmapMemory2KHRCommand& command) override;

  void Pre(vkGetFenceStatusCommand& command) override;
  void Post(vkGetFenceStatusCommand& command) override;

  void Pre(vkGetEventStatusCommand& command) override;
  void Post(vkGetEventStatusCommand& command) override;

  void Pre(vkGetSemaphoreCounterValueCommand& command) override;
  void Post(vkGetSemaphoreCounterValueCommand& command) override;

  void Pre(vkGetSemaphoreCounterValueKHRCommand& command) override;
  void Post(vkGetSemaphoreCounterValueKHRCommand& command) override;

  void Pre(vkGetQueryPoolResultsCommand& command) override;
  void Post(vkGetQueryPoolResultsCommand& command) override;

  void Pre(vkWaitForFencesCommand& command) override;
  void Post(vkWaitForFencesCommand& command) override;

  void Pre(vkWaitSemaphoresCommand& command) override;
  void Post(vkWaitSemaphoresCommand& command) override;

  void Pre(vkWaitSemaphoresKHRCommand& command) override;
  void Post(vkWaitSemaphoresKHRCommand& command) override;

  void Pre(vkCreateDebugUtilsMessengerEXTCommand& command) override;
  void Pre(vkCreateDebugReportCallbackEXTCommand& command) override;

  void Post(vkCreateDescriptorUpdateTemplateCommand& command) override;
  void Post(vkCreateDescriptorUpdateTemplateKHRCommand& command) override;
  void Pre(vkDestroyDescriptorUpdateTemplateCommand& command) override;
  void Pre(vkDestroyDescriptorUpdateTemplateKHRCommand& command) override;

  void Pre(vkUpdateDescriptorSetWithTemplateCommand& command) override;
  void Pre(vkUpdateDescriptorSetWithTemplateKHRCommand& command) override;
  void Pre(vkCmdPushDescriptorSetWithTemplateCommand& command) override;
  void Pre(vkCmdPushDescriptorSetWithTemplateKHRCommand& command) override;

private:
  PlayerManager& m_Manager;
  static thread_local VkResult tl_recorderReturnValue;
  static thread_local uint64_t tl_recorderSemaphoreCounterValue;
};

} // namespace vulkan
} // namespace gits
