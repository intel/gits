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

} // namespace vulkan
} // namespace gits
