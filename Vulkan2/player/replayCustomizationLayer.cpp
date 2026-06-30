// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "replayCustomizationLayer.h"
#include "playerManager.h"

namespace gits {
namespace vulkan {

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

} // namespace vulkan
} // namespace gits
