// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanExecWrap.h
*
* @brief Automatically generated declarations
*
*/

#pragma once

#include "gitsPluginVulkan.h"
#include "vulkanTools_lite.h"
#include "vulkanRecorderWrapper.h"
#include "log2.h"

#if defined GITS_PLATFORM_X11
#include <dlfcn.h>
#include <unistd.h>
#endif

extern const std::unordered_map<std::string, PFN_vkVoidFunction> interceptorExportedFunctions;

#ifdef BUILD_FOR_VULKAN_LAYER
static const VkLayerProperties VulkanGITSRecorderLayer = {
    "VK_LAYER_INTEL_vulkan_GITS_recorder", VK_MAKE_API_VERSION(0, 1, 3, 229), 1,
    "Vulkan Layer used to record GITS Vulkan streams"};
#endif

namespace gits {
namespace Vulkan {

// vkNegotiateLoaderLayerInterfaceVersion

VkResult recExecWrap_vkNegotiateLoaderLayerInterfaceVersion(
    VkNegotiateLayerInterface* pVersionStruct) {
  if (pVersionStruct->loaderLayerInterfaceVersion >= 2) {
    auto get_instance = interceptorExportedFunctions.find("vkGetInstanceProcAddr");
    if (get_instance != interceptorExportedFunctions.end()) {
      pVersionStruct->pfnGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)get_instance->second;
    } else {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    auto get_device = interceptorExportedFunctions.find("vkGetDeviceProcAddr");
    if (get_device != interceptorExportedFunctions.end()) {
      pVersionStruct->pfnGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)get_device->second;
    } else {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    auto get_physical_device = interceptorExportedFunctions.find("GetPhysicalDeviceProcAddr");
    if (get_physical_device != interceptorExportedFunctions.end()) {
      pVersionStruct->pfnGetPhysicalDeviceProcAddr =
          (PFN_GetPhysicalDeviceProcAddr)get_physical_device->second;
    } else {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  return VK_SUCCESS;
}

// vkIAmGITS

void recExecWrap_vkIAmGITS() {
  CGitsPluginVulkan::RecorderWrapper().DisableConfigOptions();
}

// vkPauseRecordingGITS

void recExecWrap_vkPauseRecordingGITS() {
  CGitsPluginVulkan::RecorderWrapper().PauseRecording();
}

// vkContinueRecordingGITS

void recExecWrap_vkContinueRecordingGITS() {
  CGitsPluginVulkan::RecorderWrapper().ContinueRecording();
}

// vkGetDeviceProcAddr

PFN_vkVoidFunction recExecWrap_vkGetDeviceProcAddr(VkDevice device, const char* pName) {
  if (strcmp(VK_UNWIND_QUEUE_PRESENT_GITS_FUNCTION_NAME, pName) == 0) {
    CGitsPluginVulkan::RecorderWrapper().IgnoreNextQueuePresentGITS();
    return NULL;
  }

#ifdef BUILD_FOR_VULKAN_INTERCEPTOR
  PFN_vkVoidFunction return_value =
      CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetDeviceProcAddr(device, pName);
  if (strcmp(VK_TAG_MEMORY_CONTENTS_UPDATE_GITS_FUNCTION_NAME, pName) == 0) {
    // Return value from GITS recorder
  } else if (return_value == NULL) {
    return NULL;
  }
#endif

  auto return_value_gits = interceptorExportedFunctions.find(pName);
  if (return_value_gits != interceptorExportedFunctions.end()) {
    return return_value_gits->second;
  }

#ifdef BUILD_FOR_VULKAN_LAYER
  PFN_vkVoidFunction return_value =
      CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetDeviceProcAddr(device, pName);
#endif
  LOG_WARNING << "vkGetDeviceProcAddr() returned driver function for: " << pName;
  return return_value;
}

// vkGetInstanceProcAddr

PFN_vkVoidFunction recExecWrap_vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
#ifdef BUILD_FOR_VULKAN_INTERCEPTOR
  PFN_vkVoidFunction return_value =
      CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetInstanceProcAddr(instance, pName);
  if ((strcmp(VK_PASS_PHYSICAL_DEVICE_MEMORY_PROPERTIES_GITS_FUNCTION_NAME, pName) == 0) ||
      (strcmp(VK_I_AM_GITS_FUNCTION_NAME, pName) == 0) ||
      (strcmp(VK_PAUSE_RECORDING_GITS_FUNCTION_NAME, pName) == 0) ||
      (strcmp(VK_CONTINUE_RECORDING_GITS_FUNCTION_NAME, pName) == 0)) {
    // These are special, GITS internal functions which are not available in any
    // Vulkan driver. They are used for player <-> recorder communication so we
    // need to provide function pointers even though driver returns NULL for
    // them.
  } else if (return_value == NULL) {
    return NULL;
  }
#endif

  auto return_value_gits = interceptorExportedFunctions.find(pName);
  if (return_value_gits != interceptorExportedFunctions.end()) {
    return return_value_gits->second;
  }

#ifdef BUILD_FOR_VULKAN_LAYER
  PFN_vkVoidFunction return_value =
      CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetInstanceProcAddr(instance, pName);
#endif

  LOG_WARNING << "vkGetInstanceProcAddr() returned driver function for: " << pName;
  return return_value;
}

// GetPhysicalDeviceProcAddr

PFN_vkVoidFunction recExecWrap_GetPhysicalDeviceProcAddr(VkInstance instance, const char* pName) {
#ifdef BUILD_FOR_VULKAN_INTERCEPTOR
  PFN_vkVoidFunction return_value =
      CGitsPluginVulkan::RecorderWrapper().Drivers().GetPhysicalDeviceProcAddr(instance, pName);
  if (return_value == NULL) {
    return NULL;
  }
#endif

  auto return_value_gits = interceptorExportedFunctions.find(pName);
  if (return_value_gits != interceptorExportedFunctions.end()) {
    return return_value_gits->second;
  }

#ifdef BUILD_FOR_VULKAN_LAYER
  PFN_vkVoidFunction return_value =
      CGitsPluginVulkan::RecorderWrapper().Drivers().GetPhysicalDeviceProcAddr(instance, pName);
#endif

  LOG_WARNING << "GetPhysicalDeviceProcAddr() returned driver function for: " << pName;
  return return_value;
}

// vkQueueSubmit

VkResult recExecWrap_vkQueueSubmit(VkQueue queue,
                                   uint32_t submitCount,
                                   const VkSubmitInfo* pSubmits,
                                   VkFence fence) {
  CVkDriver& drvVk = CGitsPluginVulkan::RecorderWrapper().Drivers();
  VkResult return_value = VK_SUCCESS;
  if (!CGitsPluginVulkan::Configuration().vulkan.recorder.dumpSubmits.empty() &&
      CGitsPluginVulkan::Configuration().common.recorder.enabled &&
      !CGitsPluginVulkan::_recorderFinished) {
    for (uint32_t i = 0; i < submitCount; i++) {
      VkFence fenceNew = VK_NULL_HANDLE;
      VkSubmitInfo const& submitInfoOrig = pSubmits[i];

      if (submitInfoOrig.commandBufferCount == 0) {
        // last submit in QueueSubmit (restoring original fence)
        if (i == (submitCount - 1)) {
          fenceNew = fence;
        }
        return_value = drvVk.vkQueueSubmit(queue, 1, &submitInfoOrig, fenceNew);
        if (return_value != VK_SUCCESS) {
          LOG_WARNING << "vkQueueSubmit failed.";
        }
      }

      for (uint32_t cmdBufIndex = 0; cmdBufIndex < submitInfoOrig.commandBufferCount;
           cmdBufIndex++) {
        const VkCommandBuffer& cmdbuffer = submitInfoOrig.pCommandBuffers[cmdBufIndex];
        VkSubmitInfo submitInfoNew;
        // last command buffer in queue submit (restoring original settings)
        if (cmdBufIndex == (submitInfoOrig.commandBufferCount - 1)) {
          // last submit in QueueSubmit (restoring original fence)
          if (i == (submitCount - 1)) {
            fenceNew = fence;
          }
          submitInfoNew = {VK_STRUCTURE_TYPE_SUBMIT_INFO,
                           submitInfoOrig.pNext,
                           submitInfoOrig.waitSemaphoreCount,
                           submitInfoOrig.pWaitSemaphores,
                           submitInfoOrig.pWaitDstStageMask,
                           1,
                           &cmdbuffer,
                           submitInfoOrig.signalSemaphoreCount,
                           submitInfoOrig.pSignalSemaphores};
        } else {
          submitInfoNew = {
              VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, 0, 1, &cmdbuffer, 0, nullptr};
        }
        return_value = drvVk.vkQueueSubmit(queue, 1, &submitInfoNew, fenceNew);
        if (return_value != VK_SUCCESS) {
          LOG_WARNING << "vkQueueSubmit failed. Cannot dump submit image.";
        } else {
          CGitsPluginVulkan::RecorderWrapper().dumpScreenshot(queue, cmdbuffer, i, cmdBufIndex);
        }
      }
    }
  } else {
    return_value = drvVk.vkQueueSubmit(queue, submitCount, pSubmits, fence);
  }

  CGitsPluginVulkan::RecorderWrapper().resetMemoryAfterQueueSubmit(queue, submitCount, pSubmits);
  return return_value;
}

VkResult recExecWrap_vkQueueSubmit2(VkQueue queue,
                                    uint32_t submitCount,
                                    const VkSubmitInfo2* pSubmits,
                                    VkFence fence,
                                    bool isKHR = false) {
  CVkDriver& drvVk = CGitsPluginVulkan::RecorderWrapper().Drivers();
  VkResult return_value = VK_SUCCESS;
  if (!CGitsPluginVulkan::Configuration().vulkan.recorder.dumpSubmits.empty() &&
      CGitsPluginVulkan::Configuration().common.recorder.enabled &&
      !CGitsPluginVulkan::_recorderFinished) {
    for (uint32_t i = 0; i < submitCount; i++) {
      VkFence fenceNew = VK_NULL_HANDLE;
      VkSubmitInfo2 const& submitInfoOrig = pSubmits[i];

      if (submitInfoOrig.commandBufferInfoCount == 0) {
        // last submit in QueueSubmit (restoring original fence)
        if (i == (submitCount - 1)) {
          fenceNew = fence;
        }
        if (isKHR) {
          return_value = drvVk.vkQueueSubmit2KHR(queue, 1, &submitInfoOrig, fenceNew);
        } else {
          return_value = drvVk.vkQueueSubmit2(queue, 1, &submitInfoOrig, fenceNew);
        }
        if (return_value != VK_SUCCESS) {
          LOG_WARNING << "vkQueueSubmit2 failed.";
        }
      }

      for (uint32_t cmdBufIndex = 0; cmdBufIndex < submitInfoOrig.commandBufferInfoCount;
           cmdBufIndex++) {
        const VkCommandBufferSubmitInfo& cmdbufferSubmitInfo =
            submitInfoOrig.pCommandBufferInfos[cmdBufIndex];
        VkSubmitInfo2 submitInfoNew;
        // last command buffer in queue submit (restoring original settings)
        if (cmdBufIndex == (submitInfoOrig.commandBufferInfoCount - 1)) {
          // last submit in QueueSubmit (restoring original fence)
          if (i == (submitCount - 1)) {
            fenceNew = fence;
          }
          submitInfoNew = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                           submitInfoOrig.pNext,
                           submitInfoOrig.flags,
                           submitInfoOrig.waitSemaphoreInfoCount,
                           submitInfoOrig.pWaitSemaphoreInfos,
                           1,
                           &cmdbufferSubmitInfo,
                           submitInfoOrig.signalSemaphoreInfoCount,
                           submitInfoOrig.pSignalSemaphoreInfos};
        } else {
          submitInfoNew = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                           nullptr,
                           0,
                           0,
                           nullptr,
                           1,
                           &cmdbufferSubmitInfo,
                           0,
                           nullptr};
        }
        if (isKHR) {
          return_value = drvVk.vkQueueSubmit2KHR(queue, 1, &submitInfoNew, fenceNew);
        } else {
          return_value = drvVk.vkQueueSubmit2(queue, 1, &submitInfoNew, fenceNew);
        }
        if (return_value != VK_SUCCESS) {
          LOG_WARNING << "vkQueueSubmit2 failed. Cannot dump submit image.";
        } else {
          CGitsPluginVulkan::RecorderWrapper().dumpScreenshot(
              queue, cmdbufferSubmitInfo.commandBuffer, i, cmdBufIndex);
        }
      }
    }
  } else {
    if (isKHR) {
      return_value = drvVk.vkQueueSubmit2KHR(queue, submitCount, pSubmits, fence);
    } else {
      return_value = drvVk.vkQueueSubmit2(queue, submitCount, pSubmits, fence);
    }
  }

