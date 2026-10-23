// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructureScratch.h"

#include "log.h"

#include <algorithm>

namespace gits {
namespace vulkan {

namespace {

constexpr VkDeviceSize MIN_CHUNK_SIZE = 64ull * 1024 * 1024;

VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

} // namespace

void AccelerationStructureScratch::SetPhysicalDevice(VkPhysicalDevice physicalDevice) {
  m_PhysicalDevice = physicalDevice;
  VkInstanceLevelDispatchTable* dispatchTable =
      m_DispatchTablesHolder.GetInstanceDispatchTable(physicalDevice);
  if (dispatchTable == nullptr) {
    return;
  }
  auto vkGetPhysicalDeviceProperties2Unified =
      dispatchTable->vkGetPhysicalDeviceProperties2
          ? dispatchTable->vkGetPhysicalDeviceProperties2
          : dispatchTable->vkGetPhysicalDeviceProperties2KHR;
  if (vkGetPhysicalDeviceProperties2Unified == nullptr) {
    return;
  }
  VkPhysicalDeviceAccelerationStructurePropertiesKHR asProperties{};
  asProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
  VkPhysicalDeviceProperties2 properties{};
  properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  properties.pNext = &asProperties;
  vkGetPhysicalDeviceProperties2Unified(physicalDevice, &properties);
  if (asProperties.minAccelerationStructureScratchOffsetAlignment != 0) {
    m_Alignment = std::max<VkDeviceSize>(
        m_Alignment, asProperties.minAccelerationStructureScratchOffsetAlignment);
  }
}

void AccelerationStructureScratch::ReplaceScratch(
    VkDevice device,
    VkCommandBuffer commandBuffer,
    uint32_t infoCount,
    VkAccelerationStructureBuildGeometryInfoKHR* infos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* rangeInfos) {
  VkDeviceLevelDispatchTable* dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
  if (dispatchTable == nullptr ||
      dispatchTable->vkGetAccelerationStructureBuildSizesKHR == nullptr || infos == nullptr ||
      rangeInfos == nullptr) {
    return;
  }

  Arena& arena = m_Arenas[commandBuffer];
  arena.Device = device;
  std::vector<uint32_t> primitiveCounts;
  for (uint32_t i = 0; i < infoCount; ++i) {
    VkAccelerationStructureBuildGeometryInfoKHR& info = infos[i];
    primitiveCounts.assign(info.geometryCount, 0);
    for (uint32_t g = 0; g < info.geometryCount; ++g) {
      primitiveCounts[g] = rangeInfos[i][g].primitiveCount;
    }
    VkAccelerationStructureBuildSizesInfoKHR sizes{};
    sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    dispatchTable->vkGetAccelerationStructureBuildSizesKHR(
        device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &info, primitiveCounts.data(),
        &sizes);
    const VkDeviceSize size = info.mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR
                                  ? sizes.updateScratchSize
                                  : sizes.buildScratchSize;
    if (size == 0) {
      continue;
    }
    VkDeviceAddress address = 0;
    if (!Allocate(device, arena, size, address)) {
      continue;
    }
    info.scratchData.deviceAddress = address;
    if (m_ReplacedBuildCount++ == 0) {
      LOG_INFO << "Portability: acceleration structure builds use scratch memory sized for the "
                  "replay driver (first build: "
               << size << " bytes).";
    }
  }
}

bool AccelerationStructureScratch::Allocate(VkDevice device,
                                            Arena& arena,
                                            VkDeviceSize size,
                                            VkDeviceAddress& address) {
  for (Chunk& chunk : arena.Chunks) {
    const VkDeviceSize offset = AlignUp(chunk.Used, m_Alignment);
    if (offset + size <= chunk.Size) {
      chunk.Used = offset + size;
      address = chunk.Address + offset;
      return true;
    }
  }
  Chunk chunk;
  if (!CreateChunk(device, std::max(MIN_CHUNK_SIZE, AlignUp(size, m_Alignment)), chunk)) {
    return false;
  }
  chunk.Used = size;
  address = chunk.Address;
  arena.Chunks.push_back(chunk);
  return true;
}

bool AccelerationStructureScratch::CreateChunk(VkDevice device, VkDeviceSize size, Chunk& chunk) {
  VkDeviceLevelDispatchTable* dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
  VkInstanceLevelDispatchTable* instanceDispatchTable =
      m_DispatchTablesHolder.GetInstanceDispatchTable(m_PhysicalDevice);
  if (dispatchTable == nullptr || instanceDispatchTable == nullptr) {
    return false;
  }
  auto vkGetBufferDeviceAddressUnified = dispatchTable->vkGetBufferDeviceAddress
                                             ? dispatchTable->vkGetBufferDeviceAddress
                                             : dispatchTable->vkGetBufferDeviceAddressKHR;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (dispatchTable->vkCreateBuffer(device, &bufferInfo, nullptr, &chunk.Buffer) != VK_SUCCESS) {
    return false;
  }

  VkMemoryRequirements requirements{};
  dispatchTable->vkGetBufferMemoryRequirements(device, chunk.Buffer, &requirements);
  VkPhysicalDeviceMemoryProperties memoryProperties{};
  instanceDispatchTable->vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);
  uint32_t memoryType = UINT32_MAX;
  for (uint32_t t = 0; t < memoryProperties.memoryTypeCount && memoryType == UINT32_MAX; ++t) {
    if ((requirements.memoryTypeBits & (1u << t)) &&
        (memoryProperties.memoryTypes[t].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
      memoryType = t;
    }
  }

  VkMemoryAllocateFlagsInfo flagsInfo{};
  flagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
  flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.pNext = &flagsInfo;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = memoryType;
  if (memoryType == UINT32_MAX ||
      dispatchTable->vkAllocateMemory(device, &allocateInfo, nullptr, &chunk.Memory) !=
          VK_SUCCESS ||
      dispatchTable->vkBindBufferMemory(device, chunk.Buffer, chunk.Memory, 0) != VK_SUCCESS) {
    LOG_WARNING << "Portability: allocating " << size
                << " bytes of acceleration structure scratch memory failed, the build keeps its "
                   "recorded scratch.";
    if (chunk.Memory != VK_NULL_HANDLE) {
      dispatchTable->vkFreeMemory(device, chunk.Memory, nullptr);
    }
    dispatchTable->vkDestroyBuffer(device, chunk.Buffer, nullptr);
    chunk = {};
    return false;
  }

  VkBufferDeviceAddressInfo addressInfo{};
  addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  addressInfo.buffer = chunk.Buffer;
  chunk.Address = vkGetBufferDeviceAddressUnified(device, &addressInfo);
  chunk.Size = size;
  m_TotalChunkSize += size;
  LOG_DEBUG << "Portability: acceleration structure scratch memory grown to " << m_TotalChunkSize
            << " bytes.";
  return true;
}

void AccelerationStructureScratch::Release(Arena& arena) {
  VkDeviceLevelDispatchTable* dispatchTable =
      m_DispatchTablesHolder.GetDeviceDispatchTable(arena.Device);
  for (Chunk& chunk : arena.Chunks) {
    if (dispatchTable != nullptr) {
      dispatchTable->vkDestroyBuffer(arena.Device, chunk.Buffer, nullptr);
      dispatchTable->vkFreeMemory(arena.Device, chunk.Memory, nullptr);
    }
    m_TotalChunkSize -= chunk.Size;
  }
  arena.Chunks.clear();
}

void AccelerationStructureScratch::AddCommandBuffers(VkCommandPool commandPool,
                                                     const VkCommandBuffer* commandBuffers,
                                                     uint32_t count) {
  if (commandBuffers == nullptr) {
    return;
  }
  auto& poolCommandBuffers = m_PoolCommandBuffers[commandPool];
  poolCommandBuffers.insert(commandBuffers, commandBuffers + count);
}

void AccelerationStructureScratch::ResetCommandBuffer(VkCommandBuffer commandBuffer) {
  // The previous recording has completed execution, so its scratch regions can be reused.
  auto it = m_Arenas.find(commandBuffer);
  if (it != m_Arenas.end()) {
    for (Chunk& chunk : it->second.Chunks) {
      chunk.Used = 0;
    }
  }
}

void AccelerationStructureScratch::ResetCommandPool(VkCommandPool commandPool) {
  auto it = m_PoolCommandBuffers.find(commandPool);
  if (it != m_PoolCommandBuffers.end()) {
    for (VkCommandBuffer commandBuffer : it->second) {
      ResetCommandBuffer(commandBuffer);
    }
  }
}

void AccelerationStructureScratch::FreeCommandBuffers(VkCommandPool commandPool,
                                                      const VkCommandBuffer* commandBuffers,
                                                      uint32_t count) {
  if (commandBuffers == nullptr) {
    return;
  }
  auto poolIt = m_PoolCommandBuffers.find(commandPool);
  for (uint32_t i = 0; i < count; ++i) {
    auto it = m_Arenas.find(commandBuffers[i]);
    if (it != m_Arenas.end()) {
      Release(it->second);
      m_Arenas.erase(it);
    }
    if (poolIt != m_PoolCommandBuffers.end()) {
      poolIt->second.erase(commandBuffers[i]);
    }
  }
}

void AccelerationStructureScratch::DestroyCommandPool(VkCommandPool commandPool) {
  auto poolIt = m_PoolCommandBuffers.find(commandPool);
  if (poolIt == m_PoolCommandBuffers.end()) {
    return;
  }
  for (VkCommandBuffer commandBuffer : poolIt->second) {
    auto it = m_Arenas.find(commandBuffer);
    if (it != m_Arenas.end()) {
      Release(it->second);
      m_Arenas.erase(it);
    }
  }
  m_PoolCommandBuffers.erase(poolIt);
}

void AccelerationStructureScratch::DestroyDevice(VkDevice device) {
  for (auto it = m_Arenas.begin(); it != m_Arenas.end();) {
    if (it->second.Device == device) {
      Release(it->second);
      it = m_Arenas.erase(it);
    } else {
      ++it;
    }
  }
}

} // namespace vulkan
} // namespace gits
