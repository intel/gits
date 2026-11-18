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
#include <set>

namespace gits {
namespace vulkan {

class CaptureManager;

class MapTrackingService {
public:
  MapTrackingService(CaptureManager& manager, stream::OrderingRecorder& recorder);
  ~MapTrackingService();
  void StoreInstanceApiVersion(GITSKey instanceKey, uint32_t apiVersion);
  void StorePhysicalDeviceInstance(GITSKey physicalDeviceKey, GITSKey instanceKey);
  // Returns minImportedHostPointerAlignment when VK_EXT_external_memory_host can be enabled on a
  // device with the given extensions.
  std::optional<VkDeviceSize> QueryHostPointerImport(
      HandleArgument<VkPhysicalDevice>& physicalDevice,
      const std::vector<const char*>& enabledExtensions);
  void StoreHostPointerImport(GITSKey deviceKey, VkDeviceSize alignment);
  void StoreDevice(GITSKey deviceKey, GITSKey physicalDeviceKey, uint32_t deviceGroupSize);
  void StorePhysicalDeviceMemoryProperties(
      GITSKey physicalDeviceKey, const VkPhysicalDeviceMemoryProperties& memoryProperties);
  bool IsMemoryMappable(GITSKey deviceKey, uint32_t memoryTypeIndex);
  // Returns nullptr when the allocation can't import host memory; its mappings are then tracked
  // through a shadow copy.
  void* EnableExternalMemory(GITSKey deviceKey,
                             VkDevice device,
                             VkMemoryAllocateInfo* allocationInfo,
                             std::optional<VkImportMemoryHostPointerInfoEXT>& hostPointerInfo);
  void UnlinkExternalMemory(VkMemoryAllocateInfo* allocationInfo,
                            std::optional<VkImportMemoryHostPointerInfoEXT>& hostPointerInfo);
  void StoreAllocationInfo(GITSKey deviceKey,
                           GITSKey deviceMemoryKey,
                           VkDeviceMemory memory,
                           const VkMemoryAllocateInfo& allocateInfo,
                           void* externalPtr);
  // Zeroes an allocation not backed by external memory. Only the mapped pages the application
  // writes are recorded, and the player clears host-visible memory at allocation, so pages read
  // before being written must be zero in the recording as well.
  void ClearMemory(VkDevice device,
                   GITSKey deviceMemoryKey,
                   const VkMemoryAllocateInfo& allocateInfo);
  void RemoveAllocationInfo(GITSKey deviceMemoryKey);
  void StoreData(GITSKey deviceMemoryKey, VkDeviceSize offset, VkDeviceSize size, void** ppData);
  void RemoveData(GITSKey deviceMemoryKey);
  void SnapshotAllMapped();
  void SnapshotMapped(GITSKey deviceMemoryKey);
  // Make device writes visible to the host in shadowed mappings: those of the range on
  // vkInvalidateMappedMemoryRanges and of every host-coherent mapping on a host synchronization.
  // Shadow pages the application has not written are refreshed on their next access.
  void RefreshMapped(GITSKey deviceMemoryKey, VkDeviceSize offset, VkDeviceSize size);
  void RefreshCoherentMapped();

#ifdef GITS_PLATFORM_LINUX
  bool HandleAddress(void* address, bool isWrite);
#else
  // Refreshes a protected shadow page on its first access. Returns false for other addresses.
  bool HandleAddress(void* address);
#endif

private:
  void ScheduleMemoryUpdate(GITSKey deviceMemoryKey);
  void RecordMappedDataMetaCommand(GITSKey deviceKey,
                                   GITSKey memoryKey,
                                   const std::vector<MemoryRegions::Region>& regions);

  // On Windows, writes are tracked by the write watch and Read pages are PAGE_READWRITE.
  enum class PageState : uint8_t {
    Protected, // PROT_NONE or PAGE_NOACCESS, shadow content may be stale
    Read,      // PROT_READ, refreshed from the mapped memory
    Written    // PROT_READ | PROT_WRITE, must be flushed to the mapped memory
  };

  struct AllocationInfo {
    GITSKey DeviceKey;
    VkDeviceMemory Memory;
    VkDeviceSize AllocationSize;
    uint32_t MemoryTypeIndex;
    void* ExternalMemory{nullptr};
    bool IsCoherent{true};
    bool IsMapped;
    VkDeviceSize MapOffset;
    VkDeviceSize MapSize;
    void* MappedData{nullptr};
    void* ShadowMemory{nullptr};
#ifdef GITS_PLATFORM_WINDOWS
    // The VirtualAlloc base of ShadowMemory, which is offset so that it keeps the residue of
    // MappedData modulo the allocation granularity, and with it minMemoryMapAlignment.
    void* ShadowAllocation{nullptr};
    // Pages the application wrote while they were refreshed. Their write watch was reset, but
    // they must still be copied to the mapped memory.
    std::set<void*> RefreshedWrittenPages;
#endif
    // Sized when the memory is mapped, so the fault handler never allocates.
    std::vector<PageState> PageStates;
    // Pages [AccessedBegin, AccessedEnd) bound every page accessed since the last update.
    size_t AccessedBegin{0};
    size_t AccessedEnd{0};
  };

  void ReleaseShadowMemory(AllocationInfo& allocationInfo);
#ifdef GITS_PLATFORM_WINDOWS
  char* ShadowPageBase(const AllocationInfo& allocationInfo) const;
  void InvalidateShadowMemory(AllocationInfo& allocationInfo, VkDeviceSize begin, VkDeviceSize end);
  void RefreshShadowPage(AllocationInfo& allocationInfo, size_t pageIndex);
  uint32_t m_AllocationGranularity;
  // Page-sized buffers used by RefreshShadowPage under m_Mutex.
  std::vector<char> m_PageSnapshot;
  std::vector<char> m_PageDeviceData;
#endif

  uint32_t m_PageSize;
  std::unordered_map<GITSKey, uint32_t> m_InstanceApiVersions;
  std::unordered_map<GITSKey, GITSKey> m_PhysicalDeviceToInstance;
  // minImportedHostPointerAlignment of devices with VK_EXT_external_memory_host enabled
  std::unordered_map<GITSKey, VkDeviceSize> m_HostPointerImportAlignments;
  struct DeviceInfo {
    GITSKey PhysicalDeviceKey{};
    uint32_t DeviceGroupSize{1};
  };
  std::unordered_map<GITSKey, DeviceInfo> m_Devices;
  std::unordered_map<GITSKey, VkPhysicalDeviceMemoryProperties> m_PhysicalDeviceMemoryProperties;
  std::unordered_map<GITSKey, AllocationInfo> m_Allocations;
  // Mapped allocations by shadow memory address. Points into m_Allocations, whose elements stay
  // valid until erased.
  std::map<uintptr_t, AllocationInfo*> m_ShadowAllocations;
#ifdef GITS_PLATFORM_LINUX
  bool m_SigsegvHandlerReplacedLogged{false};
#endif
  CaptureManager& m_Manager;
  stream::OrderingRecorder& m_Recorder;
  std::mutex m_Mutex;
};

} // namespace vulkan
} // namespace gits
