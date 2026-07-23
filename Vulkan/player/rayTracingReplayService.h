// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"

namespace gits {
namespace vulkan {

// Takes care of applying capture/replay handles to ray tracing pipelines
// or patches Shader Binding Tables depending on selected options.
class RayTracingReplayService {
public:
  void OnPreCreateRayTracingPipelines(vkCreateRayTracingPipelinesKHRCommand& command);
}; // class RayTracingReplayService

} // namespace vulkan
} // namespace gits
