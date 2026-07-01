// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanRecorder.h"
#include "captureManager.h"
#include "log.h"

namespace gits {
namespace vulkan {

void RecorderWrapper::LoadGlobalLevelFunctions(PFN_vkGetInstanceProcAddr getProcAddr) {
  CaptureManager::Get().LoadGlobalFunctions(getProcAddr);
}

void RecorderWrapper::LoadInstanceLevelFunctions(PFN_vkGetInstanceProcAddr getProcAddr,
                                                 VkInstance instance) {
  CaptureManager::Get().LoadInstanceFunctions(getProcAddr, instance);
}

void RecorderWrapper::LoadDeviceLevelFunctions(PFN_vkGetDeviceProcAddr getProcAddr,
                                               VkDevice device) {
  CaptureManager::Get().LoadDeviceFunctions(getProcAddr, device);
}

void RecorderWrapper::LoadDeviceLevelFunctions(void* dispatchKey, VkDevice device) {
  CaptureManager::Get().LoadDeviceFunctions(dispatchKey, device);
}

PFN_vkVoidFunction RecorderWrapper::GetFunctionWrapper(const char* name) {
  return CaptureManager::Get().GetFunctionWrapper(name);
}

} // namespace vulkan
} // namespace gits

std::unique_ptr<gits::vulkan::RecorderWrapper> g_RecorderWrapper;

#ifdef GITS_PLATFORM_LINUX
namespace {
void DetachVulkan2() {
  static bool finished = false;
  if (!finished) {
    finished = true;
    gits::vulkan::CaptureManager::Cleanup();
    LOG_INFO << "Recording done";
  }
}
} // namespace
#endif

gits::vulkan::IRecorderWrapper* STDCALL GITSRecorderVulkan2() {
  if (!g_RecorderWrapper) {
    try {
      g_RecorderWrapper = std::make_unique<gits::vulkan::RecorderWrapper>();
#ifdef GITS_PLATFORM_LINUX
      std::atexit(DetachVulkan2);
#endif
    } catch (const std::exception& e) {
      LOG_ERROR << "Cannot initialize recorder: " << e.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  return g_RecorderWrapper.get();
}
