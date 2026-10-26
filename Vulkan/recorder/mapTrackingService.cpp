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
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <pthread.h>
#include <sys/mman.h>

namespace {
struct sigaction g_PreviousSigsegv = {};
bool g_PreviousSigsegvValid = false;
// Cleared on teardown, after which every fault goes to the previous handler.
std::atomic<bool> g_MapTrackingActive{false};

void SigsegvHandler(int sig, siginfo_t* si, void* context);

bool IsWriteFault(void* context) {
#if defined(__x86_64__)
  // Bit 1 of the x86_64 page fault error code is set for write accesses.
  const auto* ucontext = static_cast<const ucontext_t*>(context);
  return (ucontext->uc_mcontext.gregs[REG_ERR] & 0x2) != 0;
#else
  return true;
#endif
}

void FallBackToDefaultAction(int sig, siginfo_t* si) {
  struct sigaction dfl = {};
  dfl.sa_handler = SIG_DFL;
  sigemptyset(&dfl.sa_mask);
  sigaction(SIGSEGV, &dfl, nullptr);
  // A kernel-generated fault re-triggers when the faulting instruction is re-executed after
  // return, so the default action reports the original context. A sent signal must be re-raised.
  if (si->si_code <= 0) {
    raise(sig);
  }
}

void ForwardToPreviousHandler(int sig, siginfo_t* si, void* context) {
  const struct sigaction& previous = g_PreviousSigsegv;
  if ((previous.sa_flags & SA_SIGINFO) != 0) {
    if (previous.sa_sigaction != nullptr && previous.sa_sigaction != SigsegvHandler) {
      sigset_t oldMask;
      pthread_sigmask(SIG_BLOCK, &previous.sa_mask, &oldMask);
      previous.sa_sigaction(sig, si, context);
      pthread_sigmask(SIG_SETMASK, &oldMask, nullptr);
      return;
    }
  } else if (previous.sa_handler == SIG_IGN) {
    // Only a sent signal can be ignored; ignoring a kernel-generated fault would re-execute the
    // faulting instruction forever.
    if (si->si_code <= 0) {
      return;
    }
  } else if (previous.sa_handler != SIG_DFL) {
    sigset_t oldMask;
    pthread_sigmask(SIG_BLOCK, &previous.sa_mask, &oldMask);
    previous.sa_handler(sig);
    pthread_sigmask(SIG_SETMASK, &oldMask, nullptr);
    return;
  }

  LOG_ERROR << "MapTrackingService signal handler caught unhandled signal:"
            << " code: " << si->si_code << " errno: " << si->si_errno
            << " at address: " << si->si_addr;
  FallBackToDefaultAction(sig, si);
}

void SigsegvHandler(int sig, siginfo_t* si, void* context) {
  const int savedErrno = errno;
  // Shadowed map pages are PROT_NONE or PROT_READ, so tracked accesses always fault with
  // SEGV_ACCERR. Anything else (including SIGSEGV sent by kill) belongs to the previous handler.
  if (si->si_code != SEGV_ACCERR || !g_MapTrackingActive.load(std::memory_order_acquire) ||
      !gits::vulkan::CaptureManager::Get().GetMapTrackingService().HandleAddress(
          si->si_addr, IsWriteFault(context))) {
    ForwardToPreviousHandler(sig, si, context);
  }
  errno = savedErrno;
}

void InstallSigsegvHandler() {
  struct sigaction sa = {};
  // SA_ONSTACK keeps stack overflow detection working for runtimes relying on sigaltstack.
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sa.sa_sigaction = SigsegvHandler;
  sigemptyset(&sa.sa_mask);
  // Keep the first non-GITS handler; only capture it on the initial install.
  struct sigaction previous = {};
  int ret = sigaction(SIGSEGV, &sa, g_PreviousSigsegvValid ? nullptr : &previous);
  GITS_ASSERT(ret != -1, "Sigaction setup for SIGSEGV failed.");
  if (!g_PreviousSigsegvValid) {
    g_PreviousSigsegv = previous;
    g_PreviousSigsegvValid = true;
  }
}

bool IsSigsegvHandlerInstalled() {
  struct sigaction current = {};
  int ret = sigaction(SIGSEGV, nullptr, &current);
  GITS_ASSERT(ret != -1, "Sigaction query for SIGSEGV failed.");
  return (current.sa_flags & SA_SIGINFO) != 0 && current.sa_sigaction == SigsegvHandler;
}

void RestoreSigsegvHandler() {
  // A handler installed after GITS may chain to it, so only the GITS handler itself is replaced.
  if (!IsSigsegvHandlerInstalled()) {
    return;
  }
  int ret = sigaction(SIGSEGV, &g_PreviousSigsegv, nullptr);
  GITS_ASSERT(ret != -1, "Sigaction restore for SIGSEGV failed.");
  g_PreviousSigsegvValid = false;
}
} // namespace

