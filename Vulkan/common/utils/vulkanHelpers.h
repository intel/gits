// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

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
