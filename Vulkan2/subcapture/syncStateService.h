// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <cstdint>
#include <vector>

namespace gits {
namespace vulkan {

class StateTrackingService;

// Tracks the signaled/unsignaled state of binary semaphores and fences across
// queue submits and presents, and invalidates command buffers submitted with
// VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT.
//
// Extracted from SubcaptureLayer to avoid duplicating the HandleKeys layout
// parsing logic across vkQueueSubmit, vkQueueSubmit2, and vkQueueSubmit2KHR.
class SyncStateService {
public:
  explicit SyncStateService(StateTrackingService& sts);

  // Called after a successful vkQueueSubmit.
  // handleKeys: m_pSubmits.HandleKeys from the decoded command.
  // Layout per VkSubmitInfo: [waitSemaphore keys...][commandBuffer keys...][signalSemaphore keys...]
  void OnQueueSubmit(const VkSubmitInfo* pSubmits,
                     uint32_t submitCount,
                     const std::vector<uint64_t>& handleKeys,
                     uint64_t fenceKey);

  // Called after a successful vkQueueSubmit2 / vkQueueSubmit2KHR.
  // handleKeys: m_pSubmits.HandleKeys from the decoded command.
  // Layout per VkSubmitInfo2: [waitSemaphoreInfo keys...][commandBufferInfo keys...][signalSemaphoreInfo keys...]
  void OnQueueSubmit2(const VkSubmitInfo2* pSubmits,
                      uint32_t submitCount,
                      const std::vector<uint64_t>& handleKeys,
                      uint64_t fenceKey);

  // Called after vkQueuePresentKHR (regardless of return value) to unsignal
  // the binary semaphores consumed by the present operation.
  // handleKeys: m_pPresentInfo.HandleKeys from the decoded command.
  // Layout: [waitSemaphore keys...][swapchain keys...]
  void OnQueuePresent(const VkPresentInfoKHR& presentInfo, const std::vector<uint64_t>& handleKeys);

  // Called after a successful vkAcquireNextImageKHR / vkAcquireNextImage2KHR.
  // The presentation engine signals the (binary) semaphore when the image is
  // acquired, so mark it signaled.  If the image was acquired before the
  // subcapture range and the first recorded submit waits on this semaphore,
  // state restore must re-signal it or that submit waits forever (GPU hang).
  // Mirrors the legacy backend's vkAcquireNextImageKHR_SD semaphoreUsed=true.
  // A no-op for a null key or a timeline semaphore.
  void OnImageAcquired(uint64_t semaphoreKey);

  // Called after a successful vkResetFences.
  void OnResetFences(const std::vector<uint64_t>& fenceKeys);

private:
  void SignalFence(uint64_t fenceKey);
  void UnsignalBinarySemaphore(uint64_t semKey);
  void SignalBinarySemaphore(uint64_t semKey);
  // Applies a submitted command buffer's recorded vkCmdSetEvent / vkCmdResetEvent
  // net effects to the corresponding EventState::IsSignaled.  No-op for non-CB keys.
  void ApplyCommandBufferEventStates(uint64_t cbKey);
  // Invalidates the CB's tracked state if it was submitted with ONE_TIME_SUBMIT_BIT.
  // Safe to call with any key - is a no-op for non-CB keys or reusable CBs.
  void InvalidateCBIfOneTimeSubmit(uint64_t key);

  StateTrackingService& m_StateTracking;
};

} // namespace vulkan
} // namespace gits