  CGitsPluginVulkan::RecorderWrapper().resetMemoryAfterQueueSubmit2(queue, submitCount, pSubmits);
  return return_value;
}

VkResult recExecWrap_vkQueueSubmit2KHR(VkQueue queue,
                                       uint32_t submitCount,
                                       const VkSubmitInfo2* pSubmits,
                                       VkFence fence) {
  auto return_value = recExecWrap_vkQueueSubmit2(queue, submitCount, pSubmits, fence, true);
  return return_value;
}

// vkMapMemory

VkResult recExecWrap_vkMapMemory(VkDevice device,
                                 VkDeviceMemory memory,
                                 VkDeviceSize offset,
                                 VkDeviceSize size,
                                 VkMemoryMapFlags flags,
                                 void** ppData) {
  CVkDriver& drvVk = CGitsPluginVulkan::RecorderWrapper().Drivers();
  VkResult return_value;
  if (CGitsPluginVulkan::Configuration().common.recorder.enabled &&
      (CGitsPluginVulkan::Configuration().vulkan.recorder.mode != TVulkanRecorderMode::ALL) &&
      ((offset != 0) || (size != 0xFFFFFFFFFFFFFFFF)) && !CGitsPluginVulkan::_recorderFinished &&
      !CGitsPluginVulkan::RecorderWrapper().IsUseExternalMemoryExtensionUsed()) {
    VkDeviceSize wholeSize = 0;
    wholeSize = CGitsPluginVulkan::RecorderWrapper().GetWholeMemorySize(memory);
    void* pointer = 0;
    return_value = drvVk.vkMapMemory(device, memory, 0, wholeSize, flags, &pointer);

    if (CGitsPluginVulkan::Configuration().vulkan.recorder.shadowMemory &&
        CGitsPluginVulkan::Configuration().common.recorder.enabled) {
      pointer = CGitsPluginVulkan::RecorderWrapper().GetShadowMemory(
          memory, (char*)pointer, (uint64_t)wholeSize, (uint64_t)0);
    }
    *ppData = (char*)pointer + offset;
  } else {
    return_value = drvVk.vkMapMemory(device, memory, offset, size, flags, ppData);

    if (CGitsPluginVulkan::Configuration().vulkan.recorder.shadowMemory &&
        CGitsPluginVulkan::Configuration().common.recorder.enabled &&
        !CGitsPluginVulkan::_recorderFinished &&
        !CGitsPluginVulkan::RecorderWrapper().IsUseExternalMemoryExtensionUsed()) {
      *ppData = CGitsPluginVulkan::RecorderWrapper().GetShadowMemory(
          memory, (char*)*ppData, (uint64_t)size, (uint64_t)offset);
    }
  }
  return return_value;
}

// vkAllocateMemory

VkResult recExecWrap_vkAllocateMemory(VkDevice device,
                                      const VkMemoryAllocateInfo* pAllocateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDeviceMemory* pMemory) {
  CVkDriver& drvVk = CGitsPluginVulkan::RecorderWrapper().Drivers();

  // Global changes - propagate to recorded streams
  {
    auto* originalAllocateInfo = const_cast<VkMemoryAllocateInfo*>(pAllocateInfo);

    if (CGitsPluginVulkan::Configuration()
            .vulkan.recorder.useCaptureReplayFeaturesForBuffersAndAccelerationStructures) {
      auto allocateFlagsInfo =
          (VkMemoryAllocateFlagsInfo*)CGitsPluginVulkan::RecorderWrapper().GetPNextStructure(
              originalAllocateInfo->pNext, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO);
      if ((allocateFlagsInfo != nullptr) &&
          isBitSet(allocateFlagsInfo->flags, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT)) {
        allocateFlagsInfo->flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
      }
    }
  }

  // Local changes - impact only current execution
  {
    auto localAllocateInfo = *pAllocateInfo;
    // Used only when useExternalMemoryExtension is set to True
    VkImportMemoryHostPointerInfoEXT hostPointerInfo = {
        VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT, // VkStructureType sType;
        localAllocateInfo.pNext,                               // const void* pNext;
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT, // VkExternalMemoryHandleTypeFlagBits handleType;
        nullptr                                                 // void* pHostPointer;
    };

    bool isMemoryMappable = false;
    if (CGitsPluginVulkan::Configuration().common.recorder.enabled) {
      isMemoryMappable = CGitsPluginVulkan::RecorderWrapper().CheckMemoryMappingFeasibility(
          device, localAllocateInfo.memoryTypeIndex, false);

      // Perform only when external memory is ENABLED
      if (CGitsPluginVulkan::RecorderWrapper().IsUseExternalMemoryExtensionUsed() &&
          isMemoryMappable && !CGitsPluginVulkan::_recorderFinished) {
        hostPointerInfo.pHostPointer = CGitsPluginVulkan::RecorderWrapper().CreateExternalMemory(
            localAllocateInfo.allocationSize);
        localAllocateInfo.pNext = &hostPointerInfo;
      }
    }

    auto return_value = drvVk.vkAllocateMemory(device, &localAllocateInfo, pAllocator, pMemory);

    // Perform only when external memory is DISABLED
    if (!CGitsPluginVulkan::RecorderWrapper().IsUseExternalMemoryExtensionUsed() &&
        isMemoryMappable && CGitsPluginVulkan::Configuration().common.recorder.enabled &&
        !CGitsPluginVulkan::_recorderFinished &&
        (CGitsPluginVulkan::Configuration().vulkan.recorder.shadowMemory ||
         CGitsPluginVulkan::Configuration().vulkan.recorder.memorySegmentSize ||
         CGitsPluginVulkan::Configuration().vulkan.recorder.memoryAccessDetection)) {
      // Clear memory
      void* ptr = nullptr;
      VkResult map_return_value = drvVk.vkMapMemory(device, *pMemory, 0, VK_WHOLE_SIZE, 0, &ptr);

      if (map_return_value == VK_SUCCESS) {
        memset(ptr, 0, (size_t)localAllocateInfo.allocationSize);
        drvVk.vkUnmapMemory(device, *pMemory);
      } else {
        LOG_WARNING
            << "vkMapMemory() was used to clear previously allocated memory but failed with "
               "the code: "
            << map_return_value << ". It may cause rendering errors!";
      }
    }

    if (CGitsPluginVulkan::Configuration().common.recorder.enabled) {
      CGitsPluginVulkan::RecorderWrapper().TrackMemoryState(
          return_value, device, pAllocateInfo, pAllocator, pMemory, hostPointerInfo.pHostPointer);
    }

    return return_value;
  }
}

void recExecWrap_vkFreeMemory(VkDevice device,
                              VkDeviceMemory memory,
                              const VkAllocationCallbacks* pAllocator) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkFreeMemory(device, memory, pAllocator);
  if (CGitsPluginVulkan::Configuration().common.recorder.enabled &&
      CGitsPluginVulkan::RecorderWrapper().IsUseExternalMemoryExtensionUsed() &&
      (memory != VK_NULL_HANDLE)) {
    CGitsPluginVulkan::RecorderWrapper().FreeExternalMemory(memory);
  }
}