#endif

namespace gits {
namespace vulkan {

MapTrackingService::MapTrackingService(CaptureManager& manager, stream::OrderingRecorder& recorder)
    : m_Manager(manager), m_Recorder(recorder) {
#ifdef GITS_PLATFORM_WINDOWS
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  m_PageSize = si.dwPageSize;
#else
  m_PageSize = static_cast<uint64_t>(sysconf(_SC_PAGESIZE));
  InstallSigsegvHandler();
  g_MapTrackingActive.store(true, std::memory_order_release);
#endif
}

MapTrackingService::~MapTrackingService() {
#ifdef GITS_PLATFORM_LINUX
  g_MapTrackingActive.store(false, std::memory_order_release);
  std::lock_guard<std::mutex> lock(m_Mutex);
  // The app may keep using its mapped pointers after recording ends, so the shadows are left
  // mapped and accessible instead of being released.
  for (auto& entry : m_ShadowAllocations) {
    AllocationInfo* allocationInfo = entry.second;
    int ret = mprotect(allocationInfo->ShadowMemory, static_cast<size_t>(allocationInfo->MapSize),
                       PROT_READ | PROT_WRITE);
    GITS_ASSERT(ret != -1);
  }
  m_ShadowAllocations.clear();
  RestoreSigsegvHandler();
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

// Undo the EnableExternalMemory injection once the driver call is done. The import must never reach
// the stream - pHostPointer is a capture-process address, and a replay that honours the struct gets
// a NULL import instead of the allocation the app asked for.
void MapTrackingService::UnlinkExternalMemory(
    VkMemoryAllocateInfo* allocationInfo,
    std::optional<VkImportMemoryHostPointerInfoEXT>& hostPointerInfo) {
  if (!hostPointerInfo.has_value()) {
    return;
  }
  GITS_ASSERT(allocationInfo->pNext == &hostPointerInfo.value(),
              "pNext head changed after the write-watch import was injected.");
  allocationInfo->pNext = hostPointerInfo->pNext;
  hostPointerInfo.reset();
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
  m_Allocations[deviceMemoryKey] = std::move(allocationInfo);
}

void MapTrackingService::RemoveAllocationInfo(GITSKey deviceMemoryKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Allocations.find(deviceMemoryKey);
  if (it == m_Allocations.end()) {
    return;
  }
#ifdef GITS_PLATFORM_WINDOWS
  if (it->second.ExternalMemory) {
    VirtualFree(it->second.ExternalMemory, 0, MEM_RELEASE);
  }
#else
  // vkFreeMemory implicitly unmaps the memory. Writes since the last submit can never reach the
  // GPU, so the shadow is dropped without touching the already freed mapping.
  ReleaseShadowMemory(it->second);
#endif
  m_Allocations.erase(it);
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
  size_t shadowSize = static_cast<size_t>(allocationInfo.MapSize);
  allocationInfo.ShadowMemory =
      mmap(nullptr, shadowSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  GITS_ASSERT(allocationInfo.ShadowMemory != MAP_FAILED);
  std::memcpy(allocationInfo.ShadowMemory, *ppData, shadowSize);
  int ret = mprotect(allocationInfo.ShadowMemory, shadowSize, PROT_NONE);
  GITS_ASSERT(ret != -1);
  allocationInfo.PageStates.assign((shadowSize + m_PageSize - 1) / m_PageSize,
                                   PageState::Protected);
  allocationInfo.AccessedBegin = 0;
  allocationInfo.AccessedEnd = 0;
  m_ShadowAllocations[reinterpret_cast<uintptr_t>(allocationInfo.ShadowMemory)] = &allocationInfo;
  if (!m_SigsegvHandlerReplacedLogged && !IsSigsegvHandlerInstalled()) {
    LOG_WARNING << "MapTrackingService SIGSEGV handler was replaced. Accessing mapped memory may "
                   "crash unless the new handler forwards to the previous one.";
    m_SigsegvHandlerReplacedLogged = true;
  }
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
  ReleaseShadowMemory(it->second);
#endif
  it->second.IsMapped = false;
  it->second.MapOffset = 0;
  it->second.MapSize = 0;
  it->second.MappedData = nullptr;
}

#ifdef GITS_PLATFORM_LINUX
void MapTrackingService::ReleaseShadowMemory(AllocationInfo& allocationInfo) {
  if (allocationInfo.ShadowMemory) {
    m_ShadowAllocations.erase(reinterpret_cast<uintptr_t>(allocationInfo.ShadowMemory));
    int ret = munmap(allocationInfo.ShadowMemory, static_cast<size_t>(allocationInfo.MapSize));
    GITS_ASSERT(ret != -1);
    allocationInfo.ShadowMemory = nullptr;
  }
  allocationInfo.PageStates = {};
  allocationInfo.AccessedBegin = 0;
  allocationInfo.AccessedEnd = 0;
}
#endif

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
    currentSize -= (baseAddress - currentPage);
    currentPage = baseAddress;
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
  const size_t accessedBegin = allocationInfo.AccessedBegin;
  const size_t accessedEnd = allocationInfo.AccessedEnd;
  if (accessedBegin == accessedEnd) {
    return;
  }

  char* shadow = static_cast<char*>(allocationInfo.ShadowMemory);
  std::vector<PageState>& pageStates = allocationInfo.PageStates;

  // Calls onRun(firstPage, pageCount) for every run of consecutive pages in the accessed range
  // accepted by matches.
  auto forEachPageRun = [&](auto matches, auto onRun) {
    size_t first = accessedBegin;
    while (first < accessedEnd) {
      if (!matches(pageStates[first])) {
        ++first;
        continue;
      }
      size_t last = first + 1;
      while (last < accessedEnd && matches(pageStates[last])) {
        ++last;
      }
      onRun(first, last - first);
      first = last;
    }
  };

  auto isWritten = [](PageState state) { return state == PageState::Written; };
  auto isAccessed = [](PageState state) { return state != PageState::Protected; };

  // Written pages are made read-only before copying so concurrent writes fault and wait for the
  // lock instead of being lost.
  std::vector<MemoryRegions::Region> regions;
  forEachPageRun(isWritten, [&](size_t firstPage, size_t pageCount) {
    uint64_t offset = firstPage * m_PageSize;
    int ret = mprotect(shadow + offset, pageCount * m_PageSize, PROT_READ);
    GITS_ASSERT(ret != -1);
    uint64_t size = std::min<uint64_t>(pageCount * m_PageSize, allocationInfo.MapSize - offset);
    std::memcpy(static_cast<char*>(allocationInfo.MappedData) + offset, shadow + offset, size);
    regions.push_back({offset, size, shadow + offset});
  });

  if (!regions.empty()) {
    RecordMappedDataMetaCommand(allocationInfo.DeviceKey, deviceMemoryKey, regions);
  }

  forEachPageRun(isAccessed, [&](size_t firstPage, size_t pageCount) {
    int ret = mprotect(shadow + firstPage * m_PageSize, pageCount * m_PageSize, PROT_NONE);
    GITS_ASSERT(ret != -1);
  });
  std::fill(pageStates.begin() + accessedBegin, pageStates.begin() + accessedEnd,
            PageState::Protected);
  allocationInfo.AccessedBegin = 0;
  allocationInfo.AccessedEnd = 0;
#endif
}

void MapTrackingService::RecordMappedDataMetaCommand(
    GITSKey deviceKey, GITSKey memoryKey, const std::vector<MemoryRegions::Region>& regions) {
  MappedDataMetaCommand command(GITS_GET_THREAD_ID());
  command.m_Key = m_Manager.CreateCommandKey();
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
  // The last shadow starting at or below the address is the only one that can contain it.
  auto it = m_ShadowAllocations.upper_bound(addr);
  if (it == m_ShadowAllocations.begin()) {
    return false;
  }
  --it;
  uintptr_t shadowBegin = it->first;
  AllocationInfo& alloc = *it->second;
  if (pageAddr >= shadowBegin + alloc.MapSize) {
    return false;
  }

  uintptr_t pageOffset = pageAddr - shadowBegin;
  size_t pageIndex = pageOffset / m_PageSize;
  PageState& state = alloc.PageStates[pageIndex];
  // Another thread may have unprotected the page while this one waited for the lock.
  if (state == PageState::Written || (!isWrite && state == PageState::Read)) {
    return true;
  }

  int ret = mprotect(reinterpret_cast<void*>(pageAddr), m_PageSize, PROT_READ | PROT_WRITE);
  GITS_ASSERT(ret != -1);
  if (isWrite) {
    state = PageState::Written;
  } else {
    size_t copySize =
        std::min(static_cast<size_t>(m_PageSize), static_cast<size_t>(alloc.MapSize - pageOffset));
    std::memcpy(reinterpret_cast<void*>(pageAddr),
                static_cast<char*>(alloc.MappedData) + pageOffset, copySize);
    ret = mprotect(reinterpret_cast<void*>(pageAddr), m_PageSize, PROT_READ);
    GITS_ASSERT(ret != -1);
    state = PageState::Read;
  }

  if (alloc.AccessedBegin == alloc.AccessedEnd) {
    alloc.AccessedBegin = pageIndex;
    alloc.AccessedEnd = pageIndex + 1;
  } else {
    alloc.AccessedBegin = std::min(alloc.AccessedBegin, pageIndex);
    alloc.AccessedEnd = std::max(alloc.AccessedEnd, pageIndex + 1);
  }
  return true;
}
#endif

} // namespace vulkan
} // namespace gits
