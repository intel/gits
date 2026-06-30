// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanRecorderInterface.h"

namespace gits {
namespace vulkan {

class RecorderWrapper : public IRecorderWrapper {
public:
  void LoadGlobalLevelFunctions(PFN_vkGetInstanceProcAddr getProcAddr) override;
  void LoadInstanceLevelFunctions(PFN_vkGetInstanceProcAddr getProcAddr,
                                  VkInstance instance) override;
  void LoadDeviceLevelFunctions(PFN_vkGetDeviceProcAddr getProcAddr, VkDevice device) override;
  void LoadDeviceLevelFunctions(void* dispatchKey, VkDevice device) override;
  PFN_vkVoidFunction GetFunctionWrapper(const char* name) override;
};

} // namespace vulkan
} // namespace gits