VkResult recExecWrap_vkCreateDebugReportCallbackEXT(
    VkInstance instance,
    const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugReportCallbackEXT* pCallback) {
  // due to bug for Autodesk Scaleform GfxPlayer
  LOG_WARNING << "Omitting vkCreateDebugReportCallbackEXT calling.";
  return VK_SUCCESS;
}

VkResult recExecWrap_vkDestroyDebugReportCallbackEXT(VkInstance instance,
                                                     VkDebugReportCallbackEXT callback,
                                                     const VkAllocationCallbacks* pAllocator) {
  // due to bug for Autodesk Scaleform GfxPlayer
  LOG_WARNING << "Omitting vkDestroyDebugReportCallbackEXT calling.";
  return VK_SUCCESS;
}

VkResult recExecWrap_vkGetFenceStatus(VkDevice device, VkFence fence) {
  return CGitsPluginVulkan::RecorderWrapper().CheckFenceStatus(device, fence);
}

VkResult recExecWrap_vkWaitForFences(VkDevice device,
                                     uint32_t fenceCount,
                                     const VkFence* pFences,
                                     VkBool32 waitAll,
                                     uint64_t timeout) {
  uint64_t timeoutSubtract = std::min(
      timeout, (uint64_t)CGitsPluginVulkan::Configuration().vulkan.recorder.shortenFenceWaitTime);
  VkResult return_value = CGitsPluginVulkan::RecorderWrapper().Drivers().vkWaitForFences(
      device, fenceCount, pFences, waitAll, timeout - timeoutSubtract);

  if ((VK_SUCCESS == return_value) &&
      (CGitsPluginVulkan::Configuration().vulkan.recorder.delayFenceChecksCount > 0)) {
    if (!waitAll) {
      return_value = VK_TIMEOUT;
    }
    for (unsigned i = 0; i < fenceCount; i++) {
      VkResult retVal = CGitsPluginVulkan::RecorderWrapper().CheckFenceStatus(device, pFences[i]);

      if (VK_SUCCESS == retVal) {
        // Some of the fences already reached their "delay" limit, so we cannot
        // change the returned value
        if (!waitAll) {
          return_value = VK_SUCCESS;
        }
      } else {
        if (waitAll) {
          return_value = VK_TIMEOUT;
        }
      }
    }
  }
  return return_value;
}

VkResult recExecWrap_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkInstance* pInstance) {
  static bool scheduled = false;
  auto& cfg = CGitsPluginVulkan::Configuration();
  bool isRecordingEnabled = cfg.common.recorder.enabled;
  bool isAllMode = cfg.vulkan.recorder.mode == TVulkanRecorderMode::ALL;
  if (!scheduled && isRecordingEnabled && isAllMode) {
    scheduled = true;
  }
  VkResult return_value = CGitsPluginVulkan::RecorderWrapper().Drivers().vkCreateInstance(
      pCreateInfo, pAllocator, pInstance);
  return return_value;
}

