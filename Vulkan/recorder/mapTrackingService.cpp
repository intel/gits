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
#include "vulkanHelpers.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>

#ifdef GITS_PLATFORM_LINUX
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

#else
#include <atomic>

namespace {
// Cleared on teardown, after which every access violation goes to the next handler.
std::atomic<bool> g_MapTrackingActive{false};
PVOID g_ExceptionHandler = nullptr;

LONG CALLBACK MapTrackingExceptionHandler(EXCEPTION_POINTERS* exceptionInfo) {
  const EXCEPTION_RECORD& record = *exceptionInfo->ExceptionRecord;
  // Stale shadow pages are PAGE_NOACCESS, so tracked accesses always raise an access violation.
  if (record.ExceptionCode != EXCEPTION_ACCESS_VIOLATION || record.NumberParameters < 2 ||
      !g_MapTrackingActive.load(std::memory_order_acquire)) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  void* address = reinterpret_cast<void*>(record.ExceptionInformation[1]);
  return gits::vulkan::CaptureManager::Get().GetMapTrackingService().HandleAddress(address)
             ? EXCEPTION_CONTINUE_EXECUTION
             : EXCEPTION_CONTINUE_SEARCH;
}

// Replaces the value at `shadow` with `desired` unless it no longer equals `expected`, which
// means the application wrote it. Returns whether a replacement was attempted.
template <typename T>
bool ReplaceUnwritten(char* shadow, const char* expected, const char* desired) {
  T expectedValue{};
  T desiredValue{};
  std::memcpy(&expectedValue, expected, sizeof(T));
  std::memcpy(&desiredValue, desired, sizeof(T));
  if (expectedValue == desiredValue) {
    return false;
  }
  std::atomic_ref<T>(*reinterpret_cast<T*>(shadow))
      .compare_exchange_strong(expectedValue, desiredValue);
  return true;
}

// Applies ReplaceUnwritten to every value of [first, last), with `expected` and `desired` holding
// the values of the range.
bool ReplaceUnwrittenRange(char* first, char* last, const char* expected, const char* desired) {
  auto replace = [&](auto value, char* address) {
    const size_t index = address - first;
    return ReplaceUnwritten<decltype(value)>(address, expected + index, desired + index);
  };
  bool replaced = false;
  char* address = first;
  for (; address < last && reinterpret_cast<uintptr_t>(address) % sizeof(uint64_t) != 0;
       ++address) {
    replaced |= replace(uint8_t{}, address);
  }
  for (; address + sizeof(uint64_t) <= last; address += sizeof(uint64_t)) {
    replaced |= replace(uint64_t{}, address);
  }
  for (; address < last; ++address) {
    replaced |= replace(uint8_t{}, address);
  }
  return replaced;
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
  m_AllocationGranularity = si.dwAllocationGranularity;
  m_PageSnapshot.resize(m_PageSize);
  m_PageDeviceData.resize(m_PageSize);
  g_ExceptionHandler = AddVectoredExceptionHandler(1, MapTrackingExceptionHandler);
  GITS_ASSERT(g_ExceptionHandler != nullptr, "Adding the mapped memory exception handler failed.");
  g_MapTrackingActive.store(true, std::memory_order_release);
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
#else
  std::lock_guard<std::mutex> lock(m_Mutex);
  // The app may keep using its mapped pointers after recording ends, so stale shadow pages are
  // made accessible instead of being released, before the handler stops refreshing them.
  for (auto& entry : m_ShadowAllocations) {
    AllocationInfo* allocationInfo = entry.second;
    char* pageBase = ShadowPageBase(*allocationInfo);
    for (size_t page = 0; page < allocationInfo->PageStates.size(); ++page) {
      if (allocationInfo->PageStates[page] == PageState::Protected) {
        DWORD oldProtect{};
        BOOL ret =
            VirtualProtect(pageBase + page * m_PageSize, m_PageSize, PAGE_READWRITE, &oldProtect);
        GITS_ASSERT(ret != 0);
        allocationInfo->PageStates[page] = PageState::Read;
      }
    }
  }
  m_ShadowAllocations.clear();
  g_MapTrackingActive.store(false, std::memory_order_release);
  RemoveVectoredExceptionHandler(g_ExceptionHandler);
  g_ExceptionHandler = nullptr;
#endif
}

void MapTrackingService::StoreInstanceApiVersion(GITSKey instanceKey, uint32_t apiVersion) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_InstanceApiVersions[instanceKey] = apiVersion;
}

