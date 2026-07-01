// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "replayCustomizationLayer.h"
#include "playerManager.h"
#include "swapchainImageSyncService.h"
#include "configurator.h"

#include <thread>
#include <chrono>
#include <cstdint>
#include <vector>

namespace gits {
namespace vulkan {

thread_local VkResult ReplayCustomizationLayer::tl_recorderReturnValue{VK_SUCCESS};
thread_local uint64_t ReplayCustomizationLayer::tl_recorderSemaphoreCounterValue{0};

void ReplayCustomizationLayer::Post(vkCreateInstanceCommand& command) {
  m_Manager.LoadInstanceFunctions(*command.m_pInstance.Value);
}

void ReplayCustomizationLayer::Post(vkCreateDeviceCommand& command) {
  void* dispatchKey = *reinterpret_cast<void**>(command.m_physicalDevice.Value);
  m_Manager.LoadDeviceFunctions(dispatchKey, *command.m_pDevice.Value);
}

void ReplayCustomizationLayer::Post(vkGetDeviceQueueCommand& command) {
  m_Manager.GetSwapchainImageSyncService().TrackQueue(command.m_device.Value,
                                                      *command.m_pQueue.Value);
}

void ReplayCustomizationLayer::Post(vkGetDeviceQueue2Command& command) {
  m_Manager.GetSwapchainImageSyncService().TrackQueue(command.m_device.Value,
                                                      *command.m_pQueue.Value);
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
void ReplayCustomizationLayer::Pre(vkCreateWin32SurfaceKHRCommand& command) {
  auto& windowService = m_Manager.GetWindowService();
  uint64_t currentHandle = windowService.GetCurrentWindowHandle(
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->hwnd));
  uint64_t currentInstance = windowService.GetCurrentInstance(
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->hinstance));
  if (currentHandle && currentInstance) {
    command.m_pCreateInfo.Value->hwnd = reinterpret_cast<HWND>(currentHandle);
    command.m_pCreateInfo.Value->hinstance = reinterpret_cast<HINSTANCE>(currentInstance);
  }
}
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
void ReplayCustomizationLayer::Pre(vkCreateXlibSurfaceKHRCommand& command) {
  auto& windowService = m_Manager.GetWindowService();
  uint64_t currentDisplay = windowService.GetCurrentWindowHandle(
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->dpy));
  uint64_t currentWindow = windowService.GetCurrentInstance(
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->window));
  if (currentDisplay && currentWindow) {
    command.m_pCreateInfo.Value->dpy = reinterpret_cast<Display*>(currentDisplay);
    command.m_pCreateInfo.Value->window = reinterpret_cast<Window>(currentWindow);
  }
}

void ReplayCustomizationLayer::Pre(vkGetPhysicalDeviceXcbPresentationSupportKHRCommand& command) {
  command.m_Skip = true;
}
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
void ReplayCustomizationLayer::Pre(vkCreateXcbSurfaceKHRCommand& command) {
  auto& windowService = m_Manager.GetWindowService();
  uint64_t currentConnection = windowService.GetCurrentWindowHandle(
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->connection));
  uint64_t currentWindow =
      windowService.GetCurrentInstance(static_cast<uint64_t>(command.m_pCreateInfo.Value->window));
  if (currentConnection && currentWindow) {
    command.m_pCreateInfo.Value->connection =
        reinterpret_cast<xcb_connection_t*>(currentConnection);
    command.m_pCreateInfo.Value->window = static_cast<xcb_window_t>(currentWindow);
  }
}
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
void ReplayCustomizationLayer::Pre(vkCreateWaylandSurfaceKHRCommand& command) {
  auto& windowService = m_Manager.GetWindowService();
  uint64_t currentDisplay = windowService.GetCurrentWindowHandle(
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->display));
  uint64_t currentWindow = windowService.GetCurrentInstance(
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->surface));
  if (currentDisplay && currentWindow) {
    command.m_pCreateInfo.Value->display = reinterpret_cast<wl_display*>(currentDisplay);
    command.m_pCreateInfo.Value->surface = reinterpret_cast<wl_surface*>(currentWindow);
  }
}
#endif

