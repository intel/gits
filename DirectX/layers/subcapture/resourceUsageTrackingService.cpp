// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gits.h"
#include "resourceUsageTrackingService.h"

namespace gits {
namespace DirectX {
void ResourceUsageTrackingService::addResource(unsigned resourceKey) {
  usageByResource_[resourceKey] = {};
}

void ResourceUsageTrackingService::commandListResourceUsage(unsigned commandListKey,
                                                            unsigned resourceKeys) {
  commandListResourceUsage_[commandListKey].push_back(resourceKeys);
}

void ResourceUsageTrackingService::commandListResourceUsage(unsigned commandListKey,
                                                            std::vector<unsigned>& resourceKeys) {
  commandListResourceUsage_[commandListKey].insert(commandListResourceUsage_[commandListKey].end(),
                                                   resourceKeys.begin(), resourceKeys.end());
}
void ResourceUsageTrackingService::commandListReset(unsigned commandListKey) {
  commandListResourceUsage_[commandListKey].clear();
}

void ResourceUsageTrackingService::executeCommandLists(unsigned commandKey,
                                                       unsigned commandQueueKey,
                                                       std::vector<unsigned>& commandListKeys) {
  std::vector<unsigned> usedResources;
  for (unsigned commandListKey : commandListKeys) {
    auto it = commandListResourceUsage_.find(commandListKey);
    if (it == commandListResourceUsage_.end()) {
      continue;
    }

    usedResources.insert(usedResources.end(), it->second.begin(), it->second.end());
  }

  const bool isWaiting = gpuExecutionTracker_.isCommandQueueWaiting(commandQueueKey);
  if (isWaiting) {
    auto* executable = new ResourceUsage{};
    executable->usedResources = usedResources;
    gpuExecutionTracker_.execute(commandKey, commandQueueKey, executable);
  } else {
    updateUsage(usedResources);
  }
}

void ResourceUsageTrackingService::destroyResource(unsigned resourceKey) {
  usageByResource_.erase(resourceKey);
}

void ResourceUsageTrackingService::commandQueueWait(unsigned callKey,
                                                    unsigned commandQueueKey,
                                                    unsigned fenceKey,
                                                    UINT64 fenceValue) {
  gpuExecutionTracker_.commandQueueWait(callKey, commandQueueKey, fenceKey, fenceValue);
}

void ResourceUsageTrackingService::commandQueueSignal(unsigned callKey,
                                                      unsigned commandQueueKey,
                                                      unsigned fenceKey,
                                                      UINT64 fenceValue) {
  gpuExecutionTracker_.commandQueueSignal(callKey, commandQueueKey, fenceKey, fenceValue);
  processReadyExecutables();
}

void ResourceUsageTrackingService::fenceSignal(unsigned callKey,
                                               unsigned fenceKey,
                                               UINT64 fenceValue) {
  gpuExecutionTracker_.fenceSignal(callKey, fenceKey, fenceValue);
  processReadyExecutables();
}

std::vector<unsigned> ResourceUsageTrackingService::getOrderedResources() {
  std::map<UsageNumber, std::vector<unsigned>> resourceByCommandKey;
  for (const auto& [resourceKey, usageNumber] : usageByResource_) {
    resourceByCommandKey[usageNumber].push_back(resourceKey);
  }

  std::vector<unsigned> orderedResources;
  for (const auto& [usageNumber, resourceKeys] : resourceByCommandKey) {
    orderedResources.insert(orderedResources.end(), resourceKeys.begin(), resourceKeys.end());
  }
  return orderedResources;
}

void ResourceUsageTrackingService::processReadyExecutables() {
  std::vector<GpuExecutionTracker::Executable*>& executables =
      gpuExecutionTracker_.getReadyExecutables();
  for (GpuExecutionTracker::Executable* executable : executables) {
    ResourceUsage* resourceUsage = static_cast<ResourceUsage*>(executable);
    updateUsage(resourceUsage->usedResources);
    delete resourceUsage;
  }
  executables.clear();
}

void ResourceUsageTrackingService::updateUsage(const std::vector<unsigned>& usedResources) {
  executeNumber_++;

  unsigned commandNumber{};
  for (unsigned resourceKey : usedResources) {
    usageByResource_[resourceKey] = {executeNumber_, commandNumber++};
  }
}

} // namespace DirectX
} // namespace gits
