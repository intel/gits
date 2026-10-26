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

#include <map>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace gits {
namespace vulkan {

class CaptureManager;

class MapTrackingService {
public:
  MapTrackingService(CaptureManager& manager, stream::OrderingRecorder& recorder);
  ~MapTrackingService();
  void StorePhysicalDevice(GITSKey deviceKey, GITSKey physicalDeviceKey);
  void StorePhysicalDeviceMemoryProperties(
      GITSKey physicalDeviceKey, const VkPhysicalDeviceMemoryProperties& memoryProperties);
  bool IsMemoryMappable(GITSKey deviceKey, uint32_t memoryTypeIndex);
  void* EnableExternalMemory(GITSKey deviceKey,
                             VkMemoryAllocateInfo* allocationInfo,
                             std::optional<VkImportMemoryHostPointerInfoEXT>& hostPointerInfo);
  void UnlinkExternalMemory(VkMemoryAllocateInfo* allocationInfo,
                            std::optional<VkImportMemoryHostPointerInfoEXT>& hostPointerInfo);
  void StoreAllocationInfo(GITSKey deviceKey,
                           GITSKey deviceMemoryKey,
                           VkDeviceMemory memory,
                           const VkMemoryAllocateInfo& allocateInfo,
                           void* externalPtr);
  void RemoveAllocationInfo(GITSKey deviceMemoryKey);
  void StoreData(GITSKey deviceMemoryKey, VkDeviceSize offset, VkDeviceSize size, void** ppData);
  void RemoveData(GITSKey deviceMemoryKey);
  void SnapshotAllMapped();

#ifdef GITS_PLATFORM_LINUX
  bool HandleAddress(void* address, bool isWrite);
#endif

private:
  void ScheduleMemoryUpdate(GITSKey deviceMemoryKey);
  void RecordMappedDataMetaCommand(GITSKey deviceKey,
                                   GITSKey memoryKey,
                                   const std::vector<MemoryRegions::Region>& regions);

  enum class PageState : uint8_t {
    Protected, // PROT_NONE, shadow content may be stale
    Read,      // PROT_READ, refreshed from the mapped memory
    Written    // PROT_READ | PROT_WRITE, must be flushed to the mapped memory
  };

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
    void* ShadowMemory{nullptr};
    // Sized when the memory is mapped, so the SIGSEGV handler never allocates.
    std::vector<PageState> PageStates;
    // Pages [AccessedBegin, AccessedEnd) bound every page accessed since the last update.
    size_t AccessedBegin{0};
    size_t AccessedEnd{0};
  };

#ifdef GITS_PLATFORM_LINUX
  void ReleaseShadowMemory(AllocationInfo& allocationInfo);
#endif

  uint32_t m_PageSize;
  std::unordered_map<GITSKey, GITSKey> m_DeviceToPhysicalDevice;
  std::unordered_map<GITSKey, VkPhysicalDeviceMemoryProperties> m_PhysicalDeviceMemoryProperties;
  std::unordered_map<GITSKey, AllocationInfo> m_Allocations;
#ifdef GITS_PLATFORM_LINUX
  // Mapped allocations by shadow memory address. Points into m_Allocations, whose elements stay
  // valid until erased.
  std::map<uintptr_t, AllocationInfo*> m_ShadowAllocations;
  bool m_SigsegvHandlerReplacedLogged{false};
#endif
  CaptureManager& m_Manager;
  stream::OrderingRecorder& m_Recorder;
  std::mutex m_Mutex;
};

} // namespace vulkan
} // namespace gits