void MapTrackingService::StorePhysicalDeviceInstance(GITSKey physicalDeviceKey,
                                                     GITSKey instanceKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_PhysicalDeviceToInstance[physicalDeviceKey] = instanceKey;
}

std::optional<VkDeviceSize> MapTrackingService::QueryHostPointerImport(
    HandleArgument<VkPhysicalDevice>& physicalDevice,
    const std::vector<const char*>& enabledExtensions) {
  auto isEnabled = [&enabledExtensions](const char* name) {
    return std::any_of(enabledExtensions.begin(), enabledExtensions.end(),
                       [name](const char* extension) { return std::strcmp(extension, name) == 0; });
  };

  uint32_t instanceApiVersion = VK_API_VERSION_1_0;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto instanceIt = m_PhysicalDeviceToInstance.find(physicalDevice.Key);
    if (instanceIt != m_PhysicalDeviceToInstance.end()) {
      auto versionIt = m_InstanceApiVersions.find(instanceIt->second);
      if (versionIt != m_InstanceApiVersions.end()) {
        instanceApiVersion = versionIt->second;
      }
    }
  }

  const auto& dt = m_Manager.GetInstanceDispatchTable(physicalDevice.Value);
  VkPhysicalDeviceProperties deviceProperties{};
  dt.vkGetPhysicalDeviceProperties(physicalDevice.Value, &deviceProperties);
  const uint32_t deviceApiVersion = std::min(instanceApiVersion, deviceProperties.apiVersion);
  // VK_EXT_external_memory_host depends on VK_KHR_external_memory, which is core in 1.1.
  if (deviceApiVersion < VK_API_VERSION_1_1 && !isEnabled(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME)) {
    return std::nullopt;
  }

  uint32_t extensionCount = 0;
  if (dt.vkEnumerateDeviceExtensionProperties(physicalDevice.Value, nullptr, &extensionCount,
                                              nullptr) != VK_SUCCESS) {
    return std::nullopt;
  }
  std::vector<VkExtensionProperties> extensions(extensionCount);
  if (dt.vkEnumerateDeviceExtensionProperties(physicalDevice.Value, nullptr, &extensionCount,
                                              extensions.data()) != VK_SUCCESS) {
    return std::nullopt;
  }
  if (std::none_of(extensions.begin(), extensions.end(), [](const VkExtensionProperties& ext) {
        return std::strcmp(ext.extensionName, VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME) == 0;
      })) {
    return std::nullopt;
  }

  auto vkGetPhysicalDeviceProperties2Unified = dt.vkGetPhysicalDeviceProperties2
                                                   ? dt.vkGetPhysicalDeviceProperties2
                                                   : dt.vkGetPhysicalDeviceProperties2KHR;
  if (!vkGetPhysicalDeviceProperties2Unified) {
    return std::nullopt;
  }
  VkPhysicalDeviceExternalMemoryHostPropertiesEXT hostProperties{};
  hostProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT;
  VkPhysicalDeviceProperties2 properties{};
  properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  properties.pNext = &hostProperties;
  vkGetPhysicalDeviceProperties2Unified(physicalDevice.Value, &properties);
  if (hostProperties.minImportedHostPointerAlignment == 0) {
    return std::nullopt;
  }
  return hostProperties.minImportedHostPointerAlignment;
}

void MapTrackingService::StoreHostPointerImport(GITSKey deviceKey, VkDeviceSize alignment) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_HostPointerImportAlignments[deviceKey] = alignment;
}

void MapTrackingService::StoreDevice(GITSKey deviceKey,
                                     GITSKey physicalDeviceKey,
                                     uint32_t deviceGroupSize) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Devices[deviceKey] = DeviceInfo{physicalDeviceKey, deviceGroupSize};
}

