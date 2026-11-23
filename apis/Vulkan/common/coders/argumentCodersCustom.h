// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"
#include "argumentCoders.h"

namespace gits {
namespace vulkan {

// VkWriteDescriptorSet - custom coder needed because pImageInfo/pBufferInfo/pTexelBufferView
// are a union selected by descriptorType; the auto-generated coder cannot express this.
uint32_t GetSize(const VkWriteDescriptorSet* src, uint32_t count);
void Encode(const VkWriteDescriptorSet* src, uint32_t count, char* dst, uint32_t& offset);
void Decode(const VkWriteDescriptorSet* dst, uint32_t count, char* src, uint32_t& offset);

// PointerArgument overloads for VkWriteDescriptorSet
uint32_t GetSize(const PointerArgument<VkWriteDescriptorSet>& arg);
void Encode(char* dst, uint32_t& offset, const PointerArgument<VkWriteDescriptorSet>& arg);
void Decode(char* src, uint32_t& offset, PointerArgument<VkWriteDescriptorSet>& arg);

// ArrayArgument overloads for VkWriteDescriptorSet
uint32_t GetSize(const ArrayArgument<VkWriteDescriptorSet>& arg);
void Encode(char* dst, uint32_t& offset, const ArrayArgument<VkWriteDescriptorSet>& arg);
void Decode(char* src, uint32_t& offset, ArrayArgument<VkWriteDescriptorSet>& arg);

// DescriptorTemplateDataArgument - serializes the pData buffer for
// vkUpdateDescriptorSetWithTemplate[KHR] and vkCmdPushDescriptorSetWithTemplate[KHR].
// The Data vector is populated during capture (Pre layer) and contains the
// typed descriptor entries built from the template's create info.
uint32_t GetSize(const DescriptorTemplateDataArgument& arg);
void Encode(char* dst, uint32_t& offset, const DescriptorTemplateDataArgument& arg);
void Decode(char* src, uint32_t& offset, DescriptorTemplateDataArgument& arg);

uint32_t GetSize(const VkBufferCreateInfo* src, uint32_t count);
void Encode(const VkBufferCreateInfo* src, uint32_t count, char* dst, uint32_t& offset);
void Decode(const VkBufferCreateInfo* dst, uint32_t count, char* src, uint32_t& offset);
uint32_t GetSize(const PointerArgument<VkBufferCreateInfo>& arg);
void Encode(char* dst, uint32_t& offset, const PointerArgument<VkBufferCreateInfo>& arg);
void Decode(char* src, uint32_t& offset, PointerArgument<VkBufferCreateInfo>& arg);
uint32_t GetSize(const ArrayArgument<VkBufferCreateInfo>& arg);
void Encode(char* dst, uint32_t& offset, const ArrayArgument<VkBufferCreateInfo>& arg);
void Decode(char* src, uint32_t& offset, ArrayArgument<VkBufferCreateInfo>& arg);

// VkImageCreateInfo - custom coder needed because queueFamiltyIndexCount
// and pQueueFamilyIndices are only valid for VK_SHARING_MODE_CONCURRENT
uint32_t GetSize(const VkImageCreateInfo* src, uint32_t count);
void Encode(const VkImageCreateInfo* src, uint32_t count, char* dst, uint32_t& offset);
void Decode(const VkImageCreateInfo* dst, uint32_t count, char* src, uint32_t& offset);

// PointerArgument overloads for VkImageCreateInfo
uint32_t GetSize(const PointerArgument<VkImageCreateInfo>& arg);
void Encode(char* dst, uint32_t& offset, const PointerArgument<VkImageCreateInfo>& arg);
void Decode(char* src, uint32_t& offset, PointerArgument<VkImageCreateInfo>& arg);

// ArrayArgument overloads for VkImageCreateInfo
uint32_t GetSize(const ArrayArgument<VkImageCreateInfo>& arg);
void Encode(char* dst, uint32_t& offset, const ArrayArgument<VkImageCreateInfo>& arg);
void Decode(char* src, uint32_t& offset, ArrayArgument<VkImageCreateInfo>& arg);

uint32_t GetSize(const VkImageViewCreateInfo* src, uint32_t count);
void Encode(const VkImageViewCreateInfo* src, uint32_t count, char* dst, uint32_t& offset);
void Decode(const VkImageViewCreateInfo* dst, uint32_t count, char* src, uint32_t& offset);
uint32_t GetSize(const PointerArgument<VkImageViewCreateInfo>& arg);
void Encode(char* dst, uint32_t& offset, const PointerArgument<VkImageViewCreateInfo>& arg);
void Decode(char* src, uint32_t& offset, PointerArgument<VkImageViewCreateInfo>& arg);
uint32_t GetSize(const ArrayArgument<VkImageViewCreateInfo>& arg);
void Encode(char* dst, uint32_t& offset, const ArrayArgument<VkImageViewCreateInfo>& arg);
void Decode(char* src, uint32_t& offset, ArrayArgument<VkImageViewCreateInfo>& arg);

uint32_t GetSize(const VkSamplerCreateInfo* src, uint32_t count);
void Encode(const VkSamplerCreateInfo* src, uint32_t count, char* dst, uint32_t& offset);
void Decode(const VkSamplerCreateInfo* dst, uint32_t count, char* src, uint32_t& offset);
uint32_t GetSize(const PointerArgument<VkSamplerCreateInfo>& arg);
void Encode(char* dst, uint32_t& offset, const PointerArgument<VkSamplerCreateInfo>& arg);
void Decode(char* src, uint32_t& offset, PointerArgument<VkSamplerCreateInfo>& arg);
uint32_t GetSize(const ArrayArgument<VkSamplerCreateInfo>& arg);
void Encode(char* dst, uint32_t& offset, const ArrayArgument<VkSamplerCreateInfo>& arg);
void Decode(char* src, uint32_t& offset, ArrayArgument<VkSamplerCreateInfo>& arg);

uint32_t GetSize(const VkAccelerationStructureCreateInfoKHR* src, uint32_t count);
void Encode(const VkAccelerationStructureCreateInfoKHR* src,
            uint32_t count,
            char* dst,
            uint32_t& offset);
void Decode(const VkAccelerationStructureCreateInfoKHR* dst,
            uint32_t count,
            char* src,
            uint32_t& offset);
uint32_t GetSize(const PointerArgument<VkAccelerationStructureCreateInfoKHR>& arg);
void Encode(char* dst,
            uint32_t& offset,
            const PointerArgument<VkAccelerationStructureCreateInfoKHR>& arg);
void Decode(char* src,
            uint32_t& offset,
            PointerArgument<VkAccelerationStructureCreateInfoKHR>& arg);
uint32_t GetSize(const ArrayArgument<VkAccelerationStructureCreateInfoKHR>& arg);
void Encode(char* dst,
            uint32_t& offset,
            const ArrayArgument<VkAccelerationStructureCreateInfoKHR>& arg);
void Decode(char* src, uint32_t& offset, ArrayArgument<VkAccelerationStructureCreateInfoKHR>& arg);

uint32_t GetSize(const VkAccelerationStructureCreateInfo2KHR* src, uint32_t count);
void Encode(const VkAccelerationStructureCreateInfo2KHR* src,
            uint32_t count,
            char* dst,
            uint32_t& offset);
void Decode(const VkAccelerationStructureCreateInfo2KHR* dst,
            uint32_t count,
            char* src,
            uint32_t& offset);
uint32_t GetSize(const PointerArgument<VkAccelerationStructureCreateInfo2KHR>& arg);
void Encode(char* dst,
            uint32_t& offset,
            const PointerArgument<VkAccelerationStructureCreateInfo2KHR>& arg);
void Decode(char* src,
            uint32_t& offset,
            PointerArgument<VkAccelerationStructureCreateInfo2KHR>& arg);
uint32_t GetSize(const ArrayArgument<VkAccelerationStructureCreateInfo2KHR>& arg);
void Encode(char* dst,
            uint32_t& offset,
            const ArrayArgument<VkAccelerationStructureCreateInfo2KHR>& arg);
void Decode(char* src, uint32_t& offset, ArrayArgument<VkAccelerationStructureCreateInfo2KHR>& arg);

//////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t GetSize(const VkRayTracingShaderGroupCreateInfoKHR* src, uint32_t count);
void Encode(const VkRayTracingShaderGroupCreateInfoKHR* src,
            uint32_t count,
            char* dst,
            uint32_t& offset);
void Decode(const VkRayTracingShaderGroupCreateInfoKHR* dst,
            uint32_t count,
            char* src,
            uint32_t& offset);

uint32_t GetSize(const PointerArgument<VkRayTracingShaderGroupCreateInfoKHR>& arg);
void Encode(char* dst,
            uint32_t& offset,
            const PointerArgument<VkRayTracingShaderGroupCreateInfoKHR>& arg);
void Decode(char* src,
            uint32_t& offset,
            PointerArgument<VkRayTracingShaderGroupCreateInfoKHR>& arg);

uint32_t GetSize(const ArrayArgument<VkRayTracingShaderGroupCreateInfoKHR>& arg);
void Encode(char* dst,
            uint32_t& offset,
            const ArrayArgument<VkRayTracingShaderGroupCreateInfoKHR>& arg);
void Decode(char* src, uint32_t& offset, ArrayArgument<VkRayTracingShaderGroupCreateInfoKHR>& arg);

//////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t GetSize(const VkRayTracingPipelineCreateInfoKHR* src, uint32_t count);
void Encode(const VkRayTracingPipelineCreateInfoKHR* src,
            uint32_t count,
            char* dst,
            uint32_t& offset);
void Decode(const VkRayTracingPipelineCreateInfoKHR* dst,
            uint32_t count,
            char* src,
            uint32_t& offset);

uint32_t GetSize(const PointerArgument<VkRayTracingPipelineCreateInfoKHR>& arg);
void Encode(char* dst,
            uint32_t& offset,
            const PointerArgument<VkRayTracingPipelineCreateInfoKHR>& arg);
void Decode(char* src, uint32_t& offset, PointerArgument<VkRayTracingPipelineCreateInfoKHR>& arg);

uint32_t GetSize(const ArrayArgument<VkRayTracingPipelineCreateInfoKHR>& arg);
void Encode(char* dst,
            uint32_t& offset,
            const ArrayArgument<VkRayTracingPipelineCreateInfoKHR>& arg);
void Decode(char* src, uint32_t& offset, ArrayArgument<VkRayTracingPipelineCreateInfoKHR>& arg);

} // namespace vulkan
} // namespace gits
