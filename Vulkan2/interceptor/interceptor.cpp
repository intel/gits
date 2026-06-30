// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanHeader.h"
#include "gitsLoader.h"
#include "vulkanRecorderInterface.h"

#if defined(GITS_PLATFORM_WINDOWS)
#define VK_INTERCEPTOR_EXPORT extern "C"
#elif defined(GITS_PLATFORM_X11)
#define VK_INTERCEPTOR_EXPORT __attribute__((visibility("default")))
#endif

std::unique_ptr<gits::CGitsLoader> g_GitsLoader;
gits::vulkan::IRecorderWrapper* g_RecorderWrapper = nullptr;
PFN_vkGetInstanceProcAddr g_vkGetInstanceProcAddr = nullptr;

void Initialize() {
  static bool s_Initialized = false;
  if (s_Initialized) {
    return;
  }
#if _DEBUG
  MessageBox(0, "Waiting for debugger...", "Waiting for debugger...", 0);
#endif
  s_Initialized = true;
  g_GitsLoader = std::make_unique<gits::CGitsLoader>("GITSRecorderVulkan2");
  g_RecorderWrapper =
      reinterpret_cast<gits::vulkan::IRecorderWrapper*>(g_GitsLoader->GetRecorderWrapperPtr());

  auto& cfg = g_GitsLoader->GetConfiguration();
  auto lib = dl::open_library(cfg.common.recorder.libVK.string().c_str());

  g_vkGetInstanceProcAddr =
      reinterpret_cast<PFN_vkGetInstanceProcAddr>(dl::load_symbol(lib, "vkGetInstanceProcAddr"));
  g_RecorderWrapper->LoadGlobalLevelFunctions(g_vkGetInstanceProcAddr);
}

VK_INTERCEPTOR_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
  Initialize();

  if (strcmp(pName, "vkCreateInstance") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(vkCreateInstance);
  } else if (strcmp(pName, "vkCreateDevice") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(vkCreateDevice);
  } else if (strcmp(pName, "vkGetDeviceProcAddr") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceProcAddr);
  }

  return g_RecorderWrapper->GetFunctionWrapper(pName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName) {
  if (strcmp(pName, "vkGetDeviceProcAddr") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceProcAddr);
  }
  return g_RecorderWrapper->GetFunctionWrapper(pName);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                                                const VkAllocationCallbacks* pAllocator,
                                                VkInstance* pInstance) {
  auto vkCreateInstanceWrapper = reinterpret_cast<PFN_vkCreateInstance>(
      g_RecorderWrapper->GetFunctionWrapper("vkCreateInstance"));
  if (vkCreateInstanceWrapper == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  VkResult result = vkCreateInstanceWrapper(pCreateInfo, pAllocator, pInstance);
  if (result != VK_SUCCESS) {
    return result;
  }
  g_RecorderWrapper->LoadInstanceLevelFunctions(g_vkGetInstanceProcAddr, *pInstance);
  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice physicalDevice,
                                              const VkDeviceCreateInfo* pCreateInfo,
                                              const VkAllocationCallbacks* pAllocator,
                                              VkDevice* pDevice) {
  auto vkCreateDeviceWrapper =
      reinterpret_cast<PFN_vkCreateDevice>(g_RecorderWrapper->GetFunctionWrapper("vkCreateDevice"));
  if (vkCreateDeviceWrapper == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  VkResult result = vkCreateDeviceWrapper(physicalDevice, pCreateInfo, pAllocator, pDevice);
  if (result != VK_SUCCESS) {
    return result;
  }
  void* dispatchKey = *reinterpret_cast<void**>(physicalDevice);
  g_RecorderWrapper->LoadDeviceLevelFunctions(dispatchKey, *pDevice);
  return result;
}
