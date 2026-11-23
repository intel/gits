// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "gpuExecutionTracker.h"

#include <vector>
#include <map>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ResourceUsageTrackingService {
public:
  void AddResource(ObjectKey resourceKey);
  void CommandListResourceUsage(ObjectKey commandListKey, ObjectKey resourceKey);
  void CommandListResourceUsage(ObjectKey commandListKey, std::vector<ObjectKey>& resourceKeys);
  void CommandListReset(ObjectKey commandListKey);
  void ExecuteCommandLists(CommandKey commandKey,
                           ObjectKey commandQueueKey,
                           std::vector<ObjectKey>& commandListKeys);
  void DestroyResource(ObjectKey resourceKey);

  void CommandQueueWait(CommandKey commandKey,
                        ObjectKey commandQueueKey,
                        ObjectKey fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(CommandKey commandKey,
                          ObjectKey commandQueueKey,
                          ObjectKey fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(CommandKey commandKey, ObjectKey fenceKey, UINT64 fenceValue);

  std::vector<ObjectKey> GetOrderedResources();

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
    std::vector<ObjectKey> UsedResources;
  };

  void ProcessReadyExecutables();
  void UpdateUsage(const std::vector<ObjectKey>& usedResources);

  unsigned m_ExecuteNumber{};
  GpuExecutionTracker m_GpuExecutionTracker;
  std::map<ObjectKey, UsageNumber> m_UsageByResource;
  std::unordered_map<ObjectKey, std::vector<ObjectKey>> m_CommandListResourceUsage;
};

} // namespace DirectX
} // namespace gits
