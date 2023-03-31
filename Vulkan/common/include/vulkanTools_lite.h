// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader.h"

namespace gits {
namespace Vulkan {

uint32_t getFormatBytesPerPixel(VkFormat format);
uint32_t getFormatNumBlocks(VkFormat format, VkExtent3D size);
uint32_t getFormatDataSize(VkFormat format, VkExtent3D size);
uint32_t getFormatAspectDataSize(VkFormat format, VkImageAspectFlagBits aspect, VkExtent3D size);
uint32_t getFormatChannelCount(VkFormat format);
VkImageAspectFlags getFormatAspectFlags(VkFormat format);
VkAccessFlags getLayoutAccessFlags(VkImageLayout layout);
bool isFormatFloat(VkFormat format);
bool isFormatCompressed(VkFormat format);
const void* ignoreLoaderSpecificStructureTypes(const void* pNext);
bool isImageDescriptor(VkDescriptorType descriptorType);
bool isBufferDescriptor(VkDescriptorType descriptorType);
bool isTexelBufferDescriptor(VkDescriptorType descriptorType);
inline bool isBitSet(VkFlags64 flags, VkFlags64 bit) {
  return (flags & bit) == bit;
}

// Helper function for calculating size of array to write in one chunk of CCode
size_t CalculateChunkSize(size_t arraySize, size_t maxChunkSize, size_t arrayIterator);

#ifndef BUILD_FOR_CCODE
const void* getPNextStructure(const void* pNext, VkStructureType structureType);
bool isImagePresentable(const VkImageCreateInfo* pCreateInfo);
#endif

} // namespace Vulkan
} // namespace gits
