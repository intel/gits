// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader.h"

#include <vector>

namespace gits {
namespace Vulkan {

uint32_t getFormatBytesPerPixel(VkFormat format);
uint32_t getFormatNumBlocks(VkFormat format, VkExtent3D size);
uint32_t getFormatDataSize(VkFormat format, VkExtent3D size);
uint32_t getFormatAspectDataSize(VkFormat format, VkImageAspectFlagBits aspect, VkExtent3D size);
uint32_t getFormatChannelCount(VkFormat format);
VkImageAspectFlags getFormatAspectFlags(VkFormat format);
VkAccessFlags getLayoutAccessFlags(VkImageLayout layout);
uint32_t getIndexElementSize(VkIndexType indexType);
uint32_t getLayerSettingsElementSize(VkLayerSettingTypeEXT layerSettingsType, const void* value);
bool isFormatFloat(VkFormat format);
bool isFormatCompressed(VkFormat format);
const void* ignoreLoaderSpecificStructureTypes(const void* pNext);
bool isSamplerDescriptor(VkDescriptorType descriptorType);
bool isImageDescriptor(VkDescriptorType descriptorType);
bool isBufferDescriptor(VkDescriptorType descriptorType);
bool isTexelBufferDescriptor(VkDescriptorType descriptorType);
std::vector<uint32_t> getRayTracingArraySizes(
    uint32_t count, VkAccelerationStructureBuildGeometryInfoKHR const* pInfos);
VkCommandExecutionSideGITS getCommandExecutionSide(VkCommandBuffer commandBuffer);
inline bool isBitSet(VkFlags64 flags, VkFlags64 bit) {
  return (flags & bit) == bit;
}

const void* getPNextStructure(const void* pNext, VkStructureType structureType);
const void* getStructStoragePointer(const void* pNext, VkStructureType structureType);
bool isImagePresentable(const VkImageCreateInfo* pCreateInfo);

} // namespace Vulkan
} // namespace gits
