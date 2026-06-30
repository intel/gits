// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureCustomizationLayer.h"
#include "captureManager.h"
#include "commandWritersCustom.h"

namespace gits {
namespace vulkan {

thread_local CaptureCustomizationLayer::AllocateInfo CaptureCustomizationLayer::s_AllocateInfo;

#ifdef VK_USE_PLATFORM_WIN32_KHR
void CaptureCustomizationLayer::Pre(vkCreateWin32SurfaceKHRCommand& command) {
  RECT clientRect;
  BOOL ret = GetClientRect(command.m_pCreateInfo.Value->hwnd, &clientRect);
  GITS_ASSERT(ret);
  int32_t width = clientRect.right - clientRect.left;
  int32_t height = clientRect.bottom - clientRect.top;
  RECT windowRect;
  ret = GetWindowRect(command.m_pCreateInfo.Value->hwnd, &windowRect);
  GITS_ASSERT(ret);
  int32_t x = windowRect.left + clientRect.left;
  int32_t y = windowRect.top + clientRect.top;
  bool isVisible = IsWindowVisible(command.m_pCreateInfo.Value->hwnd) == TRUE;

  CreateWindowMetaCommand createWindowMetaCommand(command.m_ThreadId);
  createWindowMetaCommand.m_Key = m_Manager.CreateCommandKey();
  createWindowMetaCommand.m_X.Value = x;
  createWindowMetaCommand.m_Y.Value = y;
  createWindowMetaCommand.m_Width.Value = width;
  createWindowMetaCommand.m_Height.Value = height;
  createWindowMetaCommand.m_Visible.Value = isVisible;
  createWindowMetaCommand.m_Hwnd.Value =
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->hwnd);
  createWindowMetaCommand.m_Hinstance.Value =
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->hinstance);

  m_Recorder.record(createWindowMetaCommand.m_Key,
                    new CreateWindowMetaWriter(createWindowMetaCommand));
}
#endif

void CaptureCustomizationLayer::Post(vkCreateDeviceCommand& command) {
  m_Manager.GetMapTrackingService().StorePhysicalDevice(command.m_pDevice.Key,
                                                        command.m_physicalDevice.Key);
}

void CaptureCustomizationLayer::Post(vkGetPhysicalDeviceMemoryPropertiesCommand& command) {
  m_Manager.GetMapTrackingService().StorePhysicalDeviceMemoryProperties(
      command.m_physicalDevice.Key, *command.m_pMemoryProperties.Value);
}

void CaptureCustomizationLayer::Post(vkGetPhysicalDeviceMemoryProperties2Command& command) {
  m_Manager.GetMapTrackingService().StorePhysicalDeviceMemoryProperties(
      command.m_physicalDevice.Key, command.m_pMemoryProperties.Value->memoryProperties);
}

void CaptureCustomizationLayer::Post(vkGetPhysicalDeviceMemoryProperties2KHRCommand& command) {
  m_Manager.GetMapTrackingService().StorePhysicalDeviceMemoryProperties(
      command.m_physicalDevice.Key, command.m_pMemoryProperties.Value->memoryProperties);
}

void CaptureCustomizationLayer::Pre(vkAllocateMemoryCommand& command) {
  auto& mapTrackingService = m_Manager.GetMapTrackingService();
  if (!mapTrackingService.IsMemoryMappable(command.m_device.Key,
                                           command.m_pAllocateInfo.Value->memoryTypeIndex)) {
    return;
  }
  s_AllocateInfo = AllocateInfo(command.m_pAllocateInfo.Value);
  command.m_pAllocateInfo.Value = &s_AllocateInfo.AllocateInfoModified;
  s_AllocateInfo.ExternalMemory =
      mapTrackingService.EnableExternalMemory(command.m_device.Key, command.m_pAllocateInfo.Value);
}

