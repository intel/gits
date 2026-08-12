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
  void AddResource(GITSKey resourceKey);
  void CommandListResourceUsage(GITSKey commandListKey, GITSKey resourceKey);
  void CommandListResourceUsage(GITSKey commandListKey, std::vector<GITSKey>& resourceKeys);
  void CommandListReset(GITSKey commandListKey);
  void ExecuteCommandLists(GITSKey commandKey,
                           GITSKey commandQueueKey,
                           std::vector<GITSKey>& commandListKeys);
  void DestroyResource(GITSKey resourceKey);

  void CommandQueueWait(GITSKey commandKey,
                        GITSKey commandQueueKey,
                        GITSKey fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(GITSKey commandKey,
                          GITSKey commandQueueKey,
                          GITSKey fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(GITSKey commandKey, GITSKey fenceKey, UINT64 fenceValue);

  std::vector<unsigned> GetOrderedResources();

private:
  struct UsageNumber {
    GITSKey ExecuteKey{};
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
  std::map<GITSKey, UsageNumber> m_UsageByResource;
  std::unordered_map<GITSKey, std::vector<GITSKey>> m_CommandListResourceUsage;
};

} // namespace DirectX
} // namespace gits
