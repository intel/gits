// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingService.h"
#include "captureManager.h"
#include "commandSerializersCustom.h"

#ifdef GITS_PLATFORM_LINUX
#include <csignal>
#include <sys/mman.h>

static void SigsegvHandler(int sig, siginfo_t* si, [[maybe_unused]] void* unused) {
  void* faultAddr = si->si_addr;
  bool isWrite = true;

#if defined(__x86_64__) || defined(__i386__)
  // On x86/x86_64, the CPU stores error code flags in the ucontext
  ucontext_t* context = reinterpret_cast<ucontext_t*>(unused);
  isWrite = (context->uc_mcontext.gregs[REG_ERR] & 0x2) != 0;
#endif

  if (!gits::vulkan::CaptureManager::Get().GetMapTrackingService().HandleAddress(faultAddr,
                                                                                 isWrite)) {
    LOG_ERROR << "MapTrackingService signal handler caught unhandled signal: "
              << " errno: " << si->si_errno << " at address: " << si->si_addr;
    exit(1);
  }
}

#endif

namespace gits {
namespace vulkan {

MapTrackingService::MapTrackingService(stream::OrderingRecorder& recorder) : m_Recorder(recorder) {
#ifdef GITS_PLATFORM_WINDOWS
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  m_PageSize = si.dwPageSize;
#else
  m_PageSize = static_cast<uint64_t>(sysconf(_SC_PAGESIZE));
  struct sigaction sa = {};
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = SigsegvHandler;
  sigemptyset(&sa.sa_mask);
  int ret = sigaction(SIGSEGV, &sa, nullptr);
  GITS_ASSERT(ret != -1, "Sigaction setup for SIGSEGV failed.");
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
                                   void** ppData) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Allocations.find(deviceMemoryKey);
  GITS_ASSERT(it != m_Allocations.end());
  auto& allocationInfo = it->second;
  allocationInfo.IsMapped = true;
  allocationInfo.MapOffset = offset;
  allocationInfo.MapSize = (size == VK_WHOLE_SIZE) ? allocationInfo.AllocationSize - offset : size;
  allocationInfo.MappedData = *ppData;
#ifdef GITS_PLATFORM_WINDOWS
  ResetWriteWatch(allocationInfo.MappedData, static_cast<SIZE_T>(allocationInfo.MapSize));
#else
  allocationInfo.TouchedPages.clear();
  allocationInfo.ReadTriggeredPages.clear();
  size_t shadowSize = static_cast<size_t>(allocationInfo.MapSize);
  allocationInfo.ShadowMemory =
      mmap(nullptr, shadowSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  GITS_ASSERT(allocationInfo.ShadowMemory != MAP_FAILED);
  std::memcpy(allocationInfo.ShadowMemory, *ppData, shadowSize);
  int ret = mprotect(allocationInfo.ShadowMemory, shadowSize, PROT_NONE);
  GITS_ASSERT(ret != -1);
  *ppData = allocationInfo.ShadowMemory;
#endif
}

void MapTrackingService::RemoveData(GITSKey deviceMemoryKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Allocations.find(deviceMemoryKey);
  GITS_ASSERT(it != m_Allocations.end());
  if (it->second.IsMapped) {
    ScheduleMemoryUpdate(deviceMemoryKey);
  }
#ifdef GITS_PLATFORM_LINUX
  if (it->second.ShadowMemory) {
    int ret = munmap(it->second.ShadowMemory, static_cast<size_t>(it->second.MapSize));
    GITS_ASSERT(ret != -1);
    it->second.ShadowMemory = nullptr;
  }
#endif
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
  if (allocationInfo.TouchedPages.empty() && allocationInfo.ReadTriggeredPages.empty()) {
    return;
  }

  if (!allocationInfo.TouchedPages.empty()) {
    for (uintptr_t page : allocationInfo.TouchedPages) {
      int ret = mprotect(reinterpret_cast<void*>(page), m_PageSize, PROT_READ);
      GITS_ASSERT(ret != -1);
    }

    std::vector<MemoryRegions::Region> regions;

    std::vector<uintptr_t> sortedPages(allocationInfo.TouchedPages.begin(),
                                       allocationInfo.TouchedPages.end());
    std::sort(sortedPages.begin(), sortedPages.end());

    char* baseAddress = static_cast<char*>(allocationInfo.ShadowMemory);
    uintptr_t mapStart = reinterpret_cast<uintptr_t>(baseAddress);
    uintptr_t mapEnd = mapStart + allocationInfo.MapSize;

    uintptr_t regionStart = sortedPages[0];
    uintptr_t regionEnd = regionStart + m_PageSize;

    auto flushRegion = [&](uintptr_t start, uintptr_t end) {
      if (start < mapStart) {
        start = mapStart;
      }
      if (end > mapEnd) {
        end = mapEnd;
      }
      if (end > start) {
        uint64_t offset = start - mapStart;
        uint64_t size = end - start;
        regions.push_back({offset, size, reinterpret_cast<char*>(start)});
      }
    };

    for (size_t i = 1; i < sortedPages.size(); ++i) {
      if (sortedPages[i] == regionEnd) {
        regionEnd += m_PageSize;
      } else {
        flushRegion(regionStart, regionEnd);
        regionStart = sortedPages[i];
        regionEnd = regionStart + m_PageSize;
      }
    }
    flushRegion(regionStart, regionEnd);

    for (const auto& region : regions) {
      std::memcpy(static_cast<char*>(allocationInfo.MappedData) + region.Offset, region.Data,
                  region.Size);
    }

    if (!regions.empty()) {
      RecordMappedDataMetaCommand(allocationInfo.DeviceKey, deviceMemoryKey, regions);
    }
  }

  std::unordered_set<uintptr_t> pagesToReprotect = allocationInfo.TouchedPages;
  pagesToReprotect.insert(allocationInfo.ReadTriggeredPages.begin(),
                          allocationInfo.ReadTriggeredPages.end());
  for (uintptr_t page : pagesToReprotect) {
    errno = 0;
    int ret = mprotect(reinterpret_cast<void*>(page), m_PageSize, PROT_NONE);
    GITS_ASSERT(ret != -1);
  }
  allocationInfo.TouchedPages.clear();
  allocationInfo.ReadTriggeredPages.clear();
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
  m_Recorder.Record(command.m_Key, new MappedDataMetaSerializer(command));
}

#ifdef GITS_PLATFORM_LINUX
bool MapTrackingService::HandleAddress(void* address, bool isWrite) {
  uintptr_t addr = reinterpret_cast<uintptr_t>(address);
  uintptr_t pageAddr = addr - (addr % m_PageSize);

  std::lock_guard<std::mutex> lock(m_Mutex);
  for (auto& [key, alloc] : m_Allocations) {
    if (!alloc.IsMapped || alloc.ShadowMemory == nullptr) {
      continue;
    }

    uintptr_t shadowBegin = reinterpret_cast<uintptr_t>(alloc.ShadowMemory);
    uintptr_t shadowEnd = shadowBegin + alloc.MapSize;

    if (pageAddr >= shadowBegin && pageAddr < shadowEnd) {
      errno = 0;
      int ret = mprotect(reinterpret_cast<void*>(pageAddr), m_PageSize, PROT_READ | PROT_WRITE);
      GITS_ASSERT(ret != -1);
      if (isWrite) {
        alloc.TouchedPages.insert(pageAddr);
      } else {
        uintptr_t pageOffset = pageAddr - shadowBegin;
        size_t copySize = std::min(static_cast<size_t>(m_PageSize),
                                   static_cast<size_t>(alloc.MapSize - pageOffset));
        std::memcpy(reinterpret_cast<void*>(pageAddr),
                    static_cast<char*>(alloc.MappedData) + pageOffset, copySize);
        ret = mprotect(reinterpret_cast<void*>(pageAddr), m_PageSize, PROT_READ);
        GITS_ASSERT(ret != -1);
        alloc.ReadTriggeredPages.insert(pageAddr);
      }
      return true;
    }
  }
  return false;
}
#endif

} // namespace vulkan
} // namespace gits