void MapTrackingService::StorePhysicalDeviceMemoryProperties(
    GITSKey physicalDeviceKey, const VkPhysicalDeviceMemoryProperties& memoryProperties) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_PhysicalDeviceMemoryProperties[physicalDeviceKey] = memoryProperties;
}

bool MapTrackingService::IsMemoryMappable(GITSKey deviceKey, uint32_t memoryTypeIndex) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto deviceIt = m_Devices.find(deviceKey);
  GITS_ASSERT(deviceIt != m_Devices.end());
  GITSKey physicalDeviceKey = deviceIt->second.PhysicalDeviceKey;
  auto it = m_PhysicalDeviceMemoryProperties.find(physicalDeviceKey);
  GITS_ASSERT(it != m_PhysicalDeviceMemoryProperties.end());
  VkPhysicalDeviceMemoryProperties& memoryProperties = it->second;
  return (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags &
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
}

void* MapTrackingService::EnableExternalMemory(
    GITSKey deviceKey,
    VkDevice device,
    VkMemoryAllocateInfo* allocationInfo,
    std::optional<VkImportMemoryHostPointerInfoEXT>& hostPointerInfo) {
#ifdef GITS_PLATFORM_WINDOWS
  VkDeviceSize alignment{};
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_HostPointerImportAlignments.find(deviceKey);
    if (it == m_HostPointerImportAlignments.end()) {
      return nullptr;
    }
    alignment = it->second;
  }
  if (allocationInfo->allocationSize % alignment != 0) {
    return nullptr;
  }
  // Memory can have only one external source, and exported memory must stay driver-owned so other
  // APIs and processes can import it.
  if (IsMemoryImport(allocationInfo->pNext) || IsMemoryExport(allocationInfo->pNext)) {
    return nullptr;
  }
  // Host pointer imports can't be dedicated allocations.
  auto* dedicatedInfo = static_cast<const VkMemoryDedicatedAllocateInfo*>(
      getPNextStructure(allocationInfo->pNext, VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO));
  if (dedicatedInfo &&
      (dedicatedInfo->image != VK_NULL_HANDLE || dedicatedInfo->buffer != VK_NULL_HANDLE)) {
    return nullptr;
  }

  void* hostPtr = VirtualAlloc(nullptr, static_cast<SIZE_T>(allocationInfo->allocationSize),
                               MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE);
  GITS_ASSERT(hostPtr != nullptr);
  VkMemoryHostPointerPropertiesEXT hostPointerProperties{};
  hostPointerProperties.sType = VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT;
  const auto& dt = m_Manager.GetDeviceDispatchTable(device);
  if (dt.vkGetMemoryHostPointerPropertiesEXT(device,
                                             VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT,
                                             hostPtr, &hostPointerProperties) != VK_SUCCESS ||
      (hostPointerProperties.memoryTypeBits & (1u << allocationInfo->memoryTypeIndex)) == 0) {
    VirtualFree(hostPtr, 0, MEM_RELEASE);
    return nullptr;
  }
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
  allocationInfo.IsExported = IsMemoryExport(allocateInfo.pNext);
  auto deviceIt = m_Devices.find(deviceKey);
  if (deviceIt != m_Devices.end()) {
    auto propertiesIt = m_PhysicalDeviceMemoryProperties.find(deviceIt->second.PhysicalDeviceKey);
    if (propertiesIt != m_PhysicalDeviceMemoryProperties.end()) {
      allocationInfo.IsCoherent =
          isBitSet(propertiesIt->second.memoryTypes[allocateInfo.memoryTypeIndex].propertyFlags,
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
  }
  m_Allocations[deviceMemoryKey] = std::move(allocationInfo);
}

void MapTrackingService::ClearMemory(VkDevice device,
                                     GITSKey deviceMemoryKey,
                                     const VkMemoryAllocateInfo& allocateInfo) {
  VkDeviceMemory memory{};
  VkMemoryPropertyFlags propertyFlags{};
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Allocations.find(deviceMemoryKey);
    GITS_ASSERT(it != m_Allocations.end());
    const AllocationInfo& allocationInfo = it->second;
    if (allocationInfo.ExternalMemory) {
      return;
    }
    const DeviceInfo& deviceInfo = m_Devices.at(allocationInfo.DeviceKey);
    const VkPhysicalDeviceMemoryProperties& memoryProperties =
        m_PhysicalDeviceMemoryProperties.at(deviceInfo.PhysicalDeviceKey);
    // The application cannot map memory with multiple instances either.
    if (HasMultipleInstances(allocateInfo, memoryProperties, deviceInfo.DeviceGroupSize)) {
      return;
    }
    memory = allocationInfo.Memory;
    propertyFlags = memoryProperties.memoryTypes[allocationInfo.MemoryTypeIndex].propertyFlags;
  }

  VkDeviceLevelDispatchTable& dispatchTable = m_Manager.GetDeviceDispatchTable(device);
  void* data{};
  const auto errorMessage = [&](const char* operation, VkResult result) {
    std::ostringstream message;
    message << "Cannot record memory (key " << deviceMemoryKey << ", "
            << allocateInfo.allocationSize << " bytes): " << operation
            << " the whole allocation to zero it failed with " << result
            << ". The player starts from zeroed memory, so pages the application reads before "
               "writing them would differ in the replay. Record on a system with more free host "
               "memory.";
    return message.str();
  };
  VkResult result = dispatchTable.vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &data);
  if (result != VK_SUCCESS) {
    GITS_ASSERT(false, errorMessage("mapping", result));
    return;
  }
  const VkDeviceSize size = allocateInfo.allocationSize;
  std::memset(data, 0, static_cast<size_t>(size));
  if (!(propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, memory, 0,
                              VK_WHOLE_SIZE};
    result = dispatchTable.vkFlushMappedMemoryRanges(device, 1, &range);
  }
  dispatchTable.vkUnmapMemory(device, memory);
  if (result != VK_SUCCESS) {
    GITS_ASSERT(false, errorMessage("flushing", result));
  }
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
#endif
  // vkFreeMemory implicitly unmaps the memory. Writes since the last submit can never reach the
  // GPU, so the shadow is dropped without touching the already freed mapping.
  ReleaseShadowMemory(it->second);
  if (it->second.IsMapped && it->second.IsExported) {
    m_MappedExportedCount.fetch_sub(1, std::memory_order_release);
  }
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
  if (allocationInfo.IsExported && !allocationInfo.IsMapped) {
    m_MappedExportedCount.fetch_add(1, std::memory_order_release);
  }
  allocationInfo.IsMapped = true;
  allocationInfo.MapOffset = offset;
  allocationInfo.MapSize = (size == VK_WHOLE_SIZE) ? allocationInfo.AllocationSize - offset : size;
  allocationInfo.MappedData = *ppData;
