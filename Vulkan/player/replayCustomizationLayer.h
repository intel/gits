// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <vector>

namespace gits {
namespace vulkan {

class PlayerManager;

class ReplayCustomizationLayer : public Layer {
public:
  ReplayCustomizationLayer(PlayerManager& manager)
      : Layer("ReplayCustomization"), m_Manager(manager) {}

  void Pre(vkCreateInstanceCommand& command) override;
  void Post(vkCreateInstanceCommand& command) override;
  void Post(vkCreateDeviceCommand& command) override;
  void Post(vkGetDeviceQueueCommand& command) override;
  void Post(vkGetDeviceQueue2Command& command) override;
#ifdef VK_USE_PLATFORM_WIN32_KHR
  void Pre(vkCreateWin32SurfaceKHRCommand& command) override;
#endif
  void Post(vkCreateSwapchainKHRCommand& command) override;
  void Pre(vkQueuePresentKHRCommand& command) override;
#ifdef VK_USE_PLATFORM_XLIB_KHR
  void Pre(vkCreateXlibSurfaceKHRCommand& command) override;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
  void Pre(vkCreateXcbSurfaceKHRCommand& command) override;
  void Pre(vkGetPhysicalDeviceXcbPresentationSupportKHRCommand& command) override;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  void Pre(vkCreateWaylandSurfaceKHRCommand& command) override;
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

  void Pre(vkCreateGraphicsPipelinesCommand& command) override;
  void Pre(vkAcquireNextImageKHRCommand& command) override;
  void Pre(vkAcquireNextImage2KHRCommand& command) override;
  // Per-fence "pending signal" tracking, delegated to FencePendingSignalService.
  // It mirrors the legacy player's SD()._fencestates[fence]->fenceUsed guard
  // (vulkanPlayerRunWrap.h:1000-1004,1422): a fence is pending once a
  // submit/acquire/sparse-bind that signals it has been replayed and stays
  // pending until it is reset (or destroyed).  The fence catch-up wait in
  // Post(vkGetFenceStatus)/Post(vkWaitForFences) is only injected for pending
  // fences; polling a fence with no pending signal (e.g. a subcapture
  // frame-pacing fence whose signalling submit is before the cut) must NOT wait,
  // otherwise the player either deadlocks or stalls on every poll.
  void Post(vkCreateFenceCommand& command) override;
  void Post(vkQueueSubmitCommand& command) override;
  void Post(vkQueueSubmit2Command& command) override;
  void Post(vkQueueSubmit2KHRCommand& command) override;
  void Post(vkQueueBindSparseCommand& command) override;
  void Post(vkAcquireNextImageKHRCommand& command) override;
  void Post(vkAcquireNextImage2KHRCommand& command) override;
  void Post(vkResetFencesCommand& command) override;
  void Post(vkDestroyFenceCommand& command) override;

private:
  PlayerManager& m_Manager;
  static thread_local VkResult tl_recorderReturnValue;
  static thread_local uint64_t tl_recorderSemaphoreCounterValue;
  // Backing storage for the filtered ppEnabledLayerNames array produced by the
  // Common.Vulkan.Shared.SuppressLayers (--suppressVKLayers) handling. Must
  // outlive the create call, hence stored on the layer rather than a local.
  // Instance-only: device-level layers (VkDeviceCreateInfo::ppEnabledLayerNames)
  // were deprecated in Vulkan 1.0.13 and are ignored by every conformant loader.
  static thread_local std::vector<const char*> tl_instanceLayerNames;
};

} // namespace vulkan
} // namespace gits
