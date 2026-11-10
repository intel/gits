// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "submitSignalsDeferral.h"

namespace gits {
namespace vulkan {

namespace {

constexpr const char* g_UnsupportedChain =
    "chains a structure whose operations cannot be moved to another submission";

// The structures that may stay in the submission when its signals are moved out. Others, as keyed
// mutex releases or D3D12 fence values, complete with the signals.
bool KeepsInSubmission(const VkBaseInStructure& node) {
  switch (node.sType) {
  case VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO:
  case VK_STRUCTURE_TYPE_PERFORMANCE_QUERY_SUBMIT_INFO_KHR:
  case VK_STRUCTURE_TYPE_FRAME_BOUNDARY_EXT:
    return true;
  case VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO:
    return reinterpret_cast<const VkDeviceGroupSubmitInfo&>(node).signalSemaphoreCount == 0;
  case VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO:
    return !reinterpret_cast<const VkProtectedSubmitInfo&>(node).protectedSubmit;
  default:
    return false;
  }
}

bool ChainKeepsInSubmission(const void* pNext) {
  for (auto* node = static_cast<const VkBaseInStructure*>(pNext); node != nullptr;
       node = node->pNext) {
    if (!KeepsInSubmission(*node)) {
      return false;
    }
  }
  return true;
}

constexpr const char* g_WaitsAfterSignal = "has a batch waiting for semaphores after an earlier "
                                           "batch of it signals some, and the wait may "
                                           "depend on those signals";

// Whether a batch waits for semaphores after an earlier batch signals some. The wait may depend on
// the signals, directly or through work on other queues, so deferring them past the whole
// submission could make the submission wait for itself.
template <typename Submit, typename WaitCountOf, typename SignalCountOf>
bool WaitsAfterSignal(uint32_t submitCount,
                      const Submit* submits,
                      WaitCountOf waitCountOf,
                      SignalCountOf signalCountOf) {
  bool signaled = false;
  for (uint32_t i = 0; submits && i < submitCount; ++i) {
    if (signaled && waitCountOf(submits[i]) > 0) {
      return true;
    }
    signaled = signaled || signalCountOf(submits[i]) > 0;
  }
  return false;
}

constexpr const char* g_StageScopedSignal =
    "signals semaphores at specific pipeline stages, and signaling them after the copies would "
    "make them wait for the commands at every stage";

constexpr const char* g_WaitBlocksCopies =
    "waits for a semaphore value that may be signaled after the submission, and its signals may "
    "not wait for that value before they are signaled after the copies, which wait for every "
    "semaphore wait of the submission";

// The copies also wait for the semaphore waits of the earlier submissions to the queue, which
// neither the signals nor the fence of the submission wait for. The fence waits for the semaphore
// waits of its own submission already.
constexpr const char* g_EarlierWaitBlocksCopies =
    "is submitted to a queue whose earlier work waits for a semaphore value that may be signaled "
    "after the submission, and its signals may not wait for that value before they are signaled "
    "after the copies, which wait for every semaphore wait before them";

// Whether a signal operation already waits for every command before it, as the deferred one does.
bool SignalsAfterAllCommands(VkPipelineStageFlags2 stages) {
  return stages & (VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
}

// Whether deferring the signals past the copies may make them wait for a semaphore wait whose
// signal depends on them. The barrier before the copies waits for every stage, so the copies wait
// for every semaphore wait of the submission, whatever its stages, while a signal operation of the
// submission does not wait for a semaphore wait unless commands between them execute at the
// stages of the wait. The deferred signals are submitted after every wait, as no batch waits after
// an earlier one signals.
bool WaitBlocksCopies(VkSemaphore semaphore,
                      uint64_t value,
                      VkPipelineStageFlags2 stages,
                      const MayWaitForLaterSignals& mayWaitForLaterSignals) {
  return stages != 0 && mayWaitForLaterSignals(semaphore, value);
}

VkTimelineSemaphoreSubmitInfo* FindTimelineInfo(const void* pNext) {
  for (auto* node = static_cast<const VkBaseInStructure*>(pNext); node != nullptr;
       node = node->pNext) {
    if (node->sType == VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO) {
      return reinterpret_cast<VkTimelineSemaphoreSubmitInfo*>(const_cast<VkBaseInStructure*>(node));
    }
  }
  return nullptr;
}

const char* StageProblem(uint32_t submitCount,
                         const VkSubmitInfo* submits,
                         bool signalsFence,
                         const MayWaitForLaterSignals& mayWaitForLaterSignals,
                         bool earlierWorkMayWaitForLaterSignals) {
  bool signalsSemaphores = false;
  for (uint32_t i = 0; submits && i < submitCount; ++i) {
    signalsSemaphores =
        signalsSemaphores || (submits[i].pSignalSemaphores && submits[i].signalSemaphoreCount > 0);
  }
  if ((signalsSemaphores || signalsFence) && earlierWorkMayWaitForLaterSignals) {
    return g_EarlierWaitBlocksCopies;
  }
  for (uint32_t i = 0; signalsSemaphores && i < submitCount; ++i) {
    const VkSubmitInfo& submit = submits[i];
    const VkTimelineSemaphoreSubmitInfo* timelineInfo = FindTimelineInfo(submit.pNext);
    for (uint32_t j = 0;
         submit.pWaitSemaphores && submit.pWaitDstStageMask && j < submit.waitSemaphoreCount; ++j) {
      const uint64_t value = timelineInfo && timelineInfo->pWaitSemaphoreValues &&
                                     j < timelineInfo->waitSemaphoreValueCount
                                 ? timelineInfo->pWaitSemaphoreValues[j]
                                 : 0;
      if (WaitBlocksCopies(submit.pWaitSemaphores[j], value, submit.pWaitDstStageMask[j],
                           mayWaitForLaterSignals)) {
        return g_WaitBlocksCopies;
      }
    }
  }
  return nullptr;
}

const char* StageProblem(uint32_t submitCount,
                         const VkSubmitInfo2* submits,
                         bool signalsFence,
                         const MayWaitForLaterSignals& mayWaitForLaterSignals,
                         bool earlierWorkMayWaitForLaterSignals) {
  bool signalsSemaphores = false;
  for (uint32_t i = 0; submits && i < submitCount; ++i) {
    const VkSubmitInfo2& submit = submits[i];
    for (uint32_t j = 0; submit.pSignalSemaphoreInfos && j < submit.signalSemaphoreInfoCount; ++j) {
      signalsSemaphores = true;
      if (!SignalsAfterAllCommands(submit.pSignalSemaphoreInfos[j].stageMask)) {
        return g_StageScopedSignal;
      }
    }
  }
  if ((signalsSemaphores || signalsFence) && earlierWorkMayWaitForLaterSignals) {
    return g_EarlierWaitBlocksCopies;
  }
  for (uint32_t i = 0; signalsSemaphores && i < submitCount; ++i) {
    const VkSubmitInfo2& submit = submits[i];
    for (uint32_t j = 0; submit.pWaitSemaphoreInfos && j < submit.waitSemaphoreInfoCount; ++j) {
      const VkSemaphoreSubmitInfo& wait = submit.pWaitSemaphoreInfos[j];
      if (WaitBlocksCopies(wait.semaphore, wait.value, wait.stageMask, mayWaitForLaterSignals)) {
        return g_WaitBlocksCopies;
      }
    }
  }
  return nullptr;
}

} // namespace

const char* SubmitSignalsDeferral::Defer(uint32_t submitCount,
                                         VkSubmitInfo* submits,
                                         VkFence& fence,
                                         const MayWaitForLaterSignals& mayWaitForLaterSignals,
                                         bool earlierWorkMayWaitForLaterSignals) {
  for (uint32_t i = 0; submits && i < submitCount; ++i) {
    if (!ChainKeepsInSubmission(submits[i].pNext)) {
      return g_UnsupportedChain;
    }
  }
  if (WaitsAfterSignal(
          submitCount, submits,
          [](const VkSubmitInfo& s) { return s.pWaitSemaphores ? s.waitSemaphoreCount : 0; },
          [](const VkSubmitInfo& s) { return s.pSignalSemaphores ? s.signalSemaphoreCount : 0; })) {
    return g_WaitsAfterSignal;
  }
  if (const char* problem =
          StageProblem(submitCount, submits, fence != VK_NULL_HANDLE, mayWaitForLaterSignals,
                       earlierWorkMayWaitForLaterSignals)) {
    return problem;
  }
  SubmitSignals signals;
  signals.SubmittedWith = SubmitSignals::Function::QueueSubmit;
  signals.Fence = fence;
  for (uint32_t i = 0; submits && i < submitCount; ++i) {
    VkSubmitInfo& submit = submits[i];
    VkTimelineSemaphoreSubmitInfo* timelineInfo = FindTimelineInfo(submit.pNext);
    std::vector<VkSemaphoreSubmitInfo> batch;
    for (uint32_t j = 0; submit.pSignalSemaphores && j < submit.signalSemaphoreCount; ++j) {
      VkSemaphoreSubmitInfo signal{};
      signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
      signal.semaphore = submit.pSignalSemaphores[j];
      if (timelineInfo && timelineInfo->pSignalSemaphoreValues &&
          j < timelineInfo->signalSemaphoreValueCount) {
        signal.value = timelineInfo->pSignalSemaphoreValues[j];
      }
      signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
      batch.push_back(signal);
    }
    if (!batch.empty()) {
      signals.Batches.push_back(std::move(batch));
    }
    signals.TimelineValues = signals.TimelineValues || timelineInfo;
    m_SignalCounts.push_back(submit.signalSemaphoreCount);
    m_TimelineInfos.push_back(timelineInfo);
    m_TimelineValueCounts.push_back(timelineInfo ? timelineInfo->signalSemaphoreValueCount : 0);
  }
  // Batches may share a VkTimelineSemaphoreSubmitInfo, so every batch is captured before any is
  // cleared.
  for (uint32_t i = 0; submits && i < submitCount; ++i) {
    submits[i].signalSemaphoreCount = 0;
    if (m_TimelineInfos[i]) {
      m_TimelineInfos[i]->signalSemaphoreValueCount = 0;
    }
  }
  m_Signals = std::move(signals);
  m_Fence = &fence;
  m_Submits = submits;
  fence = VK_NULL_HANDLE;
  return nullptr;
}

const char* SubmitSignalsDeferral::Defer(uint32_t submitCount,
                                         VkSubmitInfo2* submits,
                                         VkFence& fence,
                                         SubmitSignals::Function function,
                                         const MayWaitForLaterSignals& mayWaitForLaterSignals,
                                         bool earlierWorkMayWaitForLaterSignals) {
  for (uint32_t i = 0; submits && i < submitCount; ++i) {
    if (submits[i].flags & VK_SUBMIT_PROTECTED_BIT) {
      return "is a protected submission, whose resources cannot be copied to host memory";
    }
    if (!ChainKeepsInSubmission(submits[i].pNext)) {
      return g_UnsupportedChain;
    }
  }
  if (WaitsAfterSignal(
          submitCount, submits,
          [](const VkSubmitInfo2& s) {
            return s.pWaitSemaphoreInfos ? s.waitSemaphoreInfoCount : 0;
          },
          [](const VkSubmitInfo2& s) {
            return s.pSignalSemaphoreInfos ? s.signalSemaphoreInfoCount : 0;
          })) {
    return g_WaitsAfterSignal;
  }
  if (const char* problem =
          StageProblem(submitCount, submits, fence != VK_NULL_HANDLE, mayWaitForLaterSignals,
                       earlierWorkMayWaitForLaterSignals)) {
    return problem;
  }
  SubmitSignals signals;
  signals.SubmittedWith = function;
  signals.Fence = fence;
  for (uint32_t i = 0; submits && i < submitCount; ++i) {
    VkSubmitInfo2& submit = submits[i];
    if (submit.pSignalSemaphoreInfos && submit.signalSemaphoreInfoCount > 0) {
      std::vector<VkSemaphoreSubmitInfo> batch(submit.pSignalSemaphoreInfos,
                                               submit.pSignalSemaphoreInfos +
                                                   submit.signalSemaphoreInfoCount);
      // The signals execute after the copies, so their first synchronization scope has to
      // include them. They wait for every command already.
      for (VkSemaphoreSubmitInfo& signal : batch) {
        signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
      }
      signals.Batches.push_back(std::move(batch));
    }
    m_SignalCounts.push_back(submit.signalSemaphoreInfoCount);
    submit.signalSemaphoreInfoCount = 0;
  }
  m_Signals = std::move(signals);
  m_Fence = &fence;
  m_Submits2 = submits;
  fence = VK_NULL_HANDLE;
  return nullptr;
}

std::optional<SubmitSignals> SubmitSignalsDeferral::Restore() {
  if (!m_Signals) {
    return std::nullopt;
  }
  *m_Fence = m_Signals->Fence;
  for (size_t i = 0; i < m_SignalCounts.size(); ++i) {
    if (m_Submits) {
      m_Submits[i].signalSemaphoreCount = m_SignalCounts[i];
      if (m_TimelineInfos[i]) {
        m_TimelineInfos[i]->signalSemaphoreValueCount = m_TimelineValueCounts[i];
      }
    } else {
      m_Submits2[i].signalSemaphoreInfoCount = m_SignalCounts[i];
    }
  }
  SubmitSignals signals = std::move(*m_Signals);
  m_Signals.reset();
  m_Fence = nullptr;
  m_Submits = nullptr;
  m_Submits2 = nullptr;
  m_SignalCounts.clear();
  m_TimelineInfos.clear();
  m_TimelineValueCounts.clear();
  return signals;
}

} // namespace vulkan
} // namespace gits
