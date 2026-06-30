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

void CaptureCustomizationLayer::Post(vkAllocateMemoryCommand& command) {
  m_Manager.GetMapTrackingService().StoreAllocationInfo(
      command.m_device.Key, command.m_pMemory.Key, command.m_pAllocateInfo.Value->allocationSize,
      command.m_pAllocateInfo.Value->memoryTypeIndex);
}

void CaptureCustomizationLayer::Post(vkMapMemoryCommand& command) {
  m_Manager.GetMapTrackingService().StoreData(command.m_device.Key, command.m_memory.Key,
                                              *command.m_ppData.Value, command.m_size.Value);
}

void CaptureCustomizationLayer::Pre(vkUnmapMemoryCommand& command) {
  m_Manager.GetMapTrackingService().RemoveData(command.m_device.Key, command.m_memory.Key);
}

void CaptureCustomizationLayer::Pre(vkQueueSubmitCommand& command) {
  m_Manager.GetMapTrackingService().SnapshotAllMapped();
}

} // namespace vulkan
} // namespace gits