VkResult recExecWrap_vkCreateDevice(VkPhysicalDevice physicalDevice,
                                    const VkDeviceCreateInfo* pCreateInfo,
                                    const VkAllocationCallbacks* pAllocator,
                                    VkDevice* pDevice) {
  // Enable capture/replay features for buffers and acceleration structures when
  // requested in the recorder config file
  if (CGitsPluginVulkan::Configuration()
          .vulkan.recorder.useCaptureReplayFeaturesForBuffersAndAccelerationStructures) {
    // Core 1.2
    {
      auto vulkan12Features =
          (VkPhysicalDeviceVulkan12Features*)CGitsPluginVulkan::RecorderWrapper().GetPNextStructure(
              pCreateInfo->pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
      if ((vulkan12Features != nullptr) && (vulkan12Features->bufferDeviceAddress == VK_TRUE)) {
        vulkan12Features->bufferDeviceAddressCaptureReplay = VK_TRUE;
      }
    }
    // KHR
    {
      auto bufferDeviceAddressFeatures =
          (VkPhysicalDeviceBufferDeviceAddressFeatures*)CGitsPluginVulkan::RecorderWrapper()
              .GetPNextStructure(pCreateInfo->pNext,
                                 VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES);
      if ((bufferDeviceAddressFeatures != nullptr) &&
          (bufferDeviceAddressFeatures->bufferDeviceAddress == VK_TRUE)) {
        bufferDeviceAddressFeatures->bufferDeviceAddressCaptureReplay = VK_TRUE;
      }

      auto accelerationStructureFeatures =
          (VkPhysicalDeviceAccelerationStructureFeaturesKHR*)CGitsPluginVulkan::RecorderWrapper()
              .GetPNextStructure(
                  pCreateInfo->pNext,
                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR);
      if ((accelerationStructureFeatures != nullptr) &&
          (accelerationStructureFeatures->accelerationStructure == VK_TRUE)) {
        accelerationStructureFeatures->accelerationStructureCaptureReplay = VK_TRUE;
      }
    }
    // EXT
    {
      auto bufferDeviceAddressFeatures =
          (VkPhysicalDeviceBufferDeviceAddressFeatures*)CGitsPluginVulkan::RecorderWrapper()
              .GetPNextStructure(
                  pCreateInfo->pNext,
                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT);
      if ((bufferDeviceAddressFeatures != nullptr) &&
          (bufferDeviceAddressFeatures->bufferDeviceAddress == VK_TRUE)) {
        bufferDeviceAddressFeatures->bufferDeviceAddressCaptureReplay = VK_TRUE;
      }
    }
  }

  // Enable capture/replay feature for ray tracing pipelines when requested in
  // the recorder config file
  if (CGitsPluginVulkan::Configuration()
          .vulkan.recorder.useCaptureReplayFeaturesForRayTracingPipelines) {
    VkPhysicalDeviceProperties properties;
    CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetPhysicalDeviceProperties(physicalDevice,
                                                                                 &properties);

    // Don't enable it on non-Intel hardware as it may not support the feature
    if (properties.vendorID == VENDOR_ID_INTEL) {
      auto rayTracingPipelineFeatures =
          (VkPhysicalDeviceRayTracingPipelineFeaturesKHR*)CGitsPluginVulkan::RecorderWrapper()
              .GetPNextStructure(
                  pCreateInfo->pNext,
                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR);

      if ((rayTracingPipelineFeatures != nullptr) &&
          (rayTracingPipelineFeatures->rayTracingPipeline == VK_TRUE)) {
        rayTracingPipelineFeatures->rayTracingPipelineShaderGroupHandleCaptureReplay = VK_TRUE;
      }
    } else {
      auto& cfg = CGitsPluginVulkan::Configuration();
      cfg.vulkan.recorder.useCaptureReplayFeaturesForRayTracingPipelines = false;

      CALL_ONCE[] {
        LOG_WARNING << "Capture/replay feature for recording ray tracing pipeline handles is by "
                       "default disabled on non-Intel hardware.";
      };
    }
  }

  VkDeviceCreateInfo createInfo = *pCreateInfo;
  std::set<std::string> enabledExtensionsSet(createInfo.ppEnabledExtensionNames,
                                             createInfo.ppEnabledExtensionNames +
                                                 createInfo.enabledExtensionCount);

  // Add required extensions to be used for external memory (when requested)
  if (CGitsPluginVulkan::RecorderWrapper().IsUseExternalMemoryExtensionUsed()) {
    const char* externalMemoryExtensionName = VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME;
    const char* externalMemoryHostExtensionName = VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME;

    bool isVersion11Supported =
        CGitsPluginVulkan::RecorderWrapper().IsVulkanAPIVersionSupported(1, 1, physicalDevice);
    bool externalMemorySupported =
        CGitsPluginVulkan::RecorderWrapper().AreDeviceExtensionsSupported(
            physicalDevice, 1, &externalMemoryExtensionName);
    bool externalHostMemorySupported =
        CGitsPluginVulkan::RecorderWrapper().AreDeviceExtensionsSupported(
            physicalDevice, 1, &externalMemoryHostExtensionName);

    // Version 1.1 and external HOST memory is supported
    if (isVersion11Supported && externalHostMemorySupported) {
      enabledExtensionsSet.insert(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME);
    }
    // External memory and external HOST memory is supported
    else if (externalMemorySupported && externalHostMemorySupported) {
      enabledExtensionsSet.insert(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
      enabledExtensionsSet.insert(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME);
    }
    // External memory / host is not supported, yet useExternalMemoryExtension
    // is requested via config
    else {
      throw std::runtime_error("External memory but the " VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME
                               " extension is not supported on the current hardware. Exiting!");
    }
  }

  // Enable additional extensions and features which are required to properly capture
  // acceleration structure build data during substreams recording
  VkPhysicalDevice8BitStorageFeatures gits8bitStorageFeatures = {};
  gits8bitStorageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
  gits8bitStorageFeatures.storageBuffer8BitAccess = VK_TRUE;

  VkPhysicalDevice16BitStorageFeatures gits16bitStorageFeatures = {};
  gits16bitStorageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
  gits16bitStorageFeatures.storageBuffer16BitAccess = VK_TRUE;

  if (CGitsPluginVulkan::RecorderWrapper().IsSubcaptureBeforeRestorationPhase()) {

    // If application uses ray tracing, GITS needs to inject additional extensions and features
    // in order to capture/gather some RT-related data
    auto accelerationStructureFeatures =
        (VkPhysicalDeviceAccelerationStructureFeaturesKHR*)CGitsPluginVulkan::RecorderWrapper()
            .GetPNextStructure(
                pCreateInfo->pNext,
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR);
    if ((accelerationStructureFeatures != nullptr) &&
        (accelerationStructureFeatures->accelerationStructure == VK_TRUE)) {
      // Vulkan 1.2 and 8-bit storage features
      {
        auto appVulkan12Features =
            (VkPhysicalDeviceVulkan12Features*)CGitsPluginVulkan::RecorderWrapper()
                .GetPNextStructure(pCreateInfo->pNext,
                                   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);

        if (appVulkan12Features != nullptr) {
          // VkPhysicalDeviceVulkan12Features is provided by the application
          appVulkan12Features->storageBuffer8BitAccess = VK_TRUE;
        } else {
          // VkPhysicalDeviceVulkan12Features is not available
          enabledExtensionsSet.insert(VK_KHR_8BIT_STORAGE_EXTENSION_NAME);

          auto app8bitStorageFeatures =
              (VkPhysicalDevice8BitStorageFeatures*)CGitsPluginVulkan::RecorderWrapper()
                  .GetPNextStructure(pCreateInfo->pNext,
                                     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES);
          if (app8bitStorageFeatures != nullptr) {
            // VkPhysicalDevice8BitStorageFeatures is provided by the application
            app8bitStorageFeatures->storageBuffer8BitAccess = VK_TRUE;
          } else {
            // VkPhysicalDevice8BitStorageFeatures is not available - inject it
            gits8bitStorageFeatures.pNext = (void*)createInfo.pNext;
            createInfo.pNext = &gits8bitStorageFeatures;
          }
        }
      }

      // Vulkan 1.1 and 16-bit storage features
      {
        auto appVulkan11Features =
            (VkPhysicalDeviceVulkan11Features*)CGitsPluginVulkan::RecorderWrapper()
                .GetPNextStructure(pCreateInfo->pNext,
                                   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES);

        if (appVulkan11Features != nullptr) {
          // VkPhysicalDeviceVulkan11Features is provided by the application
          appVulkan11Features->storageBuffer16BitAccess = VK_TRUE;
        } else {
          // Vulkan 1.1 is NOT supported
          enabledExtensionsSet.insert(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);

          auto app16bitStorageFeatures =
              (VkPhysicalDevice16BitStorageFeatures*)CGitsPluginVulkan::RecorderWrapper()
                  .GetPNextStructure(pCreateInfo->pNext,
                                     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES);

          if (app16bitStorageFeatures != nullptr) {
            // VkPhysicalDevice16BitStorageFeatures is provided by the application
            app16bitStorageFeatures->storageBuffer16BitAccess = VK_TRUE;
          } else {
            // VkPhysicalDevice16BitStorageFeatures is not available - inject it
            gits16bitStorageFeatures.pNext = (void*)createInfo.pNext;
            createInfo.pNext = &gits16bitStorageFeatures;
          }
        }
      }
    }
  }

  // std::set was used to remove duplicate entries. Now we need to convert it to a vector
  // in order to pass data to a CreateInfo structure.
  std::vector<const char*> enabledExtensions;
  enabledExtensions.reserve(enabledExtensionsSet.size());
  for (const auto& extension : enabledExtensionsSet) {
    enabledExtensions.emplace_back(extension.c_str());
  }

  createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
  createInfo.ppEnabledExtensionNames = enabledExtensions.data();

  VkResult return_value = CGitsPluginVulkan::RecorderWrapper().Drivers().vkCreateDevice(
      physicalDevice, &createInfo, pAllocator, pDevice);
  if (return_value != VK_SUCCESS) {
    LOG_WARNING << "Device creation failed with the following error: " << return_value;
  }
  return return_value;
}

#ifdef BUILD_FOR_VULKAN_LAYER
namespace {

VkResult EnumerateLayerHelper(const uint32_t count,
                              uint32_t* pPropertyCount,
                              VkLayerProperties* pProperties) {
  if (pProperties == NULL) {
    *pPropertyCount = count;
    return VK_SUCCESS;
  }

  uint32_t copy_size = *pPropertyCount < count ? *pPropertyCount : count;
  memcpy(pProperties, &VulkanGITSRecorderLayer, copy_size * sizeof(VkLayerProperties));
  *pPropertyCount = copy_size;
  if (copy_size < count) {
    return VK_INCOMPLETE;
  }

  return VK_SUCCESS;
}

} // namespace
#endif

VkResult recExecWrap_vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount,
                                                        VkLayerProperties* pProperties) {
#ifdef BUILD_FOR_VULKAN_INTERCEPTOR

  CVkDriver& drvVk = CGitsPluginVulkan::RecorderWrapper().Drivers();
  VkResult return_value = drvVk.vkEnumerateInstanceLayerProperties(pPropertyCount, pProperties);

  if (CGitsPluginVulkan::Configuration().common.recorder.enabled &&
      !CGitsPluginVulkan::_recorderFinished &&
      !CGitsPluginVulkan::Configuration().vulkan.shared.suppressLayers.empty()) {
    uint32_t propertyCount = 0;
    std::vector<VkLayerProperties> properties;

    if ((drvVk.vkEnumerateInstanceLayerProperties(&propertyCount, nullptr) == VK_SUCCESS) &&
        (propertyCount > 0)) {
      properties.resize(propertyCount);

      if (drvVk.vkEnumerateInstanceLayerProperties(&propertyCount, properties.data()) ==
          VK_SUCCESS) {
        properties.erase(
            std::remove_if(properties.begin(), properties.end(),
                           [](VkLayerProperties& element) {
                             auto& suppressLayers =
                                 CGitsPluginVulkan::Configuration().vulkan.shared.suppressLayers;
                             return std::find(suppressLayers.begin(), suppressLayers.end(),
                                              element.layerName) != suppressLayers.end();
                           }),
            properties.end());
      }
    }

    if (pProperties != nullptr) {
      for (uint32_t i = 0; (i < *pPropertyCount) && (i < properties.size()); ++i) {
        pProperties[i] = properties[i];
      }
      if ((return_value == VK_INCOMPLETE) && (*pPropertyCount >= properties.size())) {
        return_value = VK_SUCCESS;
      }
    }
    *pPropertyCount = static_cast<uint32_t>(properties.size());
  }
  return return_value;

#elif BUILD_FOR_VULKAN_LAYER

  return EnumerateLayerHelper(1, pPropertyCount, pProperties);

#endif
}

VkResult recExecWrap_vkEnumerateInstanceExtensionProperties(const char* pLayerName,
                                                            uint32_t* pPropertyCount,
                                                            VkExtensionProperties* pProperties) {
#ifdef BUILD_FOR_VULKAN_INTERCEPTOR

  CVkDriver& drvVk = CGitsPluginVulkan::RecorderWrapper().Drivers();
  VkResult return_value =
      drvVk.vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);

  if (CGitsPluginVulkan::Configuration().common.recorder.enabled &&
      !CGitsPluginVulkan::_recorderFinished &&
      !CGitsPluginVulkan::Configuration().vulkan.shared.suppressExtensions.empty()) {
    uint32_t propertyCount = 0;
    std::vector<VkExtensionProperties> properties;

    if ((drvVk.vkEnumerateInstanceExtensionProperties(pLayerName, &propertyCount, nullptr) ==
         VK_SUCCESS) &&
        (propertyCount > 0)) {
      properties.resize(propertyCount);

      if (drvVk.vkEnumerateInstanceExtensionProperties(pLayerName, &propertyCount,
                                                       properties.data()) == VK_SUCCESS) {
        properties.erase(
            std::remove_if(
                properties.begin(), properties.end(),
                [](VkExtensionProperties& element) {
                  auto& suppressExtensions =
                      CGitsPluginVulkan::Configuration().vulkan.shared.suppressExtensions;
                  return std::find(suppressExtensions.begin(), suppressExtensions.end(),
                                   element.extensionName) != suppressExtensions.end();
                }),
            properties.end());
      }
    }

    if (pProperties != nullptr) {
      for (uint32_t i = 0; (i < *pPropertyCount) && (i < properties.size()); ++i) {
        pProperties[i] = properties[i];
      }
      if ((return_value == VK_INCOMPLETE) && (*pPropertyCount >= properties.size())) {
        return_value = VK_SUCCESS;
      }
    }
    *pPropertyCount = static_cast<uint32_t>(properties.size());
  }
  return return_value;

#elif BUILD_FOR_VULKAN_LAYER

  if (pLayerName && !strcmp(pLayerName, VulkanGITSRecorderLayer.layerName)) {
    *pPropertyCount = 0;
    return VK_SUCCESS;
  }
  return VK_ERROR_LAYER_NOT_PRESENT;

#endif
}

