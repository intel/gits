// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "portabilityHelpers.h"

namespace gits {
namespace vulkan {
namespace portability {

uint32_t FindDeviceLocalMemoryType(DispatchTablesHolder& dispatchTablesHolder,
                                   VkPhysicalDevice physicalDevice,
                                   uint32_t memoryTypeBits) {
  VkInstanceLevelDispatchTable* dispatchTable =
      dispatchTablesHolder.GetInstanceDispatchTable(physicalDevice);
  if (dispatchTable == nullptr) {
    return UINT32_MAX;
  }
  VkPhysicalDeviceMemoryProperties memoryProperties{};
  dispatchTable->vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
  for (uint32_t t = 0; t < memoryProperties.memoryTypeCount; ++t) {
    if ((memoryTypeBits & (1u << t)) &&
        (memoryProperties.memoryTypes[t].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
      return t;
    }
  }
  return UINT32_MAX;
}

bool CreateDeviceLocalBuffer(DispatchTablesHolder& dispatchTablesHolder,
                             VkPhysicalDevice physicalDevice,
                             VkDevice device,
                             VkDeviceSize size,
                             VkBufferUsageFlags usage,
                             DeviceLocalBuffer& buffer) {
  buffer = {};
  VkDeviceLevelDispatchTable* dispatchTable = dispatchTablesHolder.GetDeviceDispatchTable(device);
  if (dispatchTable == nullptr) {
    return false;
  }
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (dispatchTable->vkCreateBuffer(device, &bufferInfo, nullptr, &buffer.Buffer) != VK_SUCCESS) {
    buffer = {};
    return false;
  }

  VkMemoryRequirements requirements{};
  dispatchTable->vkGetBufferMemoryRequirements(device, buffer.Buffer, &requirements);
  const uint32_t memoryType =
      FindDeviceLocalMemoryType(dispatchTablesHolder, physicalDevice, requirements.memoryTypeBits);

  VkMemoryAllocateFlagsInfo flagsInfo{};
  flagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
  flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.pNext =
      (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0 ? &flagsInfo : nullptr;
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = memoryType;
  if (memoryType == UINT32_MAX ||
      dispatchTable->vkAllocateMemory(device, &allocateInfo, nullptr, &buffer.Memory) !=
          VK_SUCCESS ||
      dispatchTable->vkBindBufferMemory(device, buffer.Buffer, buffer.Memory, 0) != VK_SUCCESS) {
    DestroyDeviceLocalBuffer(dispatchTablesHolder, device, buffer);
    return false;
  }
  buffer.AllocationSize = requirements.size;
  return true;
}

void DestroyDeviceLocalBuffer(DispatchTablesHolder& dispatchTablesHolder,
                              VkDevice device,
                              DeviceLocalBuffer& buffer) {
  VkDeviceLevelDispatchTable* dispatchTable = dispatchTablesHolder.GetDeviceDispatchTable(device);
  if (dispatchTable != nullptr) {
    if (buffer.Buffer != VK_NULL_HANDLE) {
      dispatchTable->vkDestroyBuffer(device, buffer.Buffer, nullptr);
    }
    if (buffer.Memory != VK_NULL_HANDLE) {
      dispatchTable->vkFreeMemory(device, buffer.Memory, nullptr);
    }
  }
  buffer = {};
}

PFN_vkGetBufferDeviceAddress GetBufferDeviceAddressFunction(
    const VkDeviceLevelDispatchTable& dispatchTable) {
  return dispatchTable.vkGetBufferDeviceAddress ? dispatchTable.vkGetBufferDeviceAddress
                                                : dispatchTable.vkGetBufferDeviceAddressKHR;
}

} // namespace portability
} // namespace vulkan
} // namespace gits
