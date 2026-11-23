// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceUsageTrackingService.h"
#include "arguments.h"
#include "log.h"

namespace gits {
namespace DirectX {
void ResourceUsageTrackingService::AddResource(ObjectKey resourceKey) {
  m_UsageByResource[resourceKey] = {};
}

void ResourceUsageTrackingService::CommandListResourceUsage(ObjectKey commandListKey,
                                                            ObjectKey resourceKey) {
  m_CommandListResourceUsage[commandListKey].push_back(resourceKey);
}

void ResourceUsageTrackingService::CommandListResourceUsage(ObjectKey commandListKey,
                                                            std::vector<ObjectKey>& resourceKeys) {
  m_CommandListResourceUsage[commandListKey].insert(
      m_CommandListResourceUsage[commandListKey].end(), resourceKeys.begin(), resourceKeys.end());
}
void ResourceUsageTrackingService::CommandListReset(ObjectKey commandListKey) {
  m_CommandListResourceUsage[commandListKey].clear();
}

void ResourceUsageTrackingService::ExecuteCommandLists(CommandKey commandKey,
                                                       ObjectKey commandQueueKey,
                                                       std::vector<ObjectKey>& commandListKeys) {
  std::vector<ObjectKey> usedResources;
  for (ObjectKey commandListKey : commandListKeys) {
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

void ResourceUsageTrackingService::DestroyResource(ObjectKey resourceKey) {
  m_UsageByResource.erase(resourceKey);
}

void ResourceUsageTrackingService::CommandQueueWait(CommandKey commandKey,
                                                    ObjectKey commandQueueKey,
                                                    ObjectKey fenceKey,
                                                    UINT64 fenceValue) {
  m_GpuExecutionTracker.CommandQueueWait(commandKey, commandQueueKey, fenceKey, fenceValue);
}

void ResourceUsageTrackingService::CommandQueueSignal(CommandKey commandKey,
                                                      ObjectKey commandQueueKey,
                                                      ObjectKey fenceKey,
                                                      UINT64 fenceValue) {
  m_GpuExecutionTracker.CommandQueueSignal(commandKey, commandQueueKey, fenceKey, fenceValue);
  ProcessReadyExecutables();
}

void ResourceUsageTrackingService::FenceSignal(CommandKey commandKey,
                                               ObjectKey fenceKey,
                                               UINT64 fenceValue) {
  m_GpuExecutionTracker.FenceSignal(commandKey, fenceKey, fenceValue);
  ProcessReadyExecutables();
}

std::vector<ObjectKey> ResourceUsageTrackingService::GetOrderedResources() {
  std::map<UsageNumber, std::vector<ObjectKey>> resourceByCommandKey;
  for (const auto& [resourceKey, usageNumber] : m_UsageByResource) {
    resourceByCommandKey[usageNumber].push_back(resourceKey);
  }

  std::vector<ObjectKey> orderedResources;
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

void ResourceUsageTrackingService::UpdateUsage(const std::vector<ObjectKey>& usedResources) {
  ++m_ExecuteNumber;

  unsigned commandNumber{};
  for (ObjectKey resourceKey : usedResources) {
    m_UsageByResource[resourceKey] = {m_ExecuteNumber, ++commandNumber};
  }
}

} // namespace DirectX
} // namespace gits
