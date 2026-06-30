// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingService.h"
#include "captureManager.h"
#include "commandWritersCustom.h"

namespace gits {
namespace vulkan {

MapTrackingService::MapTrackingService(GitsRecorder& gitsRecorder) : m_GitsRecorder(gitsRecorder) {}

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
                                   VkDeviceSize size) {
  MappedEntry& entry = m_Entries[deviceKey][deviceMemoryKey];
  entry.Ptr = data;
  entry.Size =
      (size != VK_WHOLE_SIZE) ? size : m_Allocations[deviceKey][deviceMemoryKey].AllocationSize;
  entry.Data.resize(entry.Size);
  std::memcpy(entry.Data.data(), data, entry.Size);
}

void MapTrackingService::RemoveData(GITSKey deviceKey, GITSKey deviceMemoryKey) {
  ScheduleMemoryUpdate(deviceKey, deviceMemoryKey);
  m_Entries[deviceKey].erase(deviceMemoryKey);
}

void MapTrackingService::SnapshotAllMapped() {
  for (const auto& [deviceKey, memoryMap] : m_Entries) {
    for (const auto& [memoryKey, entry] : memoryMap) {
      ScheduleMemoryUpdate(deviceKey, memoryKey);
    }
  }
}

void MapTrackingService::ScheduleMemoryUpdate(GITSKey deviceKey, GITSKey deviceMemoryKey) {
  MappedEntry& entry = m_Entries[deviceKey][deviceMemoryKey];
  const char* current = static_cast<const char*>(entry.Ptr);
  const VkDeviceSize totalSize = entry.Size;

  MappedDataMetaCommand command(GITS_GET_THREAD_ID());
  command.m_Key = CaptureManager::Get().CreateCommandKey();
  command.m_Device.Key = deviceKey;
  command.m_Memory.Key = deviceMemoryKey;
  command.m_TotalSize.Value = totalSize;
  command.m_RegionCount.Value = 1;
  command.m_Regions.push_back({0, totalSize, current});

  m_GitsRecorder.record(command.m_Key, new MappedDataMetaWriter(command));
}

} // namespace vulkan
} // namespace gits