void CaptureCustomizationLayer::Post(vkAllocateMemoryCommand& command) {
  auto& mapTrackingService = m_Manager.GetMapTrackingService();
  if (!mapTrackingService.IsMemoryMappable(command.m_device.Key,
                                           command.m_pAllocateInfo.Value->memoryTypeIndex)) {
    return;
  }

  VkMemoryAllocateInfo modifiedAllocateInfo = *command.m_pAllocateInfo.Value;
  m_Manager.GetMapTrackingService().StoreAllocationInfo(
      command.m_device.Key, command.m_pMemory.Key, *command.m_pMemory.Value, modifiedAllocateInfo,
      s_AllocateInfo.ExternalMemory);
  command.m_pAllocateInfo.Value = s_AllocateInfo.AllocateInfoPtr;
}

void CaptureCustomizationLayer::Post(vkFreeMemoryCommand& command) {
  m_Manager.GetMapTrackingService().FreeExternalMemory(command.m_memory.Key);
}

void CaptureCustomizationLayer::Post(vkMapMemoryCommand& command) {
  m_Manager.GetMapTrackingService().StoreData(command.m_memory.Key, command.m_offset.Value,
                                              command.m_size.Value, *command.m_ppData.Value);
}

void CaptureCustomizationLayer::Pre(vkUnmapMemoryCommand& command) {
  m_Manager.GetMapTrackingService().RemoveData(command.m_memory.Key);
}

void CaptureCustomizationLayer::Pre(vkQueueSubmitCommand& command) {
  m_Manager.GetMapTrackingService().SnapshotAllMapped();
}

void CaptureCustomizationLayer::Post(vkCreateDescriptorUpdateTemplateCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pDescriptorUpdateTemplate.Value) {
    m_Manager.GetDescriptorUpdateTemplateService().StoreTemplate(
        *command.m_pDescriptorUpdateTemplate.Value, command.m_pCreateInfo.Value);
  }
}

void CaptureCustomizationLayer::Post(vkCreateDescriptorUpdateTemplateKHRCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pDescriptorUpdateTemplate.Value) {
    m_Manager.GetDescriptorUpdateTemplateService().StoreTemplate(
        *command.m_pDescriptorUpdateTemplate.Value, command.m_pCreateInfo.Value);
  }
}

void CaptureCustomizationLayer::Pre(vkDestroyDescriptorUpdateTemplateCommand& command) {
  m_Manager.GetDescriptorUpdateTemplateService().RemoveTemplate(
      command.m_descriptorUpdateTemplate.Value);
}

void CaptureCustomizationLayer::Pre(vkDestroyDescriptorUpdateTemplateKHRCommand& command) {
  m_Manager.GetDescriptorUpdateTemplateService().RemoveTemplate(
      command.m_descriptorUpdateTemplate.Value);
}

void CaptureCustomizationLayer::Pre(vkUpdateDescriptorSetWithTemplateCommand& command) {
  m_Manager.GetDescriptorUpdateTemplateService().SerializeData(
      command.m_descriptorUpdateTemplate.Value, command.m_pData.Value, command.m_pData);
}

void CaptureCustomizationLayer::Pre(vkUpdateDescriptorSetWithTemplateKHRCommand& command) {
  m_Manager.GetDescriptorUpdateTemplateService().SerializeData(
      command.m_descriptorUpdateTemplate.Value, command.m_pData.Value, command.m_pData);
}

void CaptureCustomizationLayer::Pre(vkCmdPushDescriptorSetWithTemplateCommand& command) {
  m_Manager.GetDescriptorUpdateTemplateService().SerializeData(
      command.m_descriptorUpdateTemplate.Value, command.m_pData.Value, command.m_pData);
}

void CaptureCustomizationLayer::Pre(vkCmdPushDescriptorSetWithTemplateKHRCommand& command) {
  m_Manager.GetDescriptorUpdateTemplateService().SerializeData(
      command.m_descriptorUpdateTemplate.Value, command.m_pData.Value, command.m_pData);
}

} // namespace vulkan
} // namespace gits