VkResult recExecWrap_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice,
                                                      uint32_t* pPropertyCount,
                                                      VkLayerProperties* pProperties) {
#ifdef BUILD_FOR_VULKAN_INTERCEPTOR

  CVkDriver& drvVk = CGitsPluginVulkan::RecorderWrapper().Drivers();
  VkResult return_value =
      drvVk.vkEnumerateDeviceLayerProperties(physicalDevice, pPropertyCount, pProperties);

  if (CGitsPluginVulkan::Configuration().common.recorder.enabled &&
      !CGitsPluginVulkan::_recorderFinished &&
      !CGitsPluginVulkan::Configuration().vulkan.shared.suppressLayers.empty()) {
    uint32_t propertyCount = 0;
    std::vector<VkLayerProperties> properties;

    if ((drvVk.vkEnumerateDeviceLayerProperties(physicalDevice, &propertyCount, nullptr) ==
         VK_SUCCESS) &&
        (propertyCount > 0)) {
      properties.resize(propertyCount);

      if (drvVk.vkEnumerateDeviceLayerProperties(physicalDevice, &propertyCount,
                                                 properties.data()) == VK_SUCCESS) {
        properties.erase(
            std::remove_if(properties.begin(), properties.end(),
                           [](VkLayerProperties& element) {
                             auto& suppressLayers =
                                 CGitsPluginVulkan::Configuration().vulkan.shared.suppressLayers;
                             return std::find(suppressLayers.begin(), suppressLayers.end(),
                                              element.layerName) != suppressLayers.end();
                           }),
            properties.end());
      }
    }

    if (pProperties != nullptr) {
      for (uint32_t i = 0; (i < *pPropertyCount) && (i < properties.size()); ++i) {
        pProperties[i] = properties[i];
      }
      if ((return_value == VK_INCOMPLETE) && (*pPropertyCount == properties.size())) {
        return_value = VK_SUCCESS;
      }
    }
    *pPropertyCount = static_cast<uint32_t>(properties.size());
  }
  return return_value;

#elif BUILD_FOR_VULKAN_LAYER

  return EnumerateLayerHelper(1, pPropertyCount, pProperties);

#endif
}

