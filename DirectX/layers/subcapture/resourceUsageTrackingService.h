// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gpuExecutionTracker.h"

#include <vector>
#include <map>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ResourceUsageTrackingService {
public:
  void AddResource(unsigned resourceKey);
  void CommandListResourceUsage(unsigned commandListKey, unsigned resourceKey);
  void CommandListResourceUsage(unsigned commandListKey, std::vector<unsigned>& resourceKeys);
  void CommandListReset(unsigned commandListKey);
  void ExecuteCommandLists(unsigned commandKey,
                           unsigned commandQueueKey,
                           std::vector<unsigned>& commandListKeys);
  void DestroyResource(unsigned resourceKey);

  void CommandQueueWait(unsigned commandKey,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(unsigned commandKey,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(unsigned commandKey, unsigned fenceKey, UINT64 fenceValue);

  std::vector<unsigned> GetOrderedResources();

private:
  struct UsageNumber {
    unsigned ExecuteKey{};
    unsigned CommandNumber{};

    bool operator<(const UsageNumber& rhs) const {
      if (ExecuteKey == rhs.ExecuteKey) {
        return CommandNumber < rhs.CommandNumber;
      } else {
        return ExecuteKey < rhs.ExecuteKey;
      }
    }
  };
  struct ResourceUsage : public GpuExecutionTracker::Executable {
    std::vector<unsigned> UsedResources;
  };

  void ProcessReadyExecutables();
  void UpdateUsage(const std::vector<unsigned>& usedResources);

  unsigned m_ExecuteNumber{};
  GpuExecutionTracker m_GpuExecutionTracker;
  std::map<unsigned, UsageNumber> m_UsageByResource;
  std::unordered_map<unsigned, std::vector<unsigned>> m_CommandListResourceUsage;
};

} // namespace DirectX
} // namespace gits
