// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "rtsr/shared/protocol/rtsrWireTypes.h"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace gits::vulkan {

struct VulkanStateDocument {
  std::string Json;
  rtsr::RTSR_HASH Hash{};
  bool ReplacementEligible{true};
  std::string Diagnostic;
};

bool HasDynamicState(const VkPipelineDynamicStateCreateInfo* info, VkDynamicState state);

VulkanStateDocument MakeGraphicsStateDocument(
    const VkGraphicsPipelineCreateInfo& createInfo,
    const std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO>& stages,
    bool allShadersResolved,
    bool stateSnapshotAvailable,
    GITSKey layoutKey,
    GITSKey renderPassKey);
VulkanStateDocument MakeComputeStateDocument(
    const VkComputePipelineCreateInfo& createInfo,
    const std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO>& stages,
    bool allShadersResolved,
    bool stateSnapshotAvailable,
    GITSKey layoutKey);

} // namespace gits::vulkan
