// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanExecWrap.h
*
* @brief Automatically generated declarations
*
*/

#pragma once

#include <mutex>
#include <unordered_map>
#include "vulkanRecorderWrapper.h"

// Global thread synchronization
// Some apps may first wait on a timeline semaphore on one thread, and later signal that semaphore
// on another thread. This causes a deadlock which is resolved with a std::condition_variable.
//
// In case of a vkQueueSubmit() the following order of operations is required:
// - wrapper schedules vkQueueSubmit() to a stream,
// - drvVk.vkQueueSubmit() is called,
// - std::condition_variable::notify() is called.
//
// For vkWaitSemaphores() the following order of operations is required:
// - std::condition_variable::wait() is called
// - drvVk.vkWaitSemaphores() is called
// - wrapper schedules vkWaitSemaphores() to a stream.

namespace {

extern std::condition_variable_any globalConditionVariable;
extern std::unordered_map<VkSemaphore, uint64_t> globalSemaphoreConditionMap;

} // namespace

namespace gits {
namespace Vulkan {

// vkQueueSubmit

void signalOperation_vkQueueSubmit(VkQueue queue,
                                   uint32_t submitCount,
                                   const VkSubmitInfo* pSubmits,
                                   VkFence fence) {
  for (uint32_t s = 0; s < submitCount; ++s) {
    auto* pTimelineSemaphoreSubmitInfo =
        (const VkTimelineSemaphoreSubmitInfo*)CGitsPluginVulkan::RecorderWrapper()
            .GetPNextStructure(pSubmits[s].pNext, VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO);
    if (pTimelineSemaphoreSubmitInfo && pTimelineSemaphoreSubmitInfo->pSignalSemaphoreValues) {
      for (uint32_t i = 0; i < pSubmits[s].signalSemaphoreCount; ++i) {
        globalSemaphoreConditionMap[pSubmits[s].pSignalSemaphores[i]] =
            pTimelineSemaphoreSubmitInfo->pSignalSemaphoreValues[i];
      }
      globalConditionVariable.notify_one();
    }
  }
}

void signalOperation_vkQueueSubmit2(VkQueue queue,
                                    uint32_t submitCount,
                                    const VkSubmitInfo2* pSubmits,
                                    VkFence fence) {
  for (uint32_t s = 0; s < submitCount; ++s) {
    for (uint32_t i = 0; i < pSubmits[s].signalSemaphoreInfoCount; ++i) {
      globalSemaphoreConditionMap[pSubmits[s].pSignalSemaphoreInfos[i].semaphore] =
          pSubmits[s].pSignalSemaphoreInfos[i].value;
    }
    globalConditionVariable.notify_one();
  }
}

void signalOperation_vkQueueSubmit2KHR(VkQueue queue,
                                       uint32_t submitCount,
                                       const VkSubmitInfo2* pSubmits,
                                       VkFence fence) {
  signalOperation_vkQueueSubmit2(queue, submitCount, pSubmits, fence);
}

// vkSignalSemaphore

void signalOperation_vkSignalSemaphore(VkDevice device, const VkSemaphoreSignalInfo* pSignalInfo) {
  if (pSignalInfo && pSignalInfo->semaphore) {
    globalSemaphoreConditionMap[pSignalInfo->semaphore] = pSignalInfo->value;
    globalConditionVariable.notify_one();
  }
}

void signalOperation_vkSignalSemaphoreKHR(VkDevice device,
                                          const VkSemaphoreSignalInfo* pSignalInfo) {
  signalOperation_vkSignalSemaphore(device, pSignalInfo);
}

// vkWaitSemaphores

void waitOperation_vkWaitSemaphores(VkDevice device,
                                    const VkSemaphoreWaitInfo* pWaitInfo,
                                    uint64_t timeout,
                                    std::unique_lock<std::recursive_mutex>& lock) {
  if (pWaitInfo && pWaitInfo->semaphoreCount) {
    for (uint32_t i = 0; i < pWaitInfo->semaphoreCount; ++i) {
      auto it = globalSemaphoreConditionMap.find(pWaitInfo->pSemaphores[i]);
      if (it == globalSemaphoreConditionMap.end()) {
        globalSemaphoreConditionMap[pWaitInfo->pSemaphores[i]] = 0;
      }
    }

    globalConditionVariable.wait(lock, [pWaitInfo] {
      // Wait any
      if (isBitSet(pWaitInfo->flags, VK_SEMAPHORE_WAIT_ANY_BIT)) {
        for (uint32_t i = 0; i < pWaitInfo->semaphoreCount; ++i) {
          if (globalSemaphoreConditionMap[pWaitInfo->pSemaphores[i]] >= pWaitInfo->pValues[i]) {
            return true;
          }
        }
        return false;
      }
      // Wait all
      else {
        for (uint32_t i = 0; i < pWaitInfo->semaphoreCount; ++i) {
          if (globalSemaphoreConditionMap[pWaitInfo->pSemaphores[i]] < pWaitInfo->pValues[i]) {
            return false;
          }
        }
        return true;
      }
    });
  }
}

void waitOperation_vkWaitSemaphoresKHR(VkDevice device,
                                       const VkSemaphoreWaitInfo* pWaitInfo,
                                       uint64_t timeout,
                                       std::unique_lock<std::recursive_mutex>& lock) {
  waitOperation_vkWaitSemaphores(device, pWaitInfo, timeout, lock);
}

void waitOperation_vkWaitSemaphoresUnifiedGITS(VkDevice device,
                                               const VkSemaphoreWaitInfo* pWaitInfo,
                                               uint64_t timeout,
                                               std::unique_lock<std::recursive_mutex>& lock) {
  waitOperation_vkWaitSemaphores(device, pWaitInfo, timeout, lock);
}

} // namespace Vulkan
} // namespace gits
