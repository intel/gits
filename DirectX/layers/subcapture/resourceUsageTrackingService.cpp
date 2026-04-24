// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceUsageTrackingService.h"
#include "log.h"

namespace gits {
namespace DirectX {
void ResourceUsageTrackingService::AddResource(unsigned resourceKey) {
  m_UsageByResource[resourceKey] = {};
}

void ResourceUsageTrackingService::CommandListResourceUsage(unsigned commandListKey,
                                                            unsigned resourceKey) {
  m_CommandListResourceUsage[commandListKey].push_back(resourceKey);
}

void ResourceUsageTrackingService::CommandListResourceUsage(unsigned commandListKey,
                                                            std::vector<unsigned>& resourceKeys) {
  m_CommandListResourceUsage[commandListKey].insert(
      m_CommandListResourceUsage[commandListKey].end(), resourceKeys.begin(), resourceKeys.end());
}
void ResourceUsageTrackingService::CommandListReset(unsigned commandListKey) {
  m_CommandListResourceUsage[commandListKey].clear();
}

void ResourceUsageTrackingService::ExecuteCommandLists(unsigned commandKey,
                                                       unsigned commandQueueKey,
                                                       std::vector<unsigned>& commandListKeys) {
  std::vector<unsigned> usedResources;
  for (unsigned commandListKey : commandListKeys) {
    auto it = m_CommandListResourceUsage.find(commandListKey);
    if (it == m_CommandListResourceUsage.end()) {
      continue;
    }

    usedResources.insert(usedResources.end(), it->second.begin(), it->second.end());
  }

  const bool isWaiting = m_GpuExecutionTracker.IsCommandQueueWaiting(commandQueueKey);
  if (isWaiting) {
    ResourceUsage* executable = new ResourceUsage{};
    executable->UsedResources = std::move(usedResources);
    m_GpuExecutionTracker.Execute(commandKey, commandQueueKey, executable);
  } else {
    UpdateUsage(usedResources);
  }
}

void ResourceUsageTrackingService::DestroyResource(unsigned resourceKey) {
  m_UsageByResource.erase(resourceKey);
}

void ResourceUsageTrackingService::CommandQueueWait(unsigned commandKey,
                                                    unsigned commandQueueKey,
                                                    unsigned fenceKey,
                                                    UINT64 fenceValue) {
  m_GpuExecutionTracker.CommandQueueWait(commandKey, commandQueueKey, fenceKey, fenceValue);
}

void ResourceUsageTrackingService::CommandQueueSignal(unsigned commandKey,
                                                      unsigned commandQueueKey,
                                                      unsigned fenceKey,
                                                      UINT64 fenceValue) {
  m_GpuExecutionTracker.CommandQueueSignal(commandKey, commandQueueKey, fenceKey, fenceValue);
  ProcessReadyExecutables();
}

void ResourceUsageTrackingService::FenceSignal(unsigned commandKey,
                                               unsigned fenceKey,
                                               UINT64 fenceValue) {
  m_GpuExecutionTracker.FenceSignal(commandKey, fenceKey, fenceValue);
  ProcessReadyExecutables();
}

std::vector<unsigned> ResourceUsageTrackingService::GetOrderedResources() {
  std::map<UsageNumber, std::vector<unsigned>> resourceByCommandKey;
  for (const auto& [resourceKey, usageNumber] : m_UsageByResource) {
    resourceByCommandKey[usageNumber].push_back(resourceKey);
  }

  std::vector<unsigned> orderedResources;
  for (const auto& [usageNumber, keys] : resourceByCommandKey) {
    orderedResources.insert(orderedResources.end(), keys.begin(), keys.end());
  }
  return orderedResources;
}

void ResourceUsageTrackingService::ProcessReadyExecutables() {
  std::vector<GpuExecutionTracker::Executable*>& executables =
      m_GpuExecutionTracker.GetReadyExecutables();
  for (GpuExecutionTracker::Executable* executable : executables) {
    ResourceUsage* resourceUsage = static_cast<ResourceUsage*>(executable);
    UpdateUsage(resourceUsage->UsedResources);
    delete resourceUsage;
  }
  executables.clear();
}

void ResourceUsageTrackingService::UpdateUsage(const std::vector<unsigned>& usedResources) {
  ++m_ExecuteNumber;

  unsigned commandNumber{};
  for (unsigned resourceKey : usedResources) {
    m_UsageByResource[resourceKey] = {m_ExecuteNumber, ++commandNumber};
  }
}

} // namespace DirectX
} // namespace gits
