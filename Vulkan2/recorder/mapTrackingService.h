// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "orderingRecorder.h"
#include "command.h"
#include "arguments.h"

#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace gits {
namespace vulkan {

class MapTrackingService {
public:
  MapTrackingService(stream::OrderingRecorder& recorder);
  void StorePhysicalDevice(GITSKey deviceKey, GITSKey physicalDeviceKey);
  void StorePhysicalDeviceMemoryProperties(
      GITSKey physicalDeviceKey, const VkPhysicalDeviceMemoryProperties& memoryProperties);
  bool IsMemoryMappable(GITSKey deviceKey, uint32_t memoryTypeIndex);
  void* EnableExternalMemory(GITSKey deviceKey,
                             VkMemoryAllocateInfo* allocationInfo,
                             std::optional<VkImportMemoryHostPointerInfoEXT>& hostPointerInfo);
  void StoreAllocationInfo(GITSKey deviceKey,
                           GITSKey deviceMemoryKey,
                           VkDeviceMemory memory,
                           const VkMemoryAllocateInfo& allocateInfo,
                           void* externalPtr);
  void FreeExternalMemory(GITSKey deviceMemoryKey);
  void StoreData(GITSKey deviceMemoryKey, VkDeviceSize offset, VkDeviceSize size, void* data);
  void RemoveData(GITSKey deviceMemoryKey);
  void SnapshotAllMapped();

private:
  void ScheduleMemoryUpdate(GITSKey deviceMemoryKey);
  void RecordMappedDataMetaCommand(GITSKey deviceKey,
                                   GITSKey memoryKey,
                                   const std::vector<MemoryRegions::Region>& regions);

  struct AllocationInfo {
    GITSKey DeviceKey;
    VkDeviceMemory Memory;
    VkDeviceSize AllocationSize;
    uint32_t MemoryTypeIndex;
    void* ExternalMemory{nullptr};
    bool IsMapped;
    VkDeviceSize MapOffset;
    VkDeviceSize MapSize;
    void* MappedData{nullptr};
  };

  uint32_t m_PageSize;
  std::unordered_map<GITSKey, GITSKey> m_DeviceToPhysicalDevice;
  std::unordered_map<GITSKey, VkPhysicalDeviceMemoryProperties> m_PhysicalDeviceMemoryProperties;
  std::unordered_map<GITSKey, AllocationInfo> m_Allocations;
  stream::OrderingRecorder& m_Recorder;
  std::mutex m_Mutex;
};

} // namespace vulkan
} // namespace gits
