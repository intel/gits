// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "syncStateService.h"
#include "stateTrackingService.h"
#include "objectState.h"
#include "commandIdsAuto.h"

namespace gits {
namespace vulkan {

SyncStateService::SyncStateService(StateTrackingService& sts) : m_StateTracking(sts) {}

void SyncStateService::OnQueueSubmit(const VkSubmitInfo* pSubmits,
                                     uint32_t submitCount,
                                     const std::vector<uint64_t>& handleKeys,
                                     uint64_t fenceKey) {
  SignalFence(fenceKey);

  if (pSubmits && !handleKeys.empty()) {
    uint32_t keyIdx = 0;
    for (uint32_t i = 0; i < submitCount; ++i) {
      const VkSubmitInfo& info = pSubmits[i];
      for (uint32_t w = 0; w < info.waitSemaphoreCount && keyIdx < handleKeys.size();
           ++w, ++keyIdx) {
        UnsignalBinarySemaphore(handleKeys[keyIdx]);
      }
      // Skip command buffer keys to reach the signal semaphore keys.
      keyIdx += std::min<uint32_t>(info.commandBufferCount,
                                   static_cast<uint32_t>(handleKeys.size()) - keyIdx);
      for (uint32_t s = 0; s < info.signalSemaphoreCount && keyIdx < handleKeys.size();
           ++s, ++keyIdx) {
        SignalBinarySemaphore(handleKeys[keyIdx]);
      }
    }
  }

  for (uint64_t key : handleKeys) {
    InvalidateCBIfOneTimeSubmit(key);
  }
}

void SyncStateService::OnQueueSubmit2(const VkSubmitInfo2* pSubmits,
                                      uint32_t submitCount,
                                      const std::vector<uint64_t>& handleKeys,
                                      uint64_t fenceKey) {
  SignalFence(fenceKey);

  if (pSubmits && !handleKeys.empty()) {
    uint32_t keyIdx = 0;
    for (uint32_t i = 0; i < submitCount; ++i) {
      const VkSubmitInfo2& info = pSubmits[i];
      for (uint32_t w = 0; w < info.waitSemaphoreInfoCount && keyIdx < handleKeys.size();
           ++w, ++keyIdx) {
        UnsignalBinarySemaphore(handleKeys[keyIdx]);
      }
      // Skip command buffer info keys to reach the signal semaphore info keys.
      keyIdx += std::min<uint32_t>(info.commandBufferInfoCount,
                                   static_cast<uint32_t>(handleKeys.size()) - keyIdx);
      for (uint32_t s = 0; s < info.signalSemaphoreInfoCount && keyIdx < handleKeys.size();
           ++s, ++keyIdx) {
        SignalBinarySemaphore(handleKeys[keyIdx]);
      }
    }
  }

  for (uint64_t key : handleKeys) {
    InvalidateCBIfOneTimeSubmit(key);
  }
}

void SyncStateService::OnQueuePresent(const VkPresentInfoKHR& presentInfo,
                                      const std::vector<uint64_t>& handleKeys) {
  for (uint32_t w = 0; w < presentInfo.waitSemaphoreCount && w < handleKeys.size(); ++w) {
    UnsignalBinarySemaphore(handleKeys[w]);
  }
}

void SyncStateService::OnResetFences(const std::vector<uint64_t>& fenceKeys) {
  for (uint64_t key : fenceKeys) {
    auto* state = m_StateTracking.GetState(key);
    if (state) {
      static_cast<FenceState*>(state)->isSignaled = false;
    }
  }
}

void SyncStateService::SignalFence(uint64_t fenceKey) {
  if (!fenceKey) {
    return;
  }
  auto* state = m_StateTracking.GetState(fenceKey);
  if (state) {
    static_cast<FenceState*>(state)->isSignaled = true;
  }
}

void SyncStateService::UnsignalBinarySemaphore(uint64_t semKey) {
  auto* state = m_StateTracking.GetState(semKey);
  if (state && static_cast<SemaphoreState*>(state)->isBinary) {
    static_cast<SemaphoreState*>(state)->isSignaled = false;
  }
}

void SyncStateService::SignalBinarySemaphore(uint64_t semKey) {
  auto* state = m_StateTracking.GetState(semKey);
  if (state && static_cast<SemaphoreState*>(state)->isBinary) {
    static_cast<SemaphoreState*>(state)->isSignaled = true;
  }
}

void SyncStateService::InvalidateCBIfOneTimeSubmit(uint64_t key) {
  auto* objState = m_StateTracking.GetState(key);
  if (!objState || objState->creationCommandId != CommandId::ID_VKALLOCATECOMMANDBUFFERS) {
    return;
  }
  auto* cbState = static_cast<CommandBufferState*>(objState);
  if (cbState->beginFlags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) {
    cbState->isRecording = false;
    cbState->isExecutable = false;
    cbState->beginFlags = 0;
    cbState->dependencyKeys.clear();
    cbState->beginCommandBuffer.clear();
    cbState->endCommandBuffer.clear();
    cbState->recordedCommands.clear();
    cbState->recordedCommandIds.clear();
  }
}

} // namespace vulkan
} // namespace gits