#ifdef GITS_PLATFORM_WINDOWS
  const SIZE_T mapSize = static_cast<SIZE_T>(allocationInfo.MapSize);
  if (allocationInfo.ExternalMemory) {
    ResetWriteWatch(allocationInfo.MappedData, mapSize);
    return;
  }
  // Write watch works only on GITS-allocated memory, so the app writes to a shadow that is copied
  // to the driver mapping on every update.
  const SIZE_T residue =
      static_cast<SIZE_T>(reinterpret_cast<uintptr_t>(*ppData) % m_AllocationGranularity);
  allocationInfo.ShadowAllocation = VirtualAlloc(
      nullptr, residue + mapSize, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE);
  GITS_ASSERT(allocationInfo.ShadowAllocation != nullptr);
  allocationInfo.ShadowMemory = static_cast<char*>(allocationInfo.ShadowAllocation) + residue;
  std::memcpy(allocationInfo.ShadowMemory, *ppData, mapSize);
  ResetWriteWatch(allocationInfo.ShadowMemory, mapSize);
  allocationInfo.PageStates.assign((residue % m_PageSize + mapSize + m_PageSize - 1) / m_PageSize,
                                   PageState::Read);
  m_ShadowAllocations[reinterpret_cast<uintptr_t>(allocationInfo.ShadowMemory)] = &allocationInfo;
  *ppData = allocationInfo.ShadowMemory;
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
    if (it->second.IsExported) {
      m_MappedExportedCount.fetch_sub(1, std::memory_order_release);
    }
  }
  ReleaseShadowMemory(it->second);
  it->second.IsMapped = false;
  it->second.MapOffset = 0;
  it->second.MapSize = 0;
  it->second.MappedData = nullptr;
}

