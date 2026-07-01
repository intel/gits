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

} // namespace vulkan
} // namespace gits
