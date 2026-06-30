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
      // Apply each submitted command buffer's net event set/reset effects, then
      // advance keyIdx past the command buffer keys to the signal semaphore keys.
      for (uint32_t c = 0; c < info.commandBufferCount && keyIdx < handleKeys.size();
           ++c, ++keyIdx) {
        ApplyCommandBufferEventStates(handleKeys[keyIdx]);
        m_StateTracking.GetQueryPoolStateService().ApplyCommandBuffer(handleKeys[keyIdx]);
      }
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
      // Apply each submitted command buffer's net event set/reset effects, then
      // advance keyIdx past the command buffer info keys to the signal sem keys.
      for (uint32_t c = 0; c < info.commandBufferInfoCount && keyIdx < handleKeys.size();
           ++c, ++keyIdx) {
        ApplyCommandBufferEventStates(handleKeys[keyIdx]);
        m_StateTracking.GetQueryPoolStateService().ApplyCommandBuffer(handleKeys[keyIdx]);
      }
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

void SyncStateService::OnImageAcquired(uint64_t semaphoreKey) {
  if (!semaphoreKey) {
    return;
  }
  SignalBinarySemaphore(semaphoreKey);
}

void SyncStateService::OnResetFences(const std::vector<uint64_t>& fenceKeys) {
  for (uint64_t key : fenceKeys) {
    auto* state = m_StateTracking.GetState(key);
    if (state) {
      static_cast<FenceState*>(state)->IsSignaled = false;
    }
  }
}

void SyncStateService::SignalFence(uint64_t fenceKey) {
  if (!fenceKey) {
    return;
  }
  auto* state = m_StateTracking.GetState(fenceKey);
  if (state) {
    static_cast<FenceState*>(state)->IsSignaled = true;
  }
}

void SyncStateService::UnsignalBinarySemaphore(uint64_t semKey) {
  // GetState<SemaphoreState> is a typed (dynamic_cast) lookup, so a key that
  // resolves to a different object type (e.g. a fence key) is a clean no-op
  // rather than a wrong-type reinterpretation.
  auto* state = m_StateTracking.GetState<SemaphoreState>(semKey);
  if (state && state->IsBinary) {
    state->IsSignaled = false;
  }
}

void SyncStateService::SignalBinarySemaphore(uint64_t semKey) {
  // Typed lookup: see UnsignalBinarySemaphore.  Guards against an acquire/submit
  // HandleKeys slot that turns out not to be a semaphore (e.g. a fence key).
  auto* state = m_StateTracking.GetState<SemaphoreState>(semKey);
  if (state && state->IsBinary) {
    state->IsSignaled = true;
  }
}

void SyncStateService::ApplyCommandBufferEventStates(uint64_t cbKey) {
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cb) {
    return;
  }
  for (const auto& [eventKey, signaled] : cb->EventStatesAfterSubmit) {
    auto* ev = m_StateTracking.GetState<EventState>(eventKey);
    if (ev) {
      ev->IsSignaled = signaled;
    }
  }
}

void SyncStateService::InvalidateCBIfOneTimeSubmit(uint64_t key) {
  auto* objState = m_StateTracking.GetState(key);
  if (!objState || objState->CreationCommandId != CommandId::ID_VKALLOCATECOMMANDBUFFERS) {
    return;
  }
  auto* cbState = static_cast<CommandBufferState*>(objState);
  if (cbState->BeginFlags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) {
    cbState->IsRecording = false;
    cbState->IsExecutable = false;
    cbState->BeginFlags = 0;
    cbState->DependencyKeys.clear();
    cbState->BeginCommandBuffer.clear();
    cbState->EndCommandBuffer.clear();
    cbState->RecordedCommands.clear();
    cbState->RecordedCommandIds.clear();
    cbState->EventStatesAfterSubmit.clear();
    cbState->ResetQueriesAfterSubmit.clear();
    cbState->UsedQueriesAfterSubmit.clear();
  }
}

} // namespace vulkan
} // namespace gits