// Offscreen replay swapchain handling. The contract is: pre-acquire all
// swapchain images once, and never issue a real present.
void ReplayCustomizationLayer::Post(vkCreateSwapchainKHRCommand& command) {
  if (!Configurator::Get().common.player.renderOffscreen) {
    return;
  }
  if (command.m_Return.Value != VK_SUCCESS || command.m_pSwapchain.Value == nullptr) {
    return;
  }
  VkDevice device = command.m_device.Value;
  VkSwapchainKHR swapchain = *command.m_pSwapchain.Value;
  auto& dt = m_Manager.GetDeviceDispatchTable(device);

  uint32_t imageCount = 0;
  dt.vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
  // Acquire every image (no semaphore/fence). Offscreen replay never presents, so
  // the images are never released and stay valid for the whole replay.
  for (uint32_t i = 0; i < imageCount; ++i) {
    uint32_t imageIndex = 0;
    auto ret = dt.vkAcquireNextImageKHR(device, swapchain, 3000000000ULL, VK_NULL_HANDLE,
                                        VK_NULL_HANDLE, &imageIndex);
    if (ret != VK_SUCCESS && ret != VK_SUBOPTIMAL_KHR) {
      LOG_WARNING
          << "ReplayCustomizationLayer: renderOffscreen: vkAcquireNextImageKHR failed swapchain "
             "image "
          << i << " with return value " << ret;
    }
  }
}

