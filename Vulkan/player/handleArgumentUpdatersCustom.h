// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "handleMapService.h"
#include "handleResolver.h"
#include "arguments.h"

namespace gits {
namespace vulkan {

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkWriteDescriptorSet& s,
                       const HandleResolver& resolver);
void UpdateHandle(PointerArgument<VkWriteDescriptorSet>& arg, const HandleResolver& resolver);
void UpdateHandle(ArrayArgument<VkWriteDescriptorSet>& arg, const HandleResolver& resolver);

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkPushDescriptorSetInfo& s,
                       const HandleResolver& resolver);
void UpdateHandle(PointerArgument<VkPushDescriptorSetInfo>& arg, const HandleResolver& resolver);
void UpdateHandle(ArrayArgument<VkPushDescriptorSetInfo>& arg, const HandleResolver& resolver);

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkImageCreateInfo& s,
                       const HandleResolver& resolver);
void UpdateHandle(PointerArgument<VkImageCreateInfo>& arg, const HandleResolver& resolver);
void UpdateHandle(ArrayArgument<VkImageCreateInfo>& arg, const HandleResolver& resolver);

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkBufferCreateInfo& s,
                       const HandleResolver& resolver);
void UpdateHandle(PointerArgument<VkBufferCreateInfo>& arg, const HandleResolver& resolver);
void UpdateHandle(ArrayArgument<VkBufferCreateInfo>& arg, const HandleResolver& resolver);

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkImageViewCreateInfo& s,
                       const HandleResolver& resolver);
void UpdateHandle(PointerArgument<VkImageViewCreateInfo>& arg, const HandleResolver& resolver);
void UpdateHandle(ArrayArgument<VkImageViewCreateInfo>& arg, const HandleResolver& resolver);

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkSamplerCreateInfo& s,
                       const HandleResolver& resolver);
void UpdateHandle(PointerArgument<VkSamplerCreateInfo>& arg, const HandleResolver& resolver);
void UpdateHandle(ArrayArgument<VkSamplerCreateInfo>& arg, const HandleResolver& resolver);

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkAccelerationStructureCreateInfoKHR& s,
                       const HandleResolver& resolver);
void UpdateHandle(PointerArgument<VkAccelerationStructureCreateInfoKHR>& arg,
                  const HandleResolver& resolver);
void UpdateHandle(ArrayArgument<VkAccelerationStructureCreateInfoKHR>& arg,
                  const HandleResolver& resolver);

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkAccelerationStructureCreateInfo2KHR& s,
                       const HandleResolver& resolver);
void UpdateHandle(PointerArgument<VkAccelerationStructureCreateInfo2KHR>& arg,
                  const HandleResolver& resolver);
void UpdateHandle(ArrayArgument<VkAccelerationStructureCreateInfo2KHR>& arg,
                  const HandleResolver& resolver);

void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkRayTracingPipelineCreateInfoKHR& s,
                       const HandleResolver& resolver);
void UpdateHandle(PointerArgument<VkRayTracingPipelineCreateInfoKHR>& arg,
                  const HandleResolver& resolver);
void UpdateHandle(ArrayArgument<VkRayTracingPipelineCreateInfoKHR>& arg,
                  const HandleResolver& resolver);

// Mirrors the hand written recorder layout - every [src, dst] pair of an array of infos comes
// before the variable length pNext payload.
void ResolveHandleKeys(const std::vector<GITSKey>& keys,
                       uint32_t& idx,
                       std::vector<uint64_t>& handleData,
                       VkAccelerationStructureBuildGeometryInfoKHR& s,
                       const HandleResolver& resolver);
void UpdateHandle(PointerArgument<VkAccelerationStructureBuildGeometryInfoKHR>& arg,
                  const HandleResolver& resolver);
void UpdateHandle(ArrayArgument<VkAccelerationStructureBuildGeometryInfoKHR>& arg,
                  const HandleResolver& resolver);

void UpdateOutputHandle(HandleArrayOutputArgument<VkPhysicalDevice>& arg);
void UpdateOutputHandle(ArrayArgument<VkPhysicalDeviceGroupProperties>& arg);

} // namespace vulkan
} // namespace gits
