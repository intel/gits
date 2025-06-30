// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gpuExecutionTracker.h"

#include <vector>
#include <map>

namespace gits {
namespace DirectX {

class ResourceUsageTrackingService {
public:
  void addResource(unsigned resourceKey);
  void commandListResourceUsage(unsigned commandListKey, unsigned resourceKey);
  void commandListResourceUsage(unsigned commandListKey, std::vector<unsigned>& resourceKeys);
  void commandListReset(unsigned commandListKey);
  void executeCommandLists(unsigned commandKey,
                           unsigned commandQueueKey,
                           std::vector<unsigned>& commandListKeys);
  void destroyResource(unsigned resourceKey);

  void commandQueueWait(unsigned callKey,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void commandQueueSignal(unsigned callKey,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void fenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue);

  std::vector<unsigned> getOrderedResources();

private:
  struct UsageNumber {
    unsigned executeKey{};
    unsigned commandNumber{};

    bool operator<(const UsageNumber& rhs) const {
      if (executeKey == rhs.executeKey) {
        return commandNumber < rhs.commandNumber;
      } else {
        return executeKey < rhs.executeKey;
      }
    }
  };
  struct ResourceUsage : public GpuExecutionTracker::Executable {
    std::vector<unsigned> usedResources;
  };

  void processReadyExecutables();
  void updateUsage(const std::vector<unsigned>& usedResources);

  unsigned executeNumber_;
  GpuExecutionTracker gpuExecutionTracker_;
  std::map<unsigned, UsageNumber> usageByResource_;
  std::unordered_map<unsigned, std::vector<unsigned>> commandListResourceUsage_;
};

} // namespace DirectX
} // namespace gits