void ReplayCustomizationLayer::Pre(vkQueuePresentKHRCommand& command) {
  if (!Configurator::Get().common.player.renderOffscreen) {
    return;
  }
  // Consume the present's wait semaphores with an empty submit (so downstream work
  // is not left blocked on them), then skip the real present.
  VkPresentInfoKHR* presentInfo = command.m_pPresentInfo.Value;
  if (presentInfo != nullptr && presentInfo->waitSemaphoreCount > 0 &&
      presentInfo->pWaitSemaphores != nullptr) {
    std::vector<VkPipelineStageFlags> waitStages(presentInfo->waitSemaphoreCount,
                                                 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = presentInfo->waitSemaphoreCount;
    submitInfo.pWaitSemaphores = presentInfo->pWaitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages.data();
    m_Manager.GetDeviceDispatchTable(command.m_queue.Value)
        .vkQueueSubmit(command.m_queue.Value, 1, &submitInfo, VK_NULL_HANDLE);
    // Clear so any other layer (e.g. screenshots) does not wait on them again.
    presentInfo->waitSemaphoreCount = 0;
    presentInfo->pWaitSemaphores = nullptr;
  }
  command.m_Skip = true;
}

void ReplayCustomizationLayer::Post(vkAllocateMemoryCommand& command) {
  m_Manager.GetMapTrackingService().StoreAllocationInfo(
      command.m_device.Key, command.m_pMemory.Key, command.m_pAllocateInfo.Value->allocationSize,
      command.m_pAllocateInfo.Value->memoryTypeIndex);
}

void ReplayCustomizationLayer::Post(vkMapMemoryCommand& command) {
  m_Manager.GetMapTrackingService().StoreData(command.m_device.Key, command.m_memory.Key,
                                              command.m_ppData.Data, command.m_size.Value);
}

void ReplayCustomizationLayer::Pre(vkUnmapMemoryCommand& command) {
  m_Manager.GetMapTrackingService().RemoveData(command.m_device.Key, command.m_memory.Key);
}

void ReplayCustomizationLayer::Post(vkMapMemory2Command& command) {
  m_Manager.GetMapTrackingService().StoreData(
      command.m_device.Key, command.m_pMemoryMapInfo.HandleKeys[0], command.m_ppData.Data,
      command.m_pMemoryMapInfo.Value->size);
}

void ReplayCustomizationLayer::Post(vkMapMemory2KHRCommand& command) {
  m_Manager.GetMapTrackingService().StoreData(
      command.m_device.Key, command.m_pMemoryMapInfo.HandleKeys[0], command.m_ppData.Data,
      command.m_pMemoryMapInfo.Value->size);
}

void ReplayCustomizationLayer::Pre(vkUnmapMemory2Command& command) {
  m_Manager.GetMapTrackingService().RemoveData(command.m_device.Key,
                                               command.m_pMemoryUnmapInfo.HandleKeys[0]);
}

void ReplayCustomizationLayer::Pre(vkUnmapMemory2KHRCommand& command) {
  m_Manager.GetMapTrackingService().RemoveData(command.m_device.Key,
                                               command.m_pMemoryUnmapInfo.HandleKeys[0]);
}

// vkGetFenceStatus

void ReplayCustomizationLayer::Pre(vkGetFenceStatusCommand& command) {
  tl_recorderReturnValue = command.m_Return.Value;
}

void ReplayCustomizationLayer::Post(vkGetFenceStatusCommand& command) {
  // Catch-up only when the recording observed VK_SUCCESS, the live device says
  // otherwise, AND the fence actually has a pending signal.  The pending guard
  // mirrors legacy (vulkanPlayerRunWrap.h:1000-1001): a fence with no pending
  // signal (its signalling submit was before the subcapture cut, or it was
  // reset and not resubmitted) will never become signalled in this replay
  // range, so we must not wait on it -- doing so either deadlocks or stalls on
  // every one of this stream's thousands of polls.
  if (command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != tl_recorderReturnValue &&
      m_Manager.GetFencePendingSignalService().IsPending(command.m_fence.Value)) {
    auto& dispatchTable = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
    VkFence fence = command.m_fence.Value;
    // The fence has a pending signal, so it will become signalled; wait
    // indefinitely then re-query, exactly matching legacy
    // vkGetFenceStatus_WRAPRUN (vulkanPlayerRunWrap.h:1002-1004).
    dispatchTable.vkWaitForFences(command.m_device.Value, 1, &fence, VK_TRUE, UINT64_MAX);
    command.m_Return.Value = dispatchTable.vkGetFenceStatus(command.m_device.Value, fence);
  }
}

// vkGetEventStatus

void ReplayCustomizationLayer::Pre(vkGetEventStatusCommand& command) {
  tl_recorderReturnValue = command.m_Return.Value;
}

void ReplayCustomizationLayer::Post(vkGetEventStatusCommand& command) {
  while (command.m_Return.Value != VK_EVENT_SET &&
         command.m_Return.Value != tl_recorderReturnValue) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    auto& dispatchTable = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
    command.m_Return.Value =
        dispatchTable.vkGetEventStatus(command.m_device.Value, command.m_event.Value);
  }
}

// vkGetSemaphoreCounterValue

void ReplayCustomizationLayer::Pre(vkGetSemaphoreCounterValueCommand& command) {
  tl_recorderReturnValue = command.m_Return.Value;
  tl_recorderSemaphoreCounterValue =
      (command.m_pValue.Value != nullptr) ? *command.m_pValue.Value : 0;
}

void ReplayCustomizationLayer::Post(vkGetSemaphoreCounterValueCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || tl_recorderReturnValue != VK_SUCCESS) {
    return;
  }
  uint64_t currentValue = (command.m_pValue.Value != nullptr) ? *command.m_pValue.Value : 0;
  if (currentValue < tl_recorderSemaphoreCounterValue) {
    VkSemaphore semaphore = command.m_semaphore.Value;
    VkSemaphoreWaitInfo waitInfo{};
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    waitInfo.pNext = nullptr;
    waitInfo.flags = 0;
    waitInfo.semaphoreCount = 1;
    waitInfo.pSemaphores = &semaphore;
    waitInfo.pValues = &tl_recorderSemaphoreCounterValue;
    auto& dispatchTable = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
    dispatchTable.vkWaitSemaphores(command.m_device.Value, &waitInfo, 0xFFFFFFFFFFFFFFFF);
  }
}

// vkGetSemaphoreCounterValueKHR

