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

void CollectHandleKeys(std::vector<GITSKey>& keys, const VkWriteDescriptorSet& s);
void UpdateHandle(CaptureManager& manager, PointerArgument<VkWriteDescriptorSet>& arg);
void UpdateHandle(CaptureManager& manager, ArrayArgument<VkWriteDescriptorSet>& arg);

void CollectHandleKeys(std::vector<GITSKey>& keys, const VkPushDescriptorSetInfo& s);
void UpdateHandle(CaptureManager& manager, PointerArgument<VkPushDescriptorSetInfo>& arg);
void UpdateHandle(CaptureManager& manager, ArrayArgument<VkPushDescriptorSetInfo>& arg);

void CollectHandleKeys(std::vector<GITSKey>& keys, const VkImageCreateInfo& s);
void UpdateHandle(CaptureManager& manager, PointerArgument<VkImageCreateInfo>& arg);
void UpdateHandle(CaptureManager& manager, ArrayArgument<VkImageCreateInfo>& arg);

void CollectHandleKeys(std::vector<GITSKey>& keys, const VkRayTracingPipelineCreateInfoKHR& s);
void UpdateHandle(CaptureManager& manager, PointerArgument<VkRayTracingPipelineCreateInfoKHR>& arg);
void UpdateHandle(CaptureManager& manager, ArrayArgument<VkRayTracingPipelineCreateInfoKHR>& arg);

// Hand written so that an array of infos keeps every [src, dst] pair at AsBuildSrcKeyIndex(i) -
// the generated per element walk would let a pNext micromap handle shift the following pairs.
void CollectHandleKeys(std::vector<GITSKey>& keys,
                       const VkAccelerationStructureBuildGeometryInfoKHR& s);
void UpdateHandle(CaptureManager& manager,
                  PointerArgument<VkAccelerationStructureBuildGeometryInfoKHR>& arg);
void UpdateHandle(CaptureManager& manager,
                  ArrayArgument<VkAccelerationStructureBuildGeometryInfoKHR>& arg);

void UpdateOutputHandle(CaptureManager& manager,
                        ArrayArgument<VkPhysicalDeviceGroupProperties>& arg);

} // namespace vulkan
} // namespace gits
