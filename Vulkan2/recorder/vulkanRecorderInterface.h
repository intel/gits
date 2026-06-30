// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "dispatchTableAuto.h"

namespace gits {
namespace vulkan {

class IRecorderWrapper {
public:
  virtual ~IRecorderWrapper() = default;
  virtual void LoadGlobalLevelFunctions(PFN_vkGetInstanceProcAddr getProcAddr) = 0;
  virtual void LoadInstanceLevelFunctions(PFN_vkGetInstanceProcAddr getProcAddr,
                                          VkInstance instance) = 0;
  virtual void LoadDeviceLevelFunctions(PFN_vkGetDeviceProcAddr getProcAddr, VkDevice device) = 0;
  virtual void LoadDeviceLevelFunctions(void* dispatchKey, VkDevice device) = 0;
  virtual PFN_vkVoidFunction GetFunctionWrapper(const char* name) = 0;
};

} // namespace vulkan
} // namespace gits

typedef gits::vulkan::IRecorderWrapper*(STDCALL* FGITSRecorderVulkan2)();

extern "C" {
gits::vulkan::IRecorderWrapper* STDCALL GITSRecorderVulkan2() VISIBLE;
}
