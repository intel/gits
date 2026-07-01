// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "swapchainImageSyncService.h"
#include "playerManager.h"
#include "configurator.h"
#include "log.h"

namespace gits {
namespace vulkan {

SwapchainImageSyncService::SwapchainImageSyncService(PlayerManager& manager) : m_Manager(manager) {}

void SwapchainImageSyncService::TrackQueue(VkDevice device, VkQueue queue) {
  // Keep only the first queue seen per device - that is sufficient for both
  // the fake-acquire submit and the rewind-present, mirroring the behaviour of
  // queueStateStoreList[0] in the old backend.
  m_DeviceQueues.emplace(device, queue);
}

VkQueue SwapchainImageSyncService::GetQueueForDevice(VkDevice device) const {
  auto it = m_DeviceQueues.find(device);
  return it != m_DeviceQueues.end() ? it->second : VK_NULL_HANDLE;
}

// ---- Offscreen fake acquire -------------------------------------------------

VkResult SwapchainImageSyncService::AcquireFake(VkDevice device,
                                                VkQueue queue,
                                                VkSemaphore semaphore,
                                                VkFence fence,
                                                uint32_t recordedIndex,
                                                uint32_t* pImageIndex) {
  *pImageIndex = recordedIndex;

  if (queue == VK_NULL_HANDLE) {
    LOG_WARNING << "SwapchainImageSyncService::AcquireFake: no VkQueue for device - cannot "
                   "signal semaphore/fence";
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  // Signal the semaphore and/or fence so downstream work is not blocked.
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.signalSemaphoreCount = (semaphore != VK_NULL_HANDLE) ? 1u : 0u;
  submitInfo.pSignalSemaphores = (semaphore != VK_NULL_HANDLE) ? &semaphore : nullptr;

  return m_Manager.GetDeviceDispatchTable(queue).vkQueueSubmit(queue, 1, &submitInfo, fence);
}

// ---- Normal-mode rewind -----------------------------------------------------

void SwapchainImageSyncService::RewindSwapchainImage(VkDevice device,
                                                     VkQueue queue,
                                                     VkFence fence,
                                                     VkSemaphore semaphore,
                                                     VkSwapchainKHR swapchain,
                                                     uint32_t* pImageIndex) {
  auto& dt = m_Manager.GetDeviceDispatchTable(queue);

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = pImageIndex;

  if (semaphore != VK_NULL_HANDLE) {
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphore;
  }

  if (fence != VK_NULL_HANDLE) {
    const VkResult waitResult = dt.vkWaitForFences(device, 1, &fence, VK_FALSE, 2000000000ULL);
    if (waitResult == VK_TIMEOUT) {
      LOG_WARNING << "SwapchainImageSyncService::RewindSwapchainImage: vkWaitForFences timed out "
                     "before present";
    } else if (waitResult != VK_SUCCESS) {
      LOG_WARNING << "SwapchainImageSyncService::RewindSwapchainImage: vkWaitForFences returned "
                  << waitResult;
    }
    dt.vkResetFences(device, 1, &fence);
  }

  const VkResult presentResult = dt.vkQueuePresentKHR(queue, &presentInfo);
  if (presentResult != VK_SUCCESS && presentResult != VK_SUBOPTIMAL_KHR) {
    LOG_WARNING << "SwapchainImageSyncService::RewindSwapchainImage: vkQueuePresentKHR returned "
                << presentResult;
  }
  const VkResult idleResult = dt.vkQueueWaitIdle(queue);
  if (idleResult != VK_SUCCESS) {
    LOG_WARNING << "SwapchainImageSyncService::RewindSwapchainImage: vkQueueWaitIdle returned "
                << idleResult;
  }
}

// ---- Public handlers --------------------------------------------------------

void SwapchainImageSyncService::HandleAcquireNextImage(vkAcquireNextImageKHRCommand& command) {
  command.m_Skip = true;

  const uint32_t recordedIndex = *command.m_pImageIndex.Value;
  const VkResult recordedResult = command.m_Return.Value;

  VkDevice device = command.m_device.Value;
  VkQueue queue = GetQueueForDevice(device);

  if (Configurator::Get().common.player.renderOffscreen) {
    if (queue == VK_NULL_HANDLE) {
      LOG_WARNING << "SwapchainImageSyncService: no queue tracked for device - cannot fake acquire";
      command.m_Return.Value = VK_ERROR_INITIALIZATION_FAILED;
      return;
    }
    command.m_Return.Value =
        AcquireFake(device, queue, command.m_semaphore.Value, command.m_fence.Value, recordedIndex,
                    command.m_pImageIndex.Value);
    return;
  }

  // Normal rendering: call the driver, then rewind if the indices diverge.
  auto& dt = m_Manager.GetDeviceDispatchTable(device);
  command.m_Return.Value = dt.vkAcquireNextImageKHR(
      device, command.m_swapchain.Value, command.m_timeout.Value, command.m_semaphore.Value,
      command.m_fence.Value, command.m_pImageIndex.Value);

  const bool recordedSuccess =
      (recordedResult == VK_SUCCESS || recordedResult == VK_SUBOPTIMAL_KHR);
  if (!recordedSuccess || *command.m_pImageIndex.Value == recordedIndex) {
    return;
  }

  if (queue == VK_NULL_HANDLE) {
    LOG_WARNING << "SwapchainImageSyncService: no queue tracked for device - cannot rewind";
    return;
  }

  const uint32_t maxRewinds = Configurator::Get().vulkan.player.maxAllowedVkSwapchainRewinds;
  uint32_t rewindCount = 0;

  LOG_TRACE << "vkAcquireNextImageKHR rewind loop begin.";
  while (*command.m_pImageIndex.Value != recordedIndex) {
    if (++rewindCount > maxRewinds) {
      LOG_ERROR << "SwapchainImageSyncService: maximum swapchain rewind limit (" << maxRewinds
                << ") exceeded.";
      break;
    }
    RewindSwapchainImage(device, queue, command.m_fence.Value, command.m_semaphore.Value,
                         command.m_swapchain.Value, command.m_pImageIndex.Value);

    command.m_Return.Value = dt.vkAcquireNextImageKHR(
        device, command.m_swapchain.Value, command.m_timeout.Value, command.m_semaphore.Value,
        command.m_fence.Value, command.m_pImageIndex.Value);
  }
  LOG_TRACE << "vkAcquireNextImageKHR rewind loop end.";
}

void SwapchainImageSyncService::HandleAcquireNextImage2(vkAcquireNextImage2KHRCommand& command) {
  command.m_Skip = true;

  VkAcquireNextImageInfoKHR* acquireInfo = command.m_pAcquireInfo.Value;
  const uint32_t recordedIndex = *command.m_pImageIndex.Value;
  const VkResult recordedResult = command.m_Return.Value;

  VkDevice device = command.m_device.Value;
  VkQueue queue = GetQueueForDevice(device);

  if (Configurator::Get().common.player.renderOffscreen) {
    if (queue == VK_NULL_HANDLE) {
      LOG_WARNING << "SwapchainImageSyncService: no queue tracked for device - cannot fake acquire "
                     "(AcquireNextImage2)";
      command.m_Return.Value = VK_ERROR_INITIALIZATION_FAILED;
      return;
    }
    command.m_Return.Value = AcquireFake(device, queue, acquireInfo->semaphore, acquireInfo->fence,
                                         recordedIndex, command.m_pImageIndex.Value);
    return;
  }

  // Normal rendering: call the driver, then rewind if the indices diverge.
  auto& dt = m_Manager.GetDeviceDispatchTable(device);
  command.m_Return.Value =
      dt.vkAcquireNextImage2KHR(device, acquireInfo, command.m_pImageIndex.Value);

  const bool recordedSuccess =
      (recordedResult == VK_SUCCESS || recordedResult == VK_SUBOPTIMAL_KHR);
  if (!recordedSuccess || *command.m_pImageIndex.Value == recordedIndex) {
    return;
  }

  if (queue == VK_NULL_HANDLE) {
    LOG_WARNING << "SwapchainImageSyncService: no queue tracked for device - cannot rewind";
    return;
  }

  const uint32_t maxRewinds = Configurator::Get().vulkan.player.maxAllowedVkSwapchainRewinds;
  uint32_t rewindCount = 0;

  LOG_TRACE << "vkAcquireNextImage2KHR rewind loop begin.";
  while (*command.m_pImageIndex.Value != recordedIndex) {
    if (++rewindCount > maxRewinds) {
      LOG_ERROR << "SwapchainImageSyncService: maximum swapchain rewind limit (" << maxRewinds
                << ") exceeded.";
      break;
    }
    RewindSwapchainImage(device, queue, acquireInfo->fence, acquireInfo->semaphore,
                         acquireInfo->swapchain, command.m_pImageIndex.Value);

    command.m_Return.Value =
        dt.vkAcquireNextImage2KHR(device, acquireInfo, command.m_pImageIndex.Value);
  }
  LOG_TRACE << "vkAcquireNextImage2KHR rewind loop end.";
}

} // namespace vulkan
} // namespace gits
