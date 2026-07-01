// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"
#include "commandsAuto.h"

#include <unordered_map>

namespace gits {
namespace vulkan {

class PlayerManager;

// Handles vkAcquireNextImageKHR and vkAcquireNextImage2KHR replay, including:
//
//  - Offscreen mode: the swapchain is never presented, so no real acquire is
//    performed.  Instead the recorded image index is used directly and a fake
//    vkQueueSubmit is issued to signal the requested semaphore / fence.
//
//  - Normal mode: the real acquire is called.  If the returned image index
//    differs from the one the application saw at capture time, the swapchain is
//    "rewound" by presenting and re-acquiring until the indices agree, matching
//    the behaviour of the old Vulkan backend's vkAcquireNextImageKHR_WRAPRUN.
//
// The service also tracks the first VkQueue obtained per VkDevice so that the
// offscreen fake-submit and the rewind-present can always find a queue without
// requiring callers to supply one explicitly.
class SwapchainImageSyncService {
public:
  explicit SwapchainImageSyncService(PlayerManager& manager);

  // Record the first queue retrieved for a device.
  // Call this from Post(vkGetDeviceQueueCommand) and Post(vkGetDeviceQueue2Command).
  void TrackQueue(VkDevice device, VkQueue queue);

  // Fully handles vkAcquireNextImageKHR replay.
  // Sets command.m_Skip = true so the auto-generated runner does not issue a
  // redundant driver call.  command.m_Return.Value is set to the VkResult and
  // *command.m_pImageIndex.Value is set to the correct image index on return.
  void HandleAcquireNextImage(vkAcquireNextImageKHRCommand& command);

  // Fully handles vkAcquireNextImage2KHR replay (same contract as above).
  void HandleAcquireNextImage2(vkAcquireNextImage2KHRCommand& command);

private:
  // Offscreen path: use the recorded index, submit an empty batch to signal
  // the optional semaphore and fence.
  VkResult AcquireFake(VkDevice device,
                       VkQueue queue,
                       VkSemaphore semaphore,
                       VkFence fence,
                       uint32_t recordedIndex,
                       uint32_t* pImageIndex);

  // Present the swapchain at its current index so that the next acquire can
  // return a different index.  Used by the rewind loop in normal mode.
  void RewindSwapchainImage(VkDevice device,
                            VkQueue queue,
                            VkFence fence,
                            VkSemaphore semaphore,
                            VkSwapchainKHR swapchain,
                            uint32_t* pImageIndex);

  // Return the first queue tracked for this device, or VK_NULL_HANDLE.
  VkQueue GetQueueForDevice(VkDevice device) const;

  PlayerManager& m_Manager;
  // device → first queue seen for that device
  std::unordered_map<VkDevice, VkQueue> m_DeviceQueues;
};

} // namespace vulkan
} // namespace gits
