// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <bit>

namespace gits {
namespace vulkan {

inline const void* getPNextStructure(const void* pNext, VkStructureType structureType) {
  const VkBaseInStructure* pNextPtr = (const VkBaseInStructure*)pNext;
  while (pNextPtr != nullptr) {
    if (pNextPtr->sType == structureType) {
      return pNextPtr;
    }
    pNextPtr = (const VkBaseInStructure*)pNextPtr->pNext;
  }
  return nullptr;
}

// Returns the number of physical devices of a logical device created with the given info.
inline uint32_t GetDeviceGroupSize(const VkDeviceCreateInfo& createInfo) {
  const auto* groupInfo = static_cast<const VkDeviceGroupDeviceCreateInfo*>(
      getPNextStructure(createInfo.pNext, VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO));
  return groupInfo && groupInfo->physicalDeviceCount > 1 ? groupInfo->physicalDeviceCount : 1;
}

// Returns true if memory allocated with the given info has multiple instances, which cannot be
// mapped. Without an explicit device mask, the memory has an instance on every physical device of
// the logical device.
inline bool HasMultipleInstances(const VkMemoryAllocateInfo& allocateInfo,
                                 const VkPhysicalDeviceMemoryProperties& memoryProperties,
                                 uint32_t deviceGroupSize) {
  const VkMemoryType& memoryType = memoryProperties.memoryTypes[allocateInfo.memoryTypeIndex];
  if (!(memoryProperties.memoryHeaps[memoryType.heapIndex].flags &
        VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)) {
    return false;
  }
  const auto* flagsInfo = static_cast<const VkMemoryAllocateFlagsInfo*>(
      getPNextStructure(allocateInfo.pNext, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO));
  if (flagsInfo && (flagsInfo->flags & VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT)) {
    return std::popcount(flagsInfo->deviceMask) > 1;
  }
  return deviceGroupSize > 1;
}

inline bool isBitSet(VkFlags64 flags, VkFlags64 bit) {
  return (flags & bit) == bit;
}

inline bool IsImageDescriptorType(const VkWriteDescriptorSet& s) {
  auto type = s.descriptorType;
  return (type == VK_DESCRIPTOR_TYPE_SAMPLER || type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
          type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
          type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) &&
         s.pImageInfo;
}

inline bool IsBufferDescriptorType(const VkWriteDescriptorSet& s) {
  auto type = s.descriptorType;
  return (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
          type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
          type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) &&
         s.pBufferInfo;
}

inline bool IsTexelBufferDescriptorType(const VkWriteDescriptorSet& s) {
  auto type = s.descriptorType;
  return (type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ||
          type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) &&
         s.pTexelBufferView;
}

inline bool IsAccelerationStructureDescriptorType(const VkWriteDescriptorSet& s) {
  auto type = s.descriptorType;
  auto* pASWrite = (VkWriteDescriptorSetAccelerationStructureKHR*)getPNextStructure(
      s.pNext, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR);
  return (type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) && pASWrite;
}

} // namespace vulkan
} // namespace gits
