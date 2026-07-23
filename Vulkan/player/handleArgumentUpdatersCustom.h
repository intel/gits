// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "playerManager.h"
#include "handleMapService.h"
#include "arguments.h"

namespace gits {
namespace vulkan {

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkWriteDescriptorSet& s);
void UpdateHandle(PlayerManager& manager, PointerArgument<VkWriteDescriptorSet>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkWriteDescriptorSet>& arg);

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkPushDescriptorSetInfo& s);
void UpdateHandle(PlayerManager& manager, PointerArgument<VkPushDescriptorSetInfo>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkPushDescriptorSetInfo>& arg);

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkImageCreateInfo& s);
void UpdateHandle(PlayerManager& manager, PointerArgument<VkImageCreateInfo>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkImageCreateInfo>& arg);

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkRayTracingPipelineCreateInfoKHR& s);
void UpdateHandle(PlayerManager& manager, PointerArgument<VkRayTracingPipelineCreateInfoKHR>& arg);
void UpdateHandle(PlayerManager& manager, ArrayArgument<VkRayTracingPipelineCreateInfoKHR>& arg);

void UpdateOutputHandle(PlayerManager& manager, HandleArrayOutputArgument<VkPhysicalDevice>& arg);
void UpdateOutputHandle(PlayerManager& manager,
                        ArrayArgument<VkPhysicalDeviceGroupProperties>& arg);

} // namespace vulkan
} // namespace gits
