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

MapTrackingService::MapTrackingService(GitsRecorder& gitsRecorder) : m_GitsRecorder(gitsRecorder) {
#ifdef GITS_PLATFORM_WINDOWS
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  m_PageSize = si.dwPageSize;
#endif
}

void MapTrackingService::StorePhysicalDevice(GITSKey deviceKey, GITSKey physicalDeviceKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_DeviceToPhysicalDevice[deviceKey] = physicalDeviceKey;
}

void MapTrackingService::StorePhysicalDeviceMemoryProperties(
    GITSKey physicalDeviceKey, const VkPhysicalDeviceMemoryProperties& memoryProperties) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_PhysicalDeviceMemoryProperties[physicalDeviceKey] = memoryProperties;
}

bool MapTrackingService::IsMemoryMappable(GITSKey deviceKey, uint32_t memoryTypeIndex) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto deviceIt = m_DeviceToPhysicalDevice.find(deviceKey);
  GITS_ASSERT(deviceIt != m_DeviceToPhysicalDevice.end());
  GITSKey physicalDeviceKey = deviceIt->second;
  auto it = m_PhysicalDeviceMemoryProperties.find(physicalDeviceKey);
  GITS_ASSERT(it != m_PhysicalDeviceMemoryProperties.end());
  VkPhysicalDeviceMemoryProperties& memoryProperties = it->second;
  return (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags &
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
}

void* MapTrackingService::EnableExternalMemory(
    GITSKey deviceKey,
    VkMemoryAllocateInfo* allocationInfo,
    std::optional<VkImportMemoryHostPointerInfoEXT>& hostPointerInfo) {
#ifdef GITS_PLATFORM_WINDOWS
  void* hostPtr = VirtualAlloc(nullptr, static_cast<SIZE_T>(allocationInfo->allocationSize),
                               MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE);
  GITS_ASSERT(hostPtr != nullptr);
  hostPointerInfo = VkImportMemoryHostPointerInfoEXT{};
  hostPointerInfo->sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT;
  hostPointerInfo->pNext = allocationInfo->pNext;
  hostPointerInfo->handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
  hostPointerInfo->pHostPointer = hostPtr;
  allocationInfo->pNext = &hostPointerInfo.value();
  return hostPtr;
#else
  return nullptr;
#endif
}

void MapTrackingService::StoreAllocationInfo(GITSKey deviceKey,
                                             GITSKey deviceMemoryKey,
                                             VkDeviceMemory memory,
                                             const VkMemoryAllocateInfo& allocateInfo,
                                             void* externalPtr) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  AllocationInfo allocationInfo{};
  allocationInfo.DeviceKey = deviceKey;
  allocationInfo.Memory = memory;
  allocationInfo.AllocationSize = allocateInfo.allocationSize;
  allocationInfo.MemoryTypeIndex = allocateInfo.memoryTypeIndex;
  allocationInfo.ExternalMemory = externalPtr;
  m_Allocations[deviceMemoryKey] = allocationInfo;
}

void MapTrackingService::FreeExternalMemory(GITSKey deviceMemoryKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
#ifdef GITS_PLATFORM_WINDOWS
  auto it = m_Allocations.find(deviceMemoryKey);
  if (it == m_Allocations.end()) {
    return;
  }
  if (it->second.ExternalMemory) {
    VirtualFree(it->second.ExternalMemory, 0, MEM_RELEASE);
  }
  m_Allocations.erase(deviceMemoryKey);
#endif
}

void MapTrackingService::StoreData(GITSKey deviceMemoryKey,
                                   VkDeviceSize offset,
                                   VkDeviceSize size,
                                   void* data) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Allocations.find(deviceMemoryKey);
  GITS_ASSERT(it != m_Allocations.end());
  auto& allocationInfo = it->second;
  allocationInfo.IsMapped = true;
  allocationInfo.MapOffset = offset;
  allocationInfo.MapSize = (size == VK_WHOLE_SIZE) ? allocationInfo.AllocationSize - offset : size;
  allocationInfo.MappedData = data;
