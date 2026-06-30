// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "captureManager.h"
#include "handleMapService.h"
#include "arguments.h"

namespace gits {
namespace vulkan {

bool IsImageDescriptorType(VkDescriptorType type);
bool IsBufferDescriptorType(VkDescriptorType type);
bool IsTexelBufferDescriptorType(VkDescriptorType type);

void CollectHandleKeys(std::vector<GITSKey>& keys, const VkWriteDescriptorSet& s);
void CollectHandleKeys(std::vector<GITSKey>& keys, const VkPushDescriptorSetInfo& s);

void UpdateOutputHandle(CaptureManager& manager,
                        ArrayArgument<VkPhysicalDeviceGroupProperties>& arg);

void UpdateHandle(CaptureManager& manager, PointerArgument<VkWriteDescriptorSet>& arg);
void UpdateHandle(CaptureManager& manager, ArrayArgument<VkWriteDescriptorSet>& arg);
void UpdateHandle(CaptureManager& manager, PointerArgument<VkPushDescriptorSetInfo>& arg);
void UpdateHandle(CaptureManager& manager, ArrayArgument<VkPushDescriptorSetInfo>& arg);

} // namespace vulkan
} // namespace gits
