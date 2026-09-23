// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "rayTracingReplayService.h"

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
  void Pre(vkCreateDeviceCommand& command) override;
  void Post(vkCreateDeviceCommand& command) override;
  void Pre(vkDestroyDeviceCommand& command) override;
  void Post(vkGetDeviceQueueCommand& command) override;
  void Post(vkGetDeviceQueue2Command& command) override;
  void Post(vkCreateCommandPoolCommand& command) override;
  void Pre(vkDestroyCommandPoolCommand& command) override;
  void Post(vkAllocateCommandBuffersCommand& command) override;
  void Pre(vkFreeCommandBuffersCommand& command) override;
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

  // Vulkan.Player.Skip* diagnostics
  void Pre(vkCmdBuildAccelerationStructuresKHRCommand& command) override;
  void Pre(vkCmdBuildAccelerationStructuresIndirectKHRCommand& command) override;
  void Pre(vkBuildAccelerationStructuresKHRCommand& command) override;
  void Pre(vkCmdCopyAccelerationStructureKHRCommand& command) override;
  void Pre(vkCmdBuildMicromapsEXTCommand& command) override;
  void Pre(vkBuildMicromapsEXTCommand& command) override;
  void Pre(vkCmdCopyMicromapEXTCommand& command) override;
  void Pre(vkCopyMicromapEXTCommand& command) override;
  void Pre(vkCmdCopyMicromapToMemoryEXTCommand& command) override;
  void Pre(vkCopyMicromapToMemoryEXTCommand& command) override;
  void Pre(vkCmdCopyMemoryToMicromapEXTCommand& command) override;
  void Pre(vkCopyMemoryToMicromapEXTCommand& command) override;
  void Pre(vkCmdWriteMicromapsPropertiesEXTCommand& command) override;
  void Pre(vkWriteMicromapsPropertiesEXTCommand& command) override;
  void Pre(vkCmdTraceRaysKHRCommand& command) override;
  void Pre(vkCmdTraceRaysIndirectKHRCommand& command) override;
  void Pre(vkCmdTraceRaysIndirect2KHRCommand& command) override;

  void Pre(vkGetEventStatusCommand& command) override;
  void Post(vkGetEventStatusCommand& command) override;

  // Per-event "pending signal" tracking, delegated to EventPendingSignalService.
  // It mirrors the legacy player's SD()._eventstates[event]->eventUsed guard
  // (vulkanStateTracking.h:2448-2454, applied post-submit from
  // vulkanPlayerRunWrap.h:3120-3230): an event is pending once a host
  // vkSetEvent, or a vkCmdSetEvent (2/2KHR) recorded into a submitted command
  // buffer, has been replayed, and stays pending until it is reset (or
  // destroyed). The event catch-up wait in Post(vkGetEventStatus) is only
  // injected for pending events; polling an event with no pending signal (e.g.
  // one whose SET state was only established by state restore, with nothing
  // left in this replay range to reproduce it) must NOT wait, otherwise the
  // player spins forever.
  void Post(vkSetEventCommand& command) override;
  void Post(vkResetEventCommand& command) override;
  void Post(vkDestroyEventCommand& command) override;
  void Pre(vkBeginCommandBufferCommand& command) override;
  void Post(vkCmdSetEventCommand& command) override;
  void Post(vkCmdSetEvent2Command& command) override;
  void Post(vkCmdSetEvent2KHRCommand& command) override;
  void Post(vkCmdResetEventCommand& command) override;
  void Post(vkCmdResetEvent2Command& command) override;
  void Post(vkCmdResetEvent2KHRCommand& command) override;
  void Post(vkCmdExecuteCommandsCommand& command) override;
  void Post(vkResetCommandBufferCommand& command) override;
  void Post(vkResetCommandPoolCommand& command) override;

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
  void Pre(vkCreateRayTracingPipelinesKHRCommand& command) override;

private:
  PlayerManager& m_Manager;
  RayTracingReplayService m_RayTracingService;

  // Serializes the playback thread behind the GPU after every queue submission
  void WaitAfterQueueSubmit(HandleArgument<VkQueue>& queue, VkResult submitResult);
  // Feeds each submitted command buffer's buffered vkCmdSetEvent/vkCmdResetEvent
  // net effects (see EventPendingSignalService) into the pending-event set.
  void ApplySubmittedCommandBufferEventStates(const VkSubmitInfo* pSubmits, uint32_t submitCount);
  void ApplySubmittedCommandBufferEventStates(const VkSubmitInfo2* pSubmits, uint32_t submitCount);
  static thread_local VkResult tl_recorderReturnValue;
  static thread_local uint64_t tl_recorderSemaphoreCounterValue;
  // Backing storage for the filtered ppEnabled{Layer,Extension}Names arrays
  // produced by the Common.Vulkan.Shared.Suppress{Layers,Extensions} handling.
  // Must outlive the create call, hence stored on the layer rather than a local.
  // Layers are instance-only: device-level layers
  // (VkDeviceCreateInfo::ppEnabledLayerNames) were deprecated in Vulkan 1.0.13
  // and are ignored by every conformant loader.
  static thread_local std::vector<const char*> tl_instanceLayerNames;
  static thread_local std::vector<const char*> tl_instanceExtensionNames;
  static thread_local std::vector<const char*> tl_deviceExtensionNames;
};

} // namespace vulkan
} // namespace gits