void ReplayCustomizationLayer::Pre(vkGetSemaphoreCounterValueKHRCommand& command) {
  tl_recorderReturnValue = command.m_Return.Value;
  tl_recorderSemaphoreCounterValue =
      (command.m_pValue.Value != nullptr) ? *command.m_pValue.Value : 0;
}

void ReplayCustomizationLayer::Post(vkGetSemaphoreCounterValueKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || tl_recorderReturnValue != VK_SUCCESS) {
    return;
  }
  uint64_t currentValue = (command.m_pValue.Value != nullptr) ? *command.m_pValue.Value : 0;
  if (currentValue < tl_recorderSemaphoreCounterValue) {
    VkSemaphore semaphore = command.m_semaphore.Value;
    VkSemaphoreWaitInfo waitInfo{};
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    waitInfo.pNext = nullptr;
    waitInfo.flags = 0;
    waitInfo.semaphoreCount = 1;
    waitInfo.pSemaphores = &semaphore;
    waitInfo.pValues = &tl_recorderSemaphoreCounterValue;
    auto& dispatchTable = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
    dispatchTable.vkWaitSemaphoresKHR(command.m_device.Value, &waitInfo, 0xFFFFFFFFFFFFFFFF);
  }
}

// vkGetQueryPoolResults

void ReplayCustomizationLayer::Pre(vkGetQueryPoolResultsCommand& command) {
  tl_recorderReturnValue = command.m_Return.Value;
}

void ReplayCustomizationLayer::Post(vkGetQueryPoolResultsCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != tl_recorderReturnValue) {
    auto& dispatchTable = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
    dispatchTable.vkDeviceWaitIdle(command.m_device.Value);
  }
}

// vkWaitForFences

void ReplayCustomizationLayer::Pre(vkWaitForFencesCommand& command) {
  tl_recorderReturnValue = command.m_Return.Value;
}

void ReplayCustomizationLayer::Post(vkWaitForFencesCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != tl_recorderReturnValue) {
    // Only wait on fences that have a pending signal, mirroring legacy
    // vkWaitForFences_WRAPRUN which filters the array on fenceUsed
    // (vulkanPlayerRunWrap.h:1420-1425).  Fences with no pending signal will
    // never become signalled in this replay range; waiting on them deadlocks /
    // stalls, so they are dropped from the catch-up wait.
    std::vector<VkFence> pending;
    for (uint32_t i = 0; i < command.m_pFences.Size; ++i) {
      if (m_Manager.GetFencePendingSignalService().IsPending(command.m_pFences.Value[i])) {
        pending.push_back(command.m_pFences.Value[i]);
      }
    }
    if (pending.empty()) {
      // Nothing to wait for: adopt the recorded return value (legacy leaves
      // recRetVal untouched when the filtered array is empty).
      command.m_Return.Value = tl_recorderReturnValue;
      return;
    }
    // The pending subset will become signalled; wait indefinitely on it then
    // replay the original wait, exactly matching legacy vkWaitForFences_WRAPRUN
    // (vulkanPlayerRunWrap.h:1426-1433).
    auto& dispatchTable = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
    dispatchTable.vkWaitForFences(command.m_device.Value, static_cast<uint32_t>(pending.size()),
                                  pending.data(), VK_TRUE, UINT64_MAX);
    command.m_Return.Value = dispatchTable.vkWaitForFences(
        command.m_device.Value, command.m_pFences.Size, command.m_pFences.Value,
        command.m_waitAll.Value, command.m_timeout.Value);
  }
}

// vkWaitSemaphores

void ReplayCustomizationLayer::Pre(vkWaitSemaphoresCommand& command) {
  tl_recorderReturnValue = command.m_Return.Value;
}

void ReplayCustomizationLayer::Post(vkWaitSemaphoresCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != tl_recorderReturnValue) {
    auto& dispatchTable = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
    dispatchTable.vkWaitSemaphores(command.m_device.Value, command.m_pWaitInfo.Value,
                                   0xFFFFFFFFFFFFFFFF);
    command.m_Return.Value = dispatchTable.vkWaitSemaphores(
        command.m_device.Value, command.m_pWaitInfo.Value, command.m_timeout.Value);
  }
}