#ifdef GITS_PLATFORM_WINDOWS
  ResetWriteWatch(data, static_cast<SIZE_T>(allocationInfo.MapSize));
#endif
}

void MapTrackingService::RemoveData(GITSKey deviceMemoryKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Allocations.find(deviceMemoryKey);
  GITS_ASSERT(it != m_Allocations.end());
  if (it->second.IsMapped) {
    ScheduleMemoryUpdate(deviceMemoryKey);
  }
  it->second.IsMapped = false;
  it->second.MapOffset = 0;
  it->second.MapSize = 0;
  it->second.MappedData = nullptr;
}

void MapTrackingService::SnapshotAllMapped() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (const auto& [deviceMemoryKey, allocationInfo] : m_Allocations) {
    if (allocationInfo.IsMapped) {
      ScheduleMemoryUpdate(deviceMemoryKey);
    }
  }
}

void MapTrackingService::ScheduleMemoryUpdate(GITSKey deviceMemoryKey) {
  auto it = m_Allocations.find(deviceMemoryKey);
  GITS_ASSERT(it != m_Allocations.end());
  AllocationInfo& allocationInfo = it->second;

#ifdef GITS_PLATFORM_WINDOWS
  char* baseAddress = static_cast<char*>(allocationInfo.MappedData);
  uint64_t totalSize =
      allocationInfo.MapSize + (reinterpret_cast<uint64_t>(baseAddress) % m_PageSize);
  uint64_t pageCount = totalSize / m_PageSize + ((totalSize % m_PageSize) ? 1 : 0);
  std::vector<void*> touchedPages(pageCount);
  DWORD granularity{};
  UINT returnValue =
      GetWriteWatch(WRITE_WATCH_FLAG_RESET, allocationInfo.MappedData, allocationInfo.MapSize,
                    touchedPages.data(), &pageCount, &granularity);
  if (returnValue != 0) {
    static bool logged = false;
    if (!logged) {
      LOG_ERROR << "GetWriteWatch failed " << returnValue;
      logged = true;
    }
    return;
  }

  if (pageCount == 0) {
    return;
  }

  std::vector<MemoryRegions::Region> regions;

  uint64_t currentSize = granularity;
  char* currentPage = static_cast<char*>(touchedPages[0]);
  if (currentPage < baseAddress) {
    currentPage = baseAddress;
    currentSize -= (baseAddress - currentPage);
  }

  for (uint64_t i = 1; i < pageCount; ++i) {
    if (currentPage + currentSize == static_cast<char*>(touchedPages[i])) {
      currentSize += granularity;
    } else {
      uint64_t offset = currentPage - baseAddress;
      regions.push_back({offset, currentSize, currentPage});
      currentPage = static_cast<char*>(touchedPages[i]);
      currentSize = granularity;
    }
  }

  uint64_t offset = currentPage - baseAddress;
  if (offset + currentSize > allocationInfo.MapSize) {
    currentSize = allocationInfo.MapSize - offset;
  }
  regions.push_back({offset, currentSize, currentPage});
  RecordMappedDataMetaCommand(allocationInfo.DeviceKey, deviceMemoryKey, regions);
#else
  std::vector<MemoryRegions::Region> regions;
  regions.push_back({0, allocationInfo.MapSize, static_cast<char*>(allocationInfo.MappedData)});
  RecordMappedDataMetaCommand(allocationInfo.DeviceKey, deviceMemoryKey, regions);
#endif
}

void MapTrackingService::RecordMappedDataMetaCommand(
    GITSKey deviceKey, GITSKey memoryKey, const std::vector<MemoryRegions::Region>& regions) {
  MappedDataMetaCommand command(GITS_GET_THREAD_ID());
  command.m_Key = CaptureManager::Get().CreateCommandKey();
  command.m_Device.Key = deviceKey;
  command.m_Memory.Key = memoryKey;
  command.m_Regions.Size = static_cast<uint32_t>(regions.size());
  command.m_Regions.Regions = regions;
  m_GitsRecorder.record(command.m_Key, new MappedDataMetaWriter(command));
}

} // namespace vulkan
} // namespace gits
