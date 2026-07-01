// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <vector>
#include <unordered_map>

namespace gits {
namespace vulkan {

class MapTrackingService {
public:
  typedef uint64_t GITSKey;

  struct MappedEntry {
    void* Ptr{};
    uint64_t Size{};
  };

  void StoreAllocationInfo(GITSKey deviceKey,
                           GITSKey deviceMemoryKey,
                           VkDeviceSize allocationSize,
                           uint32_t memoryTypeIndex);
  void StoreData(GITSKey deviceKey, GITSKey deviceMemoryKey, void* data, uint64_t size);
  MappedEntry* GetData(GITSKey deviceKey, GITSKey deviceMemoryKey);
  void RemoveData(GITSKey deviceKey, GITSKey deviceMemoryKey);

private:
  struct AllocationInfo {
    VkDeviceSize AllocationSize;
    uint32_t MemoryTypeIndex;
  };

  std::unordered_map<GITSKey, std::unordered_map<GITSKey, AllocationInfo>> m_Allocations;
  std::unordered_map<GITSKey, std::unordered_map<GITSKey, MappedEntry>> m_Entries;
};

} // namespace vulkan
} // namespace gits