VkResult recExecWrap_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                          const char* pLayerName,
                                                          uint32_t* pPropertyCount,
                                                          VkExtensionProperties* pProperties) {
#ifdef BUILD_FOR_VULKAN_LAYER

  if (pLayerName && !strcmp(pLayerName, VulkanGITSRecorderLayer.layerName)) {
    *pPropertyCount = 0;
    return VK_SUCCESS;
  }

#endif

  CVkDriver& drvVk = CGitsPluginVulkan::RecorderWrapper().Drivers();
  VkResult return_value = drvVk.vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName,
                                                                     pPropertyCount, pProperties);

  if (CGitsPluginVulkan::Configuration().common.recorder.enabled &&
      !CGitsPluginVulkan::_recorderFinished &&
      !CGitsPluginVulkan::Configuration().vulkan.shared.suppressExtensions.empty()) {
    uint32_t propertyCount = 0;
    std::vector<VkExtensionProperties> properties;

    if ((drvVk.vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, &propertyCount,
                                                    nullptr) == VK_SUCCESS) &&
        (propertyCount > 0)) {
      properties.resize(propertyCount);

      if (drvVk.vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, &propertyCount,
                                                     properties.data()) == VK_SUCCESS) {
        properties.erase(
            std::remove_if(
                properties.begin(), properties.end(),
                [](VkExtensionProperties& element) {
                  auto& suppressExtensions =
                      CGitsPluginVulkan::Configuration().vulkan.shared.suppressExtensions;
                  return std::find(suppressExtensions.begin(), suppressExtensions.end(),
                                   element.extensionName) != suppressExtensions.end();
                }),
            properties.end());
      }
    }

    if (pProperties != nullptr) {
      for (uint32_t i = 0; (i < *pPropertyCount) && (i < properties.size()); ++i) {
        pProperties[i] = properties[i];
      }
      if ((return_value == VK_INCOMPLETE) && (*pPropertyCount == properties.size())) {
        return_value = VK_SUCCESS;
      }
    }
    *pPropertyCount = static_cast<uint32_t>(properties.size());
  }
  return return_value;
}