// vkWaitSemaphoresKHR

void ReplayCustomizationLayer::Pre(vkWaitSemaphoresKHRCommand& command) {
  tl_recorderReturnValue = command.m_Return.Value;
}

void ReplayCustomizationLayer::Post(vkWaitSemaphoresKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != tl_recorderReturnValue) {
    auto& dispatchTable = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
    dispatchTable.vkWaitSemaphoresKHR(command.m_device.Value, command.m_pWaitInfo.Value,
                                      0xFFFFFFFFFFFFFFFF);
    command.m_Return.Value = dispatchTable.vkWaitSemaphoresKHR(
        command.m_device.Value, command.m_pWaitInfo.Value, command.m_timeout.Value);
  }
}

// vkCreateDebugUtilsMessengerEXT / vkCreateDebugReportCallbackEXT
//
// The captured pCreateInfo holds a pfnUserCallback/pfnCallback pointing into the
// recording process' address space (for some apps, JIT-compiled code). Replayed
// verbatim, the loader/driver would jump to a now-invalid address and crash.
// Replace it with a GITS-owned callback that forwards the message to the GITS
// log (mirroring gfxreconstruct): the messenger stays functional so driver/layer
// diagnostics are still surfaced during replay, but always points at valid,
// replay-process code. pUserData is intentionally left untouched and unused --
// the callback logs through the global GITS logger, so the captured (now
// meaningless) pUserData value is never dereferenced.

VKAPI_ATTR VkBool32 VKAPI_CALL
debugUtilsMessengerLogCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                               VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
                               const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                               void* /*pUserData*/) {
  const char* message = (pCallbackData && pCallbackData->pMessage) ? pCallbackData->pMessage : "";
  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    LOG_ERROR << "Vulkan debug messenger: " << message;
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    LOG_WARNING << "Vulkan debug messenger: " << message;
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    LOG_INFO << "Vulkan debug messenger: " << message;
  } else {
    LOG_TRACE << "Vulkan debug messenger: " << message;
  }
  return VK_FALSE;
}

void ReplayCustomizationLayer::Pre(vkCreateDebugUtilsMessengerEXTCommand& command) {
  if (command.m_pCreateInfo.Value) {
    command.m_pCreateInfo.Value->pfnUserCallback = debugUtilsMessengerLogCallback;
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugReportLogCallback(VkDebugReportFlagsEXT flags,
                                                      VkDebugReportObjectTypeEXT /*objectType*/,
                                                      uint64_t /*object*/,
                                                      size_t /*location*/,
                                                      int32_t /*messageCode*/,
                                                      const char* pLayerPrefix,
                                                      const char* pMessage,
                                                      void* /*pUserData*/) {
  const char* prefix = pLayerPrefix ? pLayerPrefix : "";
  const char* message = pMessage ? pMessage : "";
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    LOG_ERROR << "Vulkan debug report [" << prefix << "]: " << message;
  } else if (flags &
             (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)) {
    LOG_WARNING << "Vulkan debug report [" << prefix << "]: " << message;
  } else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
    LOG_INFO << "Vulkan debug report [" << prefix << "]: " << message;
  } else {
    LOG_TRACE << "Vulkan debug report [" << prefix << "]: " << message;
  }
  return VK_FALSE;
}

void ReplayCustomizationLayer::Pre(vkCreateDebugReportCallbackEXTCommand& command) {
  if (command.m_pCreateInfo.Value) {
    command.m_pCreateInfo.Value->pfnCallback = debugReportLogCallback;
  }
}