void MapTrackingService::ReleaseShadowMemory(AllocationInfo& allocationInfo) {
#ifdef GITS_PLATFORM_WINDOWS
  if (allocationInfo.ShadowAllocation) {
    m_ShadowAllocations.erase(reinterpret_cast<uintptr_t>(allocationInfo.ShadowMemory));
    VirtualFree(allocationInfo.ShadowAllocation, 0, MEM_RELEASE);
    allocationInfo.ShadowAllocation = nullptr;
    allocationInfo.ShadowMemory = nullptr;
  }
  allocationInfo.RefreshedWrittenPages.clear();
  allocationInfo.PageStates = {};
#else
  if (allocationInfo.ShadowMemory) {
    m_ShadowAllocations.erase(reinterpret_cast<uintptr_t>(allocationInfo.ShadowMemory));
    int ret = munmap(allocationInfo.ShadowMemory, static_cast<size_t>(allocationInfo.MapSize));
    GITS_ASSERT(ret != -1);
    allocationInfo.ShadowMemory = nullptr;
  }
  allocationInfo.PageStates = {};
  allocationInfo.AccessedBegin = 0;
  allocationInfo.AccessedEnd = 0;
#endif
}

void MapTrackingService::SnapshotAllMapped() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (const auto& [deviceMemoryKey, allocationInfo] : m_Allocations) {
    if (allocationInfo.IsMapped) {
      ScheduleMemoryUpdate(deviceMemoryKey);
    }
  }
}