VkResult recExecWrap_vkCreateImage(VkDevice device,
                                   const VkImageCreateInfo* pCreateInfo,
                                   const VkAllocationCallbacks* pAllocator,
                                   VkImage* pImage) {
  // Global changes - propagate to recorded streams
  {
    VkImageCreateInfo* originalCreateInfo = const_cast<VkImageCreateInfo*>(pCreateInfo);
    originalCreateInfo->usage |= static_cast<VkImageUsageFlags>(
        CGitsPluginVulkan::Configuration().vulkan.recorder.addImageUsageFlags);

    // Handle offscreen applications
    if (CGitsPluginVulkan::RecorderWrapper().IsImagePresentable(pCreateInfo)) {
      originalCreateInfo->usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
  }

  // Local changes - impact only current execution
  VkImageCreateInfo localCreateInfo = *pCreateInfo;

  if (CGitsPluginVulkan::Configuration().common.recorder.enabled) {
    if ((CGitsPluginVulkan::Configuration().vulkan.recorder.mode != TVulkanRecorderMode::ALL) &&
        (CGitsPluginVulkan::Configuration().vulkan.recorder.crossPlatformStateRestoration.images)) {
      localCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
  }
  return CGitsPluginVulkan::RecorderWrapper().Drivers().vkCreateImage(device, &localCreateInfo,
                                                                      pAllocator, pImage);
}

namespace {

VkBufferCreateInfo OverwriteBufferCreateInfo(const VkBufferCreateInfo* pCreateInfo) {
  // Global changes - propagate to recorded streams
  if (!CGitsPluginVulkan::RecorderWrapper().IsPaused()) {
    VkBufferCreateInfo* originalCreateInfo = const_cast<VkBufferCreateInfo*>(pCreateInfo);

    // Make sure that a device address for buffers used as a storage for AS can be retrieved and saved in a stream
    if (isBitSet(originalCreateInfo->usage,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR)) {
      originalCreateInfo->usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    if (isBitSet(originalCreateInfo->usage, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) &&
        CGitsPluginVulkan::Configuration()
            .vulkan.recorder.useCaptureReplayFeaturesForBuffersAndAccelerationStructures) {
      originalCreateInfo->flags |= VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
    }
    originalCreateInfo->usage |= static_cast<VkBufferUsageFlags>(
        CGitsPluginVulkan::Configuration().vulkan.recorder.addBufferUsageFlags);
  }

  // Local changes - impact only current execution
  {
    VkBufferCreateInfo localCreateInfo = *pCreateInfo;

    if (CGitsPluginVulkan::Configuration().common.recorder.enabled &&
        (CGitsPluginVulkan::Configuration().vulkan.recorder.mode != TVulkanRecorderMode::ALL) &&
        (TBufferStateRestoration::NONE !=
         CGitsPluginVulkan::Configuration()
             .vulkan.recorder.crossPlatformStateRestoration.buffers)) {
      localCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    return localCreateInfo;
  }
}

} // namespace

VkResult recExecWrap_vkCreateBuffer(VkDevice device,
                                    const VkBufferCreateInfo* pCreateInfo,
                                    const VkAllocationCallbacks* pAllocator,
                                    VkBuffer* pBuffer) {
  auto localCreateInfo = OverwriteBufferCreateInfo(pCreateInfo);
  return CGitsPluginVulkan::RecorderWrapper().Drivers().vkCreateBuffer(device, &localCreateInfo,
                                                                       pAllocator, pBuffer);
}

VkResult recExecWrap_vkCreateSwapchainKHR(VkDevice device,
                                          const VkSwapchainCreateInfoKHR* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator,
                                          VkSwapchainKHR* pSwapchain) {
  // Global changes - propagate to recorded streams
  {
    VkSwapchainCreateInfoKHR* originalCreateInfo =
        const_cast<VkSwapchainCreateInfoKHR*>(pCreateInfo);
    originalCreateInfo->imageUsage |= static_cast<VkImageUsageFlags>(
        CGitsPluginVulkan::Configuration().vulkan.recorder.addImageUsageFlags);
  }

  // Local changes - impact only current execution
  VkSwapchainCreateInfoKHR localCreateInfo = *pCreateInfo;

  if (CGitsPluginVulkan::Configuration().common.recorder.enabled) {
    if ((CGitsPluginVulkan::Configuration().vulkan.recorder.mode != TVulkanRecorderMode::ALL) &&
        CGitsPluginVulkan::Configuration().vulkan.recorder.crossPlatformStateRestoration.images) {
      localCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
  }
  return CGitsPluginVulkan::RecorderWrapper().Drivers().vkCreateSwapchainKHR(
      device, &localCreateInfo, pAllocator, pSwapchain);
}

VkResult recExecWrap_vkCreateRayTracingPipelinesKHR(
    VkDevice device,
    VkDeferredOperationKHR deferredOperation,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines) {
  if (CGitsPluginVulkan::Configuration()
          .vulkan.recorder.useCaptureReplayFeaturesForRayTracingPipelines) {
    for (uint32_t i = 0; i < createInfoCount; ++i) {
      auto& originalCreateInfo = const_cast<VkRayTracingPipelineCreateInfoKHR&>(pCreateInfos[i]);
      originalCreateInfo.flags |=
          VK_PIPELINE_CREATE_RAY_TRACING_SHADER_GROUP_HANDLE_CAPTURE_REPLAY_BIT_KHR;
    }
  }
  return CGitsPluginVulkan::RecorderWrapper().Drivers().vkCreateRayTracingPipelinesKHR(
      device, VK_NULL_HANDLE /* deferredOperation */, pipelineCache, createInfoCount, pCreateInfos,
      pAllocator, pPipelines);
}

namespace {

VkDeviceSize GetOverriddenMemorySize(VkDeviceSize base,
                                     MemorySizeRequirementOverride const& modifier) {
  if ((modifier.percent > 0) || (modifier.fixedAmount > 0)) {
    base = base * (100 + modifier.percent) * 0.01 + modifier.fixedAmount;

    {
      // Round up to a page size
      TODO("Join this code and GetVirtualMemoryPageSize() function to avoid duplication.")
      TODO("Is this the right thing to do on non-Intel GPUs?")

      static const uint32_t pageSize = []() {
#ifdef GITS_PLATFORM_WINDOWS
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return si.dwPageSize;
#else
        return getpagesize();
#endif
      }();

      base = (base + pageSize - 1) - (base + pageSize - 1) % pageSize;
    }
  }
  return base;
}

void ProcessImageMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) {
  auto& config = CGitsPluginVulkan::Configuration();
  if (config.common.recorder.enabled) {
    pMemoryRequirements->size = GetOverriddenMemorySize(
        pMemoryRequirements->size, config.vulkan.recorder.increaseImageMemorySizeRequirement);

    if (config.vulkan.recorder.memoryOffsetAlignmentOverride.images > 0) {
      if ((config.vulkan.recorder.memoryOffsetAlignmentOverride.images <
           pMemoryRequirements->alignment) ||
          (config.vulkan.recorder.memoryOffsetAlignmentOverride.images %
               pMemoryRequirements->alignment !=
           0)) {
        LOG_WARNING
            << "Override memory offset alignment: "
            << config.vulkan.recorder.memoryOffsetAlignmentOverride.images
            << " is lower and/or not a multiple of the required image memory offset alignment: "
            << pMemoryRequirements->alignment << ". Override ignored!";
      } else {
        pMemoryRequirements->alignment =
            config.vulkan.recorder.memoryOffsetAlignmentOverride.images;
      }
    }
  }
}

void ProcessBufferMemoryRequirements(VkMemoryRequirements* pMemoryRequirements) {
  auto& config = CGitsPluginVulkan::Configuration();

  if (config.common.recorder.enabled &&
      (config.vulkan.recorder.memoryOffsetAlignmentOverride.buffers > 0)) {
    if ((config.vulkan.recorder.memoryOffsetAlignmentOverride.buffers <
         pMemoryRequirements->alignment) ||
        (config.vulkan.recorder.memoryOffsetAlignmentOverride.buffers %
             pMemoryRequirements->alignment !=
         0)) {
      LOG_WARNING
          << "Override memory offset alignment: "
          << config.vulkan.recorder.memoryOffsetAlignmentOverride.buffers
          << " is lower and/or not a multiple of the required buffer memory offset alignment: "
          << pMemoryRequirements->alignment << ". Override ignored!";
    } else {
      pMemoryRequirements->alignment = config.vulkan.recorder.memoryOffsetAlignmentOverride.buffers;
    }
  }
}

void ProcessPhysicalDeviceProperties(VkPhysicalDeviceProperties* pProperties) {
  auto& config = CGitsPluginVulkan::Configuration();

  if (config.common.recorder.enabled &&
      (config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors > 0)) {
    if ((config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors <
         pProperties->limits.minTexelBufferOffsetAlignment) ||
        (config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors %
             pProperties->limits.minTexelBufferOffsetAlignment !=
         0)) {
      LOG_WARNING
          << "Override memory offset alignment: "
          << config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors
          << " is lower and/or not a multiple of the required minTexelBufferOffsetAlignment: "
          << pProperties->limits.minTexelBufferOffsetAlignment << ". Override ignored!";
    } else {
      pProperties->limits.minTexelBufferOffsetAlignment =
          config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors;
    }

    if ((config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors <
         pProperties->limits.minUniformBufferOffsetAlignment) ||
        (config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors %
             pProperties->limits.minUniformBufferOffsetAlignment !=
         0)) {
      LOG_WARNING
          << "Override memory offset alignment: "
          << config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors
          << " is lower and/or not a multiple of the required minUniformBufferOffsetAlignment: "
          << pProperties->limits.minUniformBufferOffsetAlignment << ". Override ignored!";
    } else {
      pProperties->limits.minUniformBufferOffsetAlignment =
          config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors;
    }

    if ((config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors <
         pProperties->limits.minStorageBufferOffsetAlignment) ||
        (config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors %
             pProperties->limits.minStorageBufferOffsetAlignment !=
         0)) {
      LOG_WARNING
          << "Override memory offset alignment: "
          << config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors
          << " is lower and/or not a multiple of the required minStorageBufferOffsetAlignment: "
          << pProperties->limits.minStorageBufferOffsetAlignment << ". Override ignored!";
    } else {
      pProperties->limits.minStorageBufferOffsetAlignment =
          config.vulkan.recorder.memoryOffsetAlignmentOverride.descriptors;
    }
  }
}
} // namespace

void recExecWrap_vkGetImageMemoryRequirements(VkDevice device,
                                              VkImage image,
                                              VkMemoryRequirements* pMemoryRequirements) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetImageMemoryRequirements(device, image,
                                                                              pMemoryRequirements);
  ProcessImageMemoryRequirements(pMemoryRequirements);
}

void recExecWrap_vkGetImageMemoryRequirements2(VkDevice device,
                                               const VkImageMemoryRequirementsInfo2* pInfo,
                                               VkMemoryRequirements2* pMemoryRequirements) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetImageMemoryRequirements2(device, pInfo,
                                                                               pMemoryRequirements);
  ProcessImageMemoryRequirements(&pMemoryRequirements->memoryRequirements);
}

void recExecWrap_vkGetImageMemoryRequirements2KHR(VkDevice device,
                                                  const VkImageMemoryRequirementsInfo2* pInfo,
                                                  VkMemoryRequirements2* pMemoryRequirements) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetImageMemoryRequirements2KHR(
      device, pInfo, pMemoryRequirements);
  ProcessImageMemoryRequirements(&pMemoryRequirements->memoryRequirements);
}

void recExecWrap_vkGetBufferMemoryRequirements(VkDevice device,
                                               VkBuffer buffer,
                                               VkMemoryRequirements* pMemoryRequirements) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetBufferMemoryRequirements(device, buffer,
                                                                               pMemoryRequirements);
  ProcessBufferMemoryRequirements(pMemoryRequirements);
}

