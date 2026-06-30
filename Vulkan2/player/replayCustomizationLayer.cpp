// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "replayCustomizationLayer.h"
#include "playerManager.h"

#include <thread>
#include <chrono>

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
  if (command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != tl_recorderReturnValue) {
    auto& dispatchTable = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
    VkFence fence = command.m_fence.Value;
    dispatchTable.vkWaitForFences(command.m_device.Value, 1, &fence, VK_TRUE, 0xFFFFFFFFFFFFFFFF);
    command.m_Return.Value =
        dispatchTable.vkGetFenceStatus(command.m_device.Value, command.m_fence.Value);
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
    auto& dispatchTable = m_Manager.GetDeviceDispatchTable(command.m_device.Value);
    dispatchTable.vkWaitForFences(command.m_device.Value, command.m_pFences.Size,
                                  command.m_pFences.Value, VK_TRUE, 0xFFFFFFFFFFFFFFFF);
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

} // namespace vulkan
} // namespace gits
