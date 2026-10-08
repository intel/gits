// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanHeader2.h"
#include "gitsLoader.h"
#include "vulkanRecorderInterface.h"
#include "log.h"

#if defined(GITS_PLATFORM_WINDOWS)
#define VK_LAYER_EXPORT extern "C" __declspec(dllexport)
#elif defined(GITS_PLATFORM_X11)
#define VK_LAYER_EXPORT extern "C" __attribute__((visibility("default")))
#endif

std::unique_ptr<gits::CGitsLoader> g_GitsLoader;
gits::vulkan::IRecorderWrapper* g_RecorderWrapper = nullptr;
// Next-in-chain lookups, captured during instance/device creation.
PFN_vkGetInstanceProcAddr g_NextGetInstanceProcAddr = nullptr;
PFN_vkGetDeviceProcAddr g_NextGetDeviceProcAddr = nullptr;

void Initialize() {
  static bool s_Initialized = false;
  if (s_Initialized) {
    return;
  }
  s_Initialized = true;
  g_GitsLoader = std::make_unique<gits::CGitsLoader>("GITSRecorderVulkan2", false);
  g_RecorderWrapper =
      reinterpret_cast<gits::vulkan::IRecorderWrapper*>(g_GitsLoader->GetRecorderWrapperPtr());
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkCreateInstanceGITSLayer(const VkInstanceCreateInfo* pCreateInfo,
                          const VkAllocationCallbacks* pAllocator,
                          VkInstance* pInstance) {

  VkLayerInstanceCreateInfo* chainInfo = (VkLayerInstanceCreateInfo*)pCreateInfo->pNext;
  while (chainInfo && (chainInfo->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO ||
                       chainInfo->function != VK_LAYER_LINK_INFO)) {
    chainInfo = (VkLayerInstanceCreateInfo*)chainInfo->pNext;
  }
  if (chainInfo == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddrFunction =
      chainInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
  chainInfo->u.pLayerInfo = chainInfo->u.pLayerInfo->pNext;
  g_NextGetInstanceProcAddr = vkGetInstanceProcAddrFunction;

  g_RecorderWrapper->LoadGlobalLevelFunctions(vkGetInstanceProcAddrFunction);
  auto vkCreateInstanceWrapper = reinterpret_cast<PFN_vkCreateInstance>(
      g_RecorderWrapper->GetFunctionWrapper("vkCreateInstance"));
  if (vkCreateInstanceWrapper == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  VkResult result = vkCreateInstanceWrapper(pCreateInfo, pAllocator, pInstance);
  if (result != VK_SUCCESS) {
    return result;
  }

  g_RecorderWrapper->LoadInstanceLevelFunctions(vkGetInstanceProcAddrFunction, *pInstance);

  return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDeviceGITSLayer(VkPhysicalDevice physicalDevice,
                        const VkDeviceCreateInfo* pCreateInfo,
                        const VkAllocationCallbacks* pAllocator,
                        VkDevice* pDevice) {

  VkLayerDeviceCreateInfo* chainInfo = (VkLayerDeviceCreateInfo*)pCreateInfo->pNext;
  while (chainInfo && (chainInfo->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO ||
                       chainInfo->function != VK_LAYER_LINK_INFO)) {
    chainInfo = (VkLayerDeviceCreateInfo*)chainInfo->pNext;
  }
  if (chainInfo == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  auto vkCreateDeviceWrapper =
      reinterpret_cast<PFN_vkCreateDevice>(g_RecorderWrapper->GetFunctionWrapper("vkCreateDevice"));
  if (vkCreateDeviceWrapper == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  PFN_vkGetDeviceProcAddr vkGetDeviceProcAddrFunction =
      chainInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;
  chainInfo->u.pLayerInfo = chainInfo->u.pLayerInfo->pNext;
  g_NextGetDeviceProcAddr = vkGetDeviceProcAddrFunction;

  VkResult result = vkCreateDeviceWrapper(physicalDevice, pCreateInfo, pAllocator, pDevice);
  if (result != VK_SUCCESS) {
    return result;
  }

  g_RecorderWrapper->LoadDeviceLevelFunctions(vkGetDeviceProcAddrFunction, *pDevice);

  return result;
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetDeviceProcAddrGITSLayer(VkDevice device, const char* pName) {
  if (strcmp(pName, "vkGetDeviceProcAddr") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceProcAddrGITSLayer);
  }
  // Only expose entry points that the rest of the chain actually provides.
  if (g_NextGetDeviceProcAddr != nullptr) {
    PFN_vkVoidFunction nextFunction = g_NextGetDeviceProcAddr(device, pName);
    if (nextFunction == nullptr) {
      return nullptr;
    }
    PFN_vkVoidFunction wrapper = g_RecorderWrapper->GetFunctionWrapper(pName);
    if (wrapper != nullptr) {
      return wrapper;
    }
    // GITS does not wrap this function - pass the driver's implementation through.
    LOG_WARNING << "vkGetDeviceProcAddr() returned driver function for: " << pName;
    return nextFunction;
  }
  return g_RecorderWrapper->GetFunctionWrapper(pName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddrGITSLayer(VkInstance instance, const char* pName) {
  Initialize();

  if (strcmp(pName, "vkCreateInstance") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(vkCreateInstanceGITSLayer);
  } else if (strcmp(pName, "vkCreateDevice") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(vkCreateDeviceGITSLayer);
  } else if (strcmp(pName, "vkGetDeviceProcAddr") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceProcAddrGITSLayer);
  }

  // Only expose entry points that the rest of the chain actually provides.
  if (g_NextGetInstanceProcAddr != nullptr && instance != VK_NULL_HANDLE) {
    PFN_vkVoidFunction nextFunction = g_NextGetInstanceProcAddr(instance, pName);
    if (nextFunction == nullptr) {
      return nullptr;
    }
    PFN_vkVoidFunction wrapper = g_RecorderWrapper->GetFunctionWrapper(pName);
    if (wrapper != nullptr) {
      return wrapper;
    }
    // GITS does not wrap this function - pass the driver's implementation through.
    LOG_WARNING << "vkGetInstanceProcAddr() returned driver function for: " << pName;
    return nextFunction;
  }

  return g_RecorderWrapper->GetFunctionWrapper(pName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetPhysicalDeviceProcAddrGITSLayer(VkInstance instance, const char* pName) {
  return g_RecorderWrapper->GetFunctionWrapper(pName);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkNegotiateLoaderLayerInterfaceVersionGITSLayer(VkNegotiateLayerInterface* pVersionStruct) {
  if (pVersionStruct->sType != LAYER_NEGOTIATE_INTERFACE_STRUCT) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  if (pVersionStruct->loaderLayerInterfaceVersion >= 2) {
    pVersionStruct->pfnGetInstanceProcAddr = vkGetInstanceProcAddrGITSLayer;
    pVersionStruct->pfnGetDeviceProcAddr = vkGetDeviceProcAddrGITSLayer;
    pVersionStruct->pfnGetPhysicalDeviceProcAddr = vkGetPhysicalDeviceProcAddrGITSLayer;
  }

  return VK_SUCCESS;
}
