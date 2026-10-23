// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "sparseBufferRelocation.h"

#include "log.h"
#include "vulkanHelpers.h"

#include <algorithm>
#include <ios>

namespace gits {
namespace vulkan {

namespace {

// Granularity of the address range probes (sparse block size)
constexpr VkDeviceSize PROBE_SIZE = 64ull * 1024;

VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

// Device addresses are canonical, buffer opaque capture addresses may be zero-extended.
VkDeviceAddress ToCanonicalAddress(uint64_t address) {
  constexpr uint32_t SHIFT = 64 - 48;
  return static_cast<uint64_t>(static_cast<int64_t>(address << SHIFT) >> SHIFT);
}

} // namespace

void SparseBufferRelocation::SetPhysicalDevice(VkPhysicalDevice physicalDevice) {
  m_PhysicalDevice = physicalDevice;
  m_ProbeMemoryTypeSelected = false;
  m_ProbeMemoryType = UINT32_MAX;
}

bool SparseBufferRelocation::PrepareBuffer(
    VkDevice device,
    VkBufferCreateInfo& createInfo,
    const std::function<uint64_t(uint64_t)>& toMemoryCaptureAddress) {
  m_Pending = {};
  if ((createInfo.flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT) == 0) {
    return false;
  }
  auto* addressInfo = const_cast<VkBufferOpaqueCaptureAddressCreateInfo*>(
      static_cast<const VkBufferOpaqueCaptureAddressCreateInfo*>(getPNextStructure(
          createInfo.pNext, VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO)));
  if (addressInfo == nullptr || addressInfo->opaqueCaptureAddress == 0 ||
      IsRangeAvailable(device, toMemoryCaptureAddress(addressInfo->opaqueCaptureAddress),
                       createInfo.size)) {
    return false;
  }

  const VkDeviceAddress recorded = ToCanonicalAddress(addressInfo->opaqueCaptureAddress);
  LOG_WARNING << "Portability: the recorded address range 0x" << std::hex << recorded << "-0x"
              << recorded + createInfo.size << std::dec
              << " of a sparse buffer is not available on the replay device, the buffer is "
                 "created at another address.";
  addressInfo->opaqueCaptureAddress = 0;
  createInfo.flags &= ~VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
  if (createInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
    m_Pending.Recorded = recorded;
    m_Pending.Size = createInfo.size;
  }
  return true;
}

void SparseBufferRelocation::AddBuffer(VkDevice device, VkBuffer buffer) {
  Relocation relocation = m_Pending;
  m_Pending = {};
  VkDeviceLevelDispatchTable* dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
  if (relocation.Size == 0 || buffer == VK_NULL_HANDLE || dispatchTable == nullptr) {
    return;
  }
  auto vkGetBufferDeviceAddressUnified = dispatchTable->vkGetBufferDeviceAddress
                                             ? dispatchTable->vkGetBufferDeviceAddress
                                             : dispatchTable->vkGetBufferDeviceAddressKHR;
  if (vkGetBufferDeviceAddressUnified == nullptr) {
    return;
  }
  VkBufferDeviceAddressInfo addressInfo{};
  addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  addressInfo.buffer = buffer;
  relocation.Buffer = buffer;
  relocation.Replayed = vkGetBufferDeviceAddressUnified(device, &addressInfo);
  LOG_INFO << "Portability: sparse buffer moved from 0x" << std::hex << relocation.Recorded
           << " to 0x" << relocation.Replayed << std::dec
           << ", its shader binding table addresses are translated.";
  m_Relocations.push_back(relocation);
}

void SparseBufferRelocation::RemoveBuffer(VkBuffer buffer) {
  m_Relocations.erase(std::remove_if(m_Relocations.begin(), m_Relocations.end(),
                                     [buffer](const Relocation& relocation) {
                                       return relocation.Buffer == buffer;
                                     }),
                      m_Relocations.end());
}

void SparseBufferRelocation::TranslateRegion(VkStridedDeviceAddressRegionKHR* region) {
  if (region == nullptr || region->deviceAddress == 0) {
    return;
  }
  for (const Relocation& relocation : m_Relocations) {
    if (region->deviceAddress >= relocation.Recorded &&
        region->deviceAddress < relocation.Recorded + relocation.Size) {
      region->deviceAddress = region->deviceAddress - relocation.Recorded + relocation.Replayed;
      ++m_TranslatedRegionCount;
      return;
    }
  }
}

void SparseBufferRelocation::DestroyDevice() {
  if (m_TranslatedRegionCount != 0) {
    LOG_INFO << "Portability: translated " << m_TranslatedRegionCount
             << " shader binding table region(s) of relocated sparse buffers.";
  }
  m_Relocations.clear();
  m_Pending = {};
  m_TranslatedRegionCount = 0;
  m_ProbeMemoryTypeSelected = false;
  m_ProbeMemoryType = UINT32_MAX;
}

bool SparseBufferRelocation::IsRangeAvailable(VkDevice device,
                                              uint64_t memoryCaptureAddress,
                                              VkDeviceSize size) {
  // The driver's address space is contiguous, so the first and the last page tell
  // whether the whole range lies inside it.
  const uint64_t last =
      memoryCaptureAddress + AlignUp(std::max(size, PROBE_SIZE), PROBE_SIZE) - PROBE_SIZE;
  return IsAddressAvailable(device, memoryCaptureAddress) &&
         (last == memoryCaptureAddress || IsAddressAvailable(device, last));
}

bool SparseBufferRelocation::IsAddressAvailable(VkDevice device, uint64_t memoryCaptureAddress) {
  VkDeviceLevelDispatchTable* dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
  const uint32_t memoryType = ProbeMemoryType(device);
  if (dispatchTable == nullptr || memoryType == UINT32_MAX) {
    // Can't tell, keep the recorded address.
    return true;
  }

  VkMemoryOpaqueCaptureAddressAllocateInfo addressInfo{};
  addressInfo.sType = VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO;
  addressInfo.opaqueCaptureAddress = memoryCaptureAddress;
  VkMemoryAllocateFlagsInfo flagsInfo{};
  flagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
  flagsInfo.pNext = &addressInfo;
  flagsInfo.flags =
      VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.pNext = &flagsInfo;
  allocateInfo.allocationSize = PROBE_SIZE;
  allocateInfo.memoryTypeIndex = memoryType;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  if (dispatchTable->vkAllocateMemory(device, &allocateInfo, nullptr, &memory) != VK_SUCCESS) {
    return false;
  }
  dispatchTable->vkFreeMemory(device, memory, nullptr);
  return true;
}

uint32_t SparseBufferRelocation::ProbeMemoryType(VkDevice device) {
  if (m_ProbeMemoryTypeSelected) {
    return m_ProbeMemoryType;
  }
  m_ProbeMemoryTypeSelected = true;
  VkDeviceLevelDispatchTable* dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
  VkInstanceLevelDispatchTable* instanceDispatchTable =
      m_DispatchTablesHolder.GetInstanceDispatchTable(m_PhysicalDevice);
  if (dispatchTable == nullptr || instanceDispatchTable == nullptr) {
    return m_ProbeMemoryType;
  }

  // Memory types accepted by buffers with device addresses
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = PROBE_SIZE;
  bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VkBuffer buffer = VK_NULL_HANDLE;
  if (dispatchTable->vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    return m_ProbeMemoryType;
  }
  VkMemoryRequirements requirements{};
  dispatchTable->vkGetBufferMemoryRequirements(device, buffer, &requirements);
  dispatchTable->vkDestroyBuffer(device, buffer, nullptr);

  VkPhysicalDeviceMemoryProperties memoryProperties{};
  instanceDispatchTable->vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);
  for (uint32_t t = 0; t < memoryProperties.memoryTypeCount; ++t) {
    if ((requirements.memoryTypeBits & (1u << t)) &&
        (memoryProperties.memoryTypes[t].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
      m_ProbeMemoryType = t;
      break;
    }
  }
  if (m_ProbeMemoryType == UINT32_MAX) {
    LOG_WARNING << "Portability: no memory type to test sparse buffer address ranges with, "
                   "sparse buffers keep their recorded addresses.";
  }
  return m_ProbeMemoryType;
}

} // namespace vulkan
} // namespace gits
