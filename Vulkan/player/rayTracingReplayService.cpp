// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rayTracingReplayService.h"

namespace gits {
namespace vulkan {

void RayTracingReplayService::OnPreCreateRayTracingPipelines(
    vkCreateRayTracingPipelinesKHRCommand& command) {
  if (!command.m_pCreateInfos.CaptureReplayHandleSize ||
      command.m_pCreateInfos.CaptureReplayHandlesData.empty()) {
    return;
  }

  uint8_t* ptr = command.m_pCreateInfos.CaptureReplayHandlesData.data();

  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    auto& createInfo = command.m_pCreateInfos.Value[i];

    for (uint32_t g = 0; g < createInfo.groupCount; ++g) {
      auto& group = const_cast<VkRayTracingShaderGroupCreateInfoKHR&>(createInfo.pGroups[g]);
      group.pShaderGroupCaptureReplayHandle = ptr;
      ptr += command.m_pCreateInfos.CaptureReplayHandleSize;
    }
  }
}

} // namespace vulkan
} // namespace gits
