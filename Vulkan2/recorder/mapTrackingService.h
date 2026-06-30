// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gitsRecorder.h"
#include "command.h"
#include "arguments.h"

#include <vector>
#include <unordered_map>

namespace gits {
namespace vulkan {

class MapTrackingService {
public:
  MapTrackingService(GitsRecorder& gitsRecorder);

  void StoreAllocationInfo(GITSKey deviceKey,
                           GITSKey deviceMemoryKey,
                           VkDeviceSize allocationSize,
                           uint32_t memoryTypeIndex);
  void StoreData(GITSKey deviceKey, GITSKey deviceMemoryKey, void* data, VkDeviceSize size);
  void RemoveData(GITSKey deviceKey, GITSKey deviceMemoryKey);
  void SnapshotAllMapped();

private:
  void ScheduleMemoryUpdate(GITSKey deviceKey, GITSKey deviceMemoryKey);

  struct AllocationInfo {
    VkDeviceSize AllocationSize;
    uint32_t MemoryTypeIndex;
  };

  struct MappedEntry {
    void* Ptr{};
    VkDeviceSize Size{};
    std::vector<char> Data{};
  };

  std::unordered_map<GITSKey, std::unordered_map<GITSKey, AllocationInfo>> m_Allocations;
  std::unordered_map<GITSKey, std::unordered_map<GITSKey, MappedEntry>> m_Entries;
  GitsRecorder& m_GitsRecorder;
};

} // namespace vulkan
} // namespace gits