// An application may embed a VkDebugUtilsMessengerCreateInfoEXT (or the older
// VkDebugReportCallbackCreateInfoEXT) directly in the VkInstanceCreateInfo
// pNext chain so the driver emits diagnostics during vkCreateInstance /
// vkDestroyInstance themselves, before any standalone messenger exists. Its
// pfnUserCallback/pfnCallback is a raw pointer into the recording process'
// address space (for some apps, JIT-compiled code), captured verbatim. Replayed
// as-is, the loader walks the chain during vkCreateInstance and jumps to a now-
// invalid address, crashing with an execute access violation. The standalone
// vkCreateDebugUtilsMessengerEXT / vkCreateDebugReportCallbackEXT paths are
// already redirected above; do the same for every messenger / report struct in
// the pNext chain (an app may chain more than one) by pointing each callback at
// the GITS logging stub.  This only ever rewrites callbacks the app already set
// up -- nothing is installed when the app used no debug messenger, so it is a
// no-op for the common case and adds no per-call overhead (it runs once, at
// instance creation).
static void RedirectDebugCallbacksInPNext(const void* pNext) {
  auto* node = static_cast<VkBaseOutStructure*>(const_cast<void*>(pNext));
  while (node) {
    switch (node->sType) {
    case VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT:
      reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(node)->pfnUserCallback =
          debugUtilsMessengerLogCallback;
      break;
    case VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT:
      reinterpret_cast<VkDebugReportCallbackCreateInfoEXT*>(node)->pfnCallback =
          debugReportLogCallback;
      break;
    default:
      break;
    }
    node = node->pNext;
  }
}

void ReplayCustomizationLayer::Pre(vkCreateInstanceCommand& command) {
  if (command.m_pCreateInfo.Value) {
    RedirectDebugCallbacksInPNext(command.m_pCreateInfo.Value->pNext);
  }
}

// vkCreateDescriptorUpdateTemplate / KHR

void ReplayCustomizationLayer::Post(vkCreateDescriptorUpdateTemplateCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  m_Manager.GetDescriptorUpdateTemplateService().StoreTemplate(
      *command.m_pDescriptorUpdateTemplate.Value, command.m_pCreateInfo.Value);
}

void ReplayCustomizationLayer::Post(vkCreateDescriptorUpdateTemplateKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  m_Manager.GetDescriptorUpdateTemplateService().StoreTemplate(
      *command.m_pDescriptorUpdateTemplate.Value, command.m_pCreateInfo.Value);
}

void ReplayCustomizationLayer::Pre(vkDestroyDescriptorUpdateTemplateCommand& command) {
  m_Manager.GetDescriptorUpdateTemplateService().RemoveTemplate(
      command.m_descriptorUpdateTemplate.Value);
}

void ReplayCustomizationLayer::Pre(vkDestroyDescriptorUpdateTemplateKHRCommand& command) {
  m_Manager.GetDescriptorUpdateTemplateService().RemoveTemplate(
      command.m_descriptorUpdateTemplate.Value);
}

// vkUpdateDescriptorSetWithTemplate / KHR

void ReplayCustomizationLayer::Pre(vkUpdateDescriptorSetWithTemplateCommand& command) {
  m_Manager.GetDescriptorUpdateTemplateService().RemapHandles(
      command.m_descriptorUpdateTemplate.Value, command.m_pData);
}

void ReplayCustomizationLayer::Pre(vkUpdateDescriptorSetWithTemplateKHRCommand& command) {
  m_Manager.GetDescriptorUpdateTemplateService().RemapHandles(
      command.m_descriptorUpdateTemplate.Value, command.m_pData);
}

// vkCmdPushDescriptorSetWithTemplate / KHR

void ReplayCustomizationLayer::Pre(vkCmdPushDescriptorSetWithTemplateCommand& command) {
  m_Manager.GetDescriptorUpdateTemplateService().RemapHandles(
      command.m_descriptorUpdateTemplate.Value, command.m_pData);
}

void ReplayCustomizationLayer::Pre(vkCmdPushDescriptorSetWithTemplateKHRCommand& command) {
  m_Manager.GetDescriptorUpdateTemplateService().RemapHandles(
      command.m_descriptorUpdateTemplate.Value, command.m_pData);
}

void ReplayCustomizationLayer::Pre(vkCreateGraphicsPipelinesCommand& command) {
  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    command.m_pCreateInfos.Value[i].flags &=
        ~VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT;
  }
}

