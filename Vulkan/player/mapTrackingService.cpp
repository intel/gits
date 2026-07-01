// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingService.h"

namespace gits {
namespace vulkan {

void MapTrackingService::StoreAllocationInfo(GITSKey deviceKey,
                                             GITSKey deviceMemoryKey,
                                             VkDeviceSize allocationSize,
                                             uint32_t memoryTypeIndex) {
  AllocationInfo& info = m_Allocations[deviceKey][deviceMemoryKey];
  info.AllocationSize = allocationSize;
  info.MemoryTypeIndex = memoryTypeIndex;
}

void MapTrackingService::StoreData(GITSKey deviceKey,
                                   GITSKey deviceMemoryKey,
                                   void* data,
                                   uint64_t size) {
  MappedEntry& entry = m_Entries[deviceKey][deviceMemoryKey];
  entry.Ptr = data;
  entry.Size =
      (size != VK_WHOLE_SIZE) ? size : m_Allocations[deviceKey][deviceMemoryKey].AllocationSize;
}

MapTrackingService::MappedEntry* MapTrackingService::GetData(GITSKey deviceKey,
                                                             GITSKey deviceMemoryKey) {
  auto it = m_Entries.find(deviceKey);
  if (it == m_Entries.end()) {
    return nullptr;
  }
  auto memIt = it->second.find(deviceMemoryKey);
  if (memIt == it->second.end()) {
    return nullptr;
  }
  return &memIt->second;
}

void MapTrackingService::RemoveData(GITSKey deviceKey, GITSKey deviceMemoryKey) {
  auto it = m_Entries.find(deviceKey);
  if (it == m_Entries.end()) {
    return;
  }
  it->second.erase(deviceMemoryKey);
  if (it->second.empty()) {
    m_Entries.erase(it);
  }
}

} // namespace vulkan
} // namespace gits