void MapTrackingService::SnapshotExportedMapped() {
  if (m_MappedExportedCount.load(std::memory_order_acquire) == 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (const auto& [deviceMemoryKey, allocationInfo] : m_Allocations) {
    if (allocationInfo.IsMapped && allocationInfo.IsExported) {
      ScheduleMemoryUpdate(deviceMemoryKey);
    }
  }
}

void MapTrackingService::SnapshotMapped(GITSKey deviceMemoryKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Allocations.find(deviceMemoryKey);
  if (it != m_Allocations.end() && it->second.IsMapped) {
    ScheduleMemoryUpdate(deviceMemoryKey);
  }
}

void MapTrackingService::RefreshMapped(GITSKey deviceMemoryKey,
                                       VkDeviceSize offset,
                                       VkDeviceSize size) {
#ifdef GITS_PLATFORM_WINDOWS
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Allocations.find(deviceMemoryKey);
  if (it == m_Allocations.end() || !it->second.ShadowMemory) {
    return;
  }
  AllocationInfo& allocationInfo = it->second;
  const VkDeviceSize mapEnd = allocationInfo.MapOffset + allocationInfo.MapSize;
  const VkDeviceSize begin = std::max(offset, allocationInfo.MapOffset);
  const VkDeviceSize end = (size == VK_WHOLE_SIZE) ? mapEnd : std::min(offset + size, mapEnd);
  if (begin < end) {
    InvalidateShadowMemory(allocationInfo, begin - allocationInfo.MapOffset,
                           end - allocationInfo.MapOffset);
  }
#endif
}

void MapTrackingService::RefreshCoherentMapped() {
#ifdef GITS_PLATFORM_WINDOWS
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (auto& entry : m_ShadowAllocations) {
    AllocationInfo& allocationInfo = *entry.second;
    if (allocationInfo.IsCoherent) {
      InvalidateShadowMemory(allocationInfo, 0, allocationInfo.MapSize);
    }
  }
#endif
}

#ifdef GITS_PLATFORM_WINDOWS
char* MapTrackingService::ShadowPageBase(const AllocationInfo& allocationInfo) const {
  char* shadow = static_cast<char*>(allocationInfo.ShadowMemory);
  return shadow - reinterpret_cast<uintptr_t>(shadow) % m_PageSize;
}

// Protects the shadow pages of [begin, end) the application has not written since the last
// update, so that each is refreshed from the mapped memory on its next access. Only the pages the
// application accesses are read from the mapped memory, which is often uncached.
void MapTrackingService::InvalidateShadowMemory(AllocationInfo& allocationInfo,
                                                VkDeviceSize begin,
                                                VkDeviceSize end) {
  char* const shadow = static_cast<char*>(allocationInfo.ShadowMemory);
  char* const pageBase = ShadowPageBase(allocationInfo);
  const size_t firstPage = (shadow + begin - pageBase) / m_PageSize;
  const size_t lastPage = (shadow + end - 1 - pageBase) / m_PageSize + 1;
  const size_t pageCount = lastPage - firstPage;

  std::vector<void*> writtenPages(pageCount);
  ULONG_PTR writtenCount = pageCount;
  DWORD granularity{};
  UINT returnValue = GetWriteWatch(0, pageBase + firstPage * m_PageSize, pageCount * m_PageSize,
                                   writtenPages.data(), &writtenCount, &granularity);
  if (returnValue != 0) {
    static bool logged = false;
    if (!logged) {
      LOG_ERROR << "GetWriteWatch failed " << returnValue;
      logged = true;
    }
    return;
  }

  std::vector<PageState>& pageStates = allocationInfo.PageStates;
  size_t runBegin = firstPage;
  auto protectRun = [&](size_t runEnd) {
    if (runBegin < runEnd) {
      DWORD oldProtect{};
      BOOL ret = VirtualProtect(pageBase + runBegin * m_PageSize, (runEnd - runBegin) * m_PageSize,
                                PAGE_NOACCESS, &oldProtect);
      GITS_ASSERT(ret != 0);
      std::fill(pageStates.begin() + runBegin, pageStates.begin() + runEnd, PageState::Protected);
    }
  };
  ULONG_PTR written = 0;
  for (size_t page = firstPage; page < lastPage; ++page) {
    char* const address = pageBase + page * m_PageSize;
    bool isWritten = false;
    if (written < writtenCount && writtenPages[written] == address) {
      ++written;
      isWritten = true;
    }
    if (isWritten || allocationInfo.RefreshedWrittenPages.count(address) != 0) {
      protectRun(page);
      runBegin = page + 1;
    }
  }
  protectRun(lastPage);
}

// Copies device writes from the mapped memory to a protected shadow page. Values are replaced only
// if they still match a snapshot taken while the page was read-only, so concurrent application
// writes are kept, and a page written meanwhile is copied to the mapped memory on the next update.
void MapTrackingService::RefreshShadowPage(AllocationInfo& allocationInfo, size_t pageIndex) {
  char* const shadow = static_cast<char*>(allocationInfo.ShadowMemory);
  char* const page = ShadowPageBase(allocationInfo) + pageIndex * m_PageSize;
  char* const first = std::max(page, shadow);
  char* const last = std::min(page + m_PageSize, shadow + allocationInfo.MapSize);
  const size_t size = last - first;

  // A page the application wrote between the write watch query and its protection keeps its
  // content.
  void* writtenPage{};
  ULONG_PTR writtenCount = 1;
  DWORD granularity{};
  const bool isWritten =
      GetWriteWatch(0, page, m_PageSize, &writtenPage, &writtenCount, &granularity) != 0 ||
      writtenCount != 0;

  DWORD oldProtect{};
  BOOL ret{};
  if (!isWritten) {
    ret = VirtualProtect(page, m_PageSize, PAGE_READONLY, &oldProtect);
    GITS_ASSERT(ret != 0);
    std::memcpy(m_PageSnapshot.data(), first, size);
    std::memcpy(m_PageDeviceData.data(),
                static_cast<const char*>(allocationInfo.MappedData) + (first - shadow), size);
  }
  ret = VirtualProtect(page, m_PageSize, PAGE_READWRITE, &oldProtect);
  GITS_ASSERT(ret != 0);
  allocationInfo.PageStates[pageIndex] = PageState::Read;
  if (isWritten ||
      !ReplaceUnwrittenRange(first, last, m_PageSnapshot.data(), m_PageDeviceData.data())) {
    return;
  }
  ResetWriteWatch(page, m_PageSize);
  if (std::memcmp(first, m_PageDeviceData.data(), size) != 0) {
    allocationInfo.RefreshedWrittenPages.insert(page);
  }
}

bool MapTrackingService::HandleAddress(void* address) {
  const uintptr_t addr = reinterpret_cast<uintptr_t>(address);
  std::lock_guard<std::mutex> lock(m_Mutex);
  // The last shadow starting at or below the address is the only one that can contain it.
  auto it = m_ShadowAllocations.upper_bound(addr);
  if (it == m_ShadowAllocations.begin()) {
    return false;
  }
  --it;
  AllocationInfo& allocationInfo = *it->second;
  if (addr >= it->first + allocationInfo.MapSize) {
    return false;
  }
  const size_t pageIndex =
      (addr - reinterpret_cast<uintptr_t>(ShadowPageBase(allocationInfo))) / m_PageSize;
  // Another thread may have refreshed the page while this one waited for the lock.
  if (allocationInfo.PageStates[pageIndex] == PageState::Protected) {
    RefreshShadowPage(allocationInfo, pageIndex);
  }
  return true;
}
#endif

void MapTrackingService::ScheduleMemoryUpdate(GITSKey deviceMemoryKey) {
  auto it = m_Allocations.find(deviceMemoryKey);
  GITS_ASSERT(it != m_Allocations.end());
  AllocationInfo& allocationInfo = it->second;

#ifdef GITS_PLATFORM_WINDOWS
  char* baseAddress = static_cast<char*>(allocationInfo.ShadowMemory ? allocationInfo.ShadowMemory
                                                                     : allocationInfo.MappedData);
  uint64_t totalSize =
      allocationInfo.MapSize + (reinterpret_cast<uint64_t>(baseAddress) % m_PageSize);
  uint64_t pageCount = totalSize / m_PageSize + ((totalSize % m_PageSize) ? 1 : 0);
  std::vector<void*> touchedPages(pageCount);
  DWORD granularity{};
  UINT returnValue = GetWriteWatch(WRITE_WATCH_FLAG_RESET, baseAddress, allocationInfo.MapSize,
                                   touchedPages.data(), &pageCount, &granularity);
  if (returnValue != 0) {
    static bool logged = false;
    if (!logged) {
      LOG_ERROR << "GetWriteWatch failed " << returnValue;
      logged = true;
    }
    return;
  }

  touchedPages.resize(pageCount);
  if (!allocationInfo.RefreshedWrittenPages.empty()) {
    touchedPages.insert(touchedPages.end(), allocationInfo.RefreshedWrittenPages.begin(),
                        allocationInfo.RefreshedWrittenPages.end());
    allocationInfo.RefreshedWrittenPages.clear();
    std::sort(touchedPages.begin(), touchedPages.end());
    touchedPages.erase(std::unique(touchedPages.begin(), touchedPages.end()), touchedPages.end());
  }
  if (touchedPages.empty()) {
    return;
  }

  std::vector<MemoryRegions::Region> regions;

  uint64_t currentSize = m_PageSize;
  char* currentPage = static_cast<char*>(touchedPages[0]);
  if (currentPage < baseAddress) {
    currentSize -= (baseAddress - currentPage);
    currentPage = baseAddress;
  }

  for (size_t i = 1; i < touchedPages.size(); ++i) {
    if (currentPage + currentSize == static_cast<char*>(touchedPages[i])) {
      currentSize += m_PageSize;
    } else {
      uint64_t offset = currentPage - baseAddress;
      regions.push_back({offset, currentSize, currentPage});
      currentPage = static_cast<char*>(touchedPages[i]);
      currentSize = m_PageSize;
    }
  }

  uint64_t offset = currentPage - baseAddress;
  if (offset + currentSize > allocationInfo.MapSize) {
    currentSize = allocationInfo.MapSize - offset;
  }
  regions.push_back({offset, currentSize, currentPage});
  if (allocationInfo.ShadowMemory) {
    for (const auto& region : regions) {
      std::memcpy(static_cast<char*>(allocationInfo.MappedData) + region.Offset, region.Data,
                  region.Size);
    }
  }
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
