// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourcesDump.h"
#include "vulkanHeader2.h"

#include <functional>
#include <optional>
#include <vector>

namespace gits {
namespace vulkan {

// Whether waiting for the value of the semaphore may depend on signal operations submitted after
// the wait.
using MayWaitForLaterSignals = std::function<bool(VkSemaphore semaphore, uint64_t value)>;

// Takes the fence and semaphore signal operations out of a queue submission before it is
// replayed, and puts them back once it is.
class SubmitSignalsDeferral {
public:
  // Returns why the signals cannot be taken out, leaving the submission as it is, or nullptr once
  // they are. earlierWorkMayWaitForLaterSignals tells whether the work submitted to the queue
  // before may wait for signal operations submitted after the submission.
  const char* Defer(uint32_t submitCount,
                    VkSubmitInfo* submits,
                    VkFence& fence,
                    const MayWaitForLaterSignals& mayWaitForLaterSignals,
                    bool earlierWorkMayWaitForLaterSignals);
  const char* Defer(uint32_t submitCount,
                    VkSubmitInfo2* submits,
                    VkFence& fence,
                    SubmitSignals::Function function,
                    const MayWaitForLaterSignals& mayWaitForLaterSignals,
                    bool earlierWorkMayWaitForLaterSignals);
  // Puts the signals back into the submission and returns them, if they were taken out.
  std::optional<SubmitSignals> Restore();

private:
  std::optional<SubmitSignals> m_Signals;
  VkFence* m_Fence{};
  VkSubmitInfo* m_Submits{};
  VkSubmitInfo2* m_Submits2{};
  std::vector<uint32_t> m_SignalCounts;
  std::vector<VkTimelineSemaphoreSubmitInfo*> m_TimelineInfos;
  std::vector<uint32_t> m_TimelineValueCounts;
};

} // namespace vulkan
} // namespace gits