void ReplayCustomizationLayer::Pre(vkAcquireNextImageKHRCommand& command) {
  m_Manager.GetSwapchainImageSyncService().HandleAcquireNextImage(command);
}

void ReplayCustomizationLayer::Pre(vkAcquireNextImage2KHRCommand& command) {
  m_Manager.GetSwapchainImageSyncService().HandleAcquireNextImage2(command);
}

// Per-fence pending-signal tracking is owned by FencePendingSignalService; the
// handlers below simply feed it the signalling/reset events.

// A fence created with VK_FENCE_CREATE_SIGNALED_BIT starts signalled, which is a
// consumable pending signal exactly like a submit/acquire.  The legacy state
// dynamic initializes fenceUsed = (flags & SIGNALED_BIT) in the CFenceState ctor
// (vulkanStateDynamic.h:1271), and that runs during replay too, so mirror it
// here.  Note the player replays vkCreateFence with the original create flags,
// so the live fence really is created signalled and a status poll will already
// observe VK_SUCCESS; this marking keeps the pending set a faithful mirror of
// legacy fenceUsed and guards the (filtered) vkWaitForFences subset selection.
void ReplayCustomizationLayer::Post(vkCreateFenceCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pCreateInfo.Value != nullptr &&
      (command.m_pCreateInfo.Value->flags & VK_FENCE_CREATE_SIGNALED_BIT)) {
    m_Manager.GetFencePendingSignalService().MarkPending(*command.m_pFence.Value);
  }
}

// A queue submit / sparse bind / acquire that carries a fence signals it, so the
// fence gains a pending signal (legacy vkQueueSubmit_SD etc. set fenceUsed=true).
void ReplayCustomizationLayer::Post(vkQueueSubmitCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS) {
    m_Manager.GetFencePendingSignalService().MarkPending(command.m_fence.Value);
  }
}

void ReplayCustomizationLayer::Post(vkQueueSubmit2Command& command) {
  if (command.m_Return.Value == VK_SUCCESS) {
    m_Manager.GetFencePendingSignalService().MarkPending(command.m_fence.Value);
  }
}

void ReplayCustomizationLayer::Post(vkQueueSubmit2KHRCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS) {
    m_Manager.GetFencePendingSignalService().MarkPending(command.m_fence.Value);
  }
}

void ReplayCustomizationLayer::Post(vkQueueBindSparseCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS) {
    m_Manager.GetFencePendingSignalService().MarkPending(command.m_fence.Value);
  }
}

void ReplayCustomizationLayer::Post(vkAcquireNextImageKHRCommand& command) {
  // VK_SUBOPTIMAL_KHR still acquires (and signals); only a hard error does not.
  if (command.m_Return.Value == VK_SUCCESS || command.m_Return.Value == VK_SUBOPTIMAL_KHR) {
    m_Manager.GetFencePendingSignalService().MarkPending(command.m_fence.Value);
  }
}

void ReplayCustomizationLayer::Post(vkAcquireNextImage2KHRCommand& command) {
  if ((command.m_Return.Value == VK_SUCCESS || command.m_Return.Value == VK_SUBOPTIMAL_KHR) &&
      command.m_pAcquireInfo.Value != nullptr) {
    m_Manager.GetFencePendingSignalService().MarkPending(command.m_pAcquireInfo.Value->fence);
  }
}

// vkResetFences clears the pending signal for every fence in the array (legacy
// vkResetFences_SD sets fenceUsed=false for all fenceCount fences).
void ReplayCustomizationLayer::Post(vkResetFencesCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  for (uint32_t i = 0; i < command.m_pFences.Size; ++i) {
    m_Manager.GetFencePendingSignalService().ClearPending(command.m_pFences.Value[i]);
  }
}

// Drop tracking on destroy so a recycled VkFence handle does not inherit stale
// pending state.
void ReplayCustomizationLayer::Post(vkDestroyFenceCommand& command) {
  m_Manager.GetFencePendingSignalService().ClearPending(command.m_fence.Value);
}

} // namespace vulkan
} // namespace gits