void recExecWrap_vkGetBufferMemoryRequirements2(VkDevice device,
                                                const VkBufferMemoryRequirementsInfo2* pInfo,
                                                VkMemoryRequirements2* pMemoryRequirements) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetBufferMemoryRequirements2(
      device, pInfo, pMemoryRequirements);
  ProcessBufferMemoryRequirements(&pMemoryRequirements->memoryRequirements);
}

void recExecWrap_vkGetBufferMemoryRequirements2KHR(VkDevice device,
                                                   const VkBufferMemoryRequirementsInfo2* pInfo,
                                                   VkMemoryRequirements2* pMemoryRequirements) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetBufferMemoryRequirements2KHR(
      device, pInfo, pMemoryRequirements);
  ProcessBufferMemoryRequirements(&pMemoryRequirements->memoryRequirements);
}

void recExecWrap_vkGetDeviceBufferMemoryRequirements(VkDevice device,
                                                     const VkDeviceBufferMemoryRequirements* pInfo,
                                                     VkMemoryRequirements2* pMemoryRequirements) {
  auto memoryReqsInfo = *pInfo;
  auto createInfo = OverwriteBufferCreateInfo(memoryReqsInfo.pCreateInfo);
  memoryReqsInfo.pCreateInfo = &createInfo;

  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetDeviceBufferMemoryRequirements(
      device, &memoryReqsInfo, pMemoryRequirements);
  ProcessBufferMemoryRequirements(&pMemoryRequirements->memoryRequirements);
}

void recExecWrap_vkGetDeviceBufferMemoryRequirementsKHR(
    VkDevice device,
    const VkDeviceBufferMemoryRequirements* pInfo,
    VkMemoryRequirements2* pMemoryRequirements) {
  auto memoryReqsInfo = *pInfo;
  auto createInfo = OverwriteBufferCreateInfo(memoryReqsInfo.pCreateInfo);
  memoryReqsInfo.pCreateInfo = &createInfo;

  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetDeviceBufferMemoryRequirementsKHR(
      device, &memoryReqsInfo, pMemoryRequirements);
  ProcessBufferMemoryRequirements(&pMemoryRequirements->memoryRequirements);
}

void recExecWrap_vkGetAccelerationStructureBuildSizesKHR(
    VkDevice device,
    VkAccelerationStructureBuildTypeKHR buildType,
    const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
    const uint32_t* pMaxPrimitiveCounts,
    VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetAccelerationStructureBuildSizesKHR(
      device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);

  auto& config = CGitsPluginVulkan::Configuration();
  if (config.common.recorder.enabled) {
    pSizeInfo->accelerationStructureSize = GetOverriddenMemorySize(
        pSizeInfo->accelerationStructureSize,
        config.vulkan.recorder.increaseAccelerationStructureMemorySizeRequirement
            .accelerationStructureSize);
    pSizeInfo->buildScratchSize = GetOverriddenMemorySize(
        pSizeInfo->buildScratchSize,
        config.vulkan.recorder.increaseAccelerationStructureMemorySizeRequirement.buildScratchSize);
    pSizeInfo->updateScratchSize = GetOverriddenMemorySize(
        pSizeInfo->updateScratchSize,
        config.vulkan.recorder.increaseAccelerationStructureMemorySizeRequirement
            .updateScratchSize);
  }
}

void recExecWrap_vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice,
                                               VkPhysicalDeviceProperties* pProperties) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetPhysicalDeviceProperties(physicalDevice,
                                                                               pProperties);
  ProcessPhysicalDeviceProperties(pProperties);
}

void recExecWrap_vkGetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice,
                                                VkPhysicalDeviceProperties2* pProperties) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetPhysicalDeviceProperties2(physicalDevice,
                                                                                pProperties);
  ProcessPhysicalDeviceProperties(&pProperties->properties);
}

void recExecWrap_vkGetPhysicalDeviceProperties2KHR(VkPhysicalDevice physicalDevice,
                                                   VkPhysicalDeviceProperties2* pProperties) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetPhysicalDeviceProperties2KHR(physicalDevice,
                                                                                   pProperties);
  ProcessPhysicalDeviceProperties(&pProperties->properties);
}

void recExecWrap_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice,
                                             VkPhysicalDeviceFeatures* pFeatures) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetPhysicalDeviceFeatures(physicalDevice,
                                                                             pFeatures);
  CGitsPluginVulkan::RecorderWrapper().SuppressPhysicalDeviceFeatures(
      CGitsPluginVulkan::Configuration().vulkan.shared.suppressPhysicalDeviceFeatures, pFeatures);
}

void recExecWrap_vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice,
                                              VkPhysicalDeviceFeatures2* pFeatures) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetPhysicalDeviceFeatures2(physicalDevice,
                                                                              pFeatures);
  CGitsPluginVulkan::RecorderWrapper().SuppressPhysicalDeviceFeatures(
      CGitsPluginVulkan::Configuration().vulkan.shared.suppressPhysicalDeviceFeatures,
      &pFeatures->features);
}

void recExecWrap_vkGetPhysicalDeviceFeatures2KHR(VkPhysicalDevice physicalDevice,
                                                 VkPhysicalDeviceFeatures2* pFeatures) {
  CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetPhysicalDeviceFeatures2KHR(physicalDevice,
                                                                                 pFeatures);
  CGitsPluginVulkan::RecorderWrapper().SuppressPhysicalDeviceFeatures(
      CGitsPluginVulkan::Configuration().vulkan.shared.suppressPhysicalDeviceFeatures,
      &pFeatures->features);
}

namespace {

void UpdateUsage(VkImageUsageFlags& usageFlags) {
  auto& vkCfg = CGitsPluginVulkan::Configuration().vulkan.recorder;

  usageFlags |= static_cast<VkImageUsageFlags>(vkCfg.addImageUsageFlags);

  if (CGitsPluginVulkan::Configuration().common.recorder.enabled) {
    if ((vkCfg.mode != TVulkanRecorderMode::ALL) && vkCfg.crossPlatformStateRestoration.images) {
      usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
  }
}

} // namespace

VkResult recExecWrap_vkGetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkImageCreateFlags flags,
    VkImageFormatProperties* pImageFormatProperties) {
  UpdateUsage(usage);

  return CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetPhysicalDeviceImageFormatProperties(
      physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
}

VkResult recExecWrap_vkGetPhysicalDeviceImageFormatProperties2(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
    VkImageFormatProperties2* pImageFormatProperties) {
  auto formatInfo = *pImageFormatInfo;
  UpdateUsage(formatInfo.usage);

  return CGitsPluginVulkan::RecorderWrapper().Drivers().vkGetPhysicalDeviceImageFormatProperties2(
      physicalDevice, &formatInfo, pImageFormatProperties);
}

VkResult recExecWrap_vkGetPhysicalDeviceImageFormatProperties2KHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
    VkImageFormatProperties2* pImageFormatProperties) {
  auto formatInfo = *pImageFormatInfo;
  UpdateUsage(formatInfo.usage);

  return CGitsPluginVulkan::RecorderWrapper()
      .Drivers()
      .vkGetPhysicalDeviceImageFormatProperties2KHR(physicalDevice, &formatInfo,
                                                    pImageFormatProperties);
}

} // namespace Vulkan
} // namespace gits
