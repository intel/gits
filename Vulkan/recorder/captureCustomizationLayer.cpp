// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureCustomizationLayer.h"
#include "captureManager.h"
#include "commandSerializersCustom.h"

namespace gits {
namespace vulkan {

thread_local CaptureCustomizationLayer::AllocateInfo CaptureCustomizationLayer::s_AllocateInfo;
thread_local VkBufferCreateInfo CaptureCustomizationLayer::s_BufferCreateInfo;
thread_local VkImageCreateInfo CaptureCustomizationLayer::s_ImageCreateInfo;

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
  createWindowMetaCommand.m_DisplayProtocol.Value = CreateWindowMetaCommand::DisplayProtocol::WIN;
  createWindowMetaCommand.m_X.Value = x;
  createWindowMetaCommand.m_Y.Value = y;
  createWindowMetaCommand.m_Width.Value = width;
  createWindowMetaCommand.m_Height.Value = height;
  createWindowMetaCommand.m_Visible.Value = isVisible;
  createWindowMetaCommand.m_Hwnd.Value =
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->hwnd);
  createWindowMetaCommand.m_Hinstance.Value =
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->hinstance);

  m_Recorder.Record(createWindowMetaCommand.m_Key,
                    new CreateWindowMetaSerializer(createWindowMetaCommand));
}

void CaptureCustomizationLayer::Post(vkCreateWin32SurfaceKHRCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pSurface.Value) {
    HWND hwnd = command.m_pCreateInfo.Value->hwnd;
    RECT clientRect;
    BOOL ret = GetClientRect(hwnd, &clientRect);
    GITS_ASSERT(ret);
    int32_t width = clientRect.right - clientRect.left;
    int32_t height = clientRect.bottom - clientRect.top;
    RECT windowRect;
    ret = GetWindowRect(hwnd, &windowRect);
    GITS_ASSERT(ret);
    int32_t x = windowRect.left;
    int32_t y = windowRect.top;
    bool visible = IsWindowVisible(hwnd) == TRUE;
    m_Manager.GetWindowTrackingService().StoreSurface(
        *command.m_pSurface.Value, reinterpret_cast<uint64_t>(hwnd),
        reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->hinstance), x, y, width, height,
        visible);
  }
}
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
void CaptureCustomizationLayer::Pre(vkCreateXlibSurfaceKHRCommand& command) {
  Display* display = command.m_pCreateInfo.Value->dpy;
  Window window = command.m_pCreateInfo.Value->window;

  int32_t x{};
  int32_t y{};
  int32_t width{};
  int32_t height{};
  bool visible{true};

  XWindowAttributes attrs;
  if (XGetWindowAttributes(display, window, &attrs)) {
    x = attrs.x;
    y = attrs.y;
    width = attrs.width;
    height = attrs.height;
    visible = (attrs.map_state == IsViewable);

    Window child{};
    int rootX{};
    int rootY{};
    if (XTranslateCoordinates(display, window, attrs.root, 0, 0, &rootX, &rootY, &child)) {
      x = rootX;
      y = rootY;
    }
  }

  CreateWindowMetaCommand createWindowMetaCommand(command.m_ThreadId);
  createWindowMetaCommand.m_Key = m_Manager.CreateCommandKey();
  createWindowMetaCommand.m_DisplayProtocol.Value = CreateWindowMetaCommand::DisplayProtocol::XLIB;
  createWindowMetaCommand.m_X.Value = x;
  createWindowMetaCommand.m_Y.Value = y;
  createWindowMetaCommand.m_Width.Value = width;
  createWindowMetaCommand.m_Height.Value = height;
  createWindowMetaCommand.m_Visible.Value = visible;
  createWindowMetaCommand.m_Hwnd.Value = reinterpret_cast<uint64_t>(display);
  createWindowMetaCommand.m_Hinstance.Value = reinterpret_cast<uint64_t>(window);

  m_Recorder.Record(createWindowMetaCommand.m_Key,
                    new CreateWindowMetaSerializer(createWindowMetaCommand));
}
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
void CaptureCustomizationLayer::Pre(vkCreateXcbSurfaceKHRCommand& command) {
  xcb_connection_t* connection = command.m_pCreateInfo.Value->connection;
  xcb_window_t window = command.m_pCreateInfo.Value->window;

  int32_t x{};
  int32_t y{};
  int32_t width{};
  int32_t height{};
  bool visible{true};
  xcb_window_t rootWindow{};

  xcb_get_geometry_cookie_t geom_cookie = xcb_get_geometry(connection, window);
  xcb_get_geometry_reply_t* geom = xcb_get_geometry_reply(connection, geom_cookie, nullptr);

  if (geom) {
    x = geom->x;
    y = geom->y;
    width = geom->width;
    height = geom->height;
    rootWindow = geom->root;
    free(geom);
  }

  if (rootWindow != 0) {
    xcb_translate_coordinates_cookie_t translateCookie =
        xcb_translate_coordinates(connection, window, rootWindow, 0, 0);
    xcb_translate_coordinates_reply_t* translated =
        xcb_translate_coordinates_reply(connection, translateCookie, nullptr);
    if (translated) {
      x = translated->dst_x;
      y = translated->dst_y;
      free(translated);
    }
  }

  xcb_get_window_attributes_cookie_t attr_cookie = xcb_get_window_attributes(connection, window);
  xcb_get_window_attributes_reply_t* attr =
      xcb_get_window_attributes_reply(connection, attr_cookie, nullptr);

  if (attr) {
    visible = attr->map_state == XCB_MAP_STATE_VIEWABLE;
    free(attr);
  }

  CreateWindowMetaCommand createWindowMetaCommand(command.m_ThreadId);
  createWindowMetaCommand.m_Key = m_Manager.CreateCommandKey();
  createWindowMetaCommand.m_DisplayProtocol.Value = CreateWindowMetaCommand::DisplayProtocol::XCB;
  createWindowMetaCommand.m_X.Value = x;
  createWindowMetaCommand.m_Y.Value = y;
  createWindowMetaCommand.m_Width.Value = width;
  createWindowMetaCommand.m_Height.Value = height;
  createWindowMetaCommand.m_Visible.Value = visible;
  createWindowMetaCommand.m_Hwnd.Value = reinterpret_cast<uint64_t>(connection);
  createWindowMetaCommand.m_Hinstance.Value = static_cast<uint64_t>(window);

  m_Recorder.Record(createWindowMetaCommand.m_Key,
                    new CreateWindowMetaSerializer(createWindowMetaCommand));
}
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
void CaptureCustomizationLayer::Pre(vkCreateWaylandSurfaceKHRCommand& command) {
  CreateWindowMetaCommand createWindowMetaCommand(command.m_ThreadId);
  createWindowMetaCommand.m_Key = m_Manager.CreateCommandKey();
  createWindowMetaCommand.m_DisplayProtocol.Value =
      CreateWindowMetaCommand::DisplayProtocol::WAYLAND;
  createWindowMetaCommand.m_X.Value = 0;
  createWindowMetaCommand.m_Y.Value = 0;
  createWindowMetaCommand.m_Width.Value = 0;
  createWindowMetaCommand.m_Height.Value = 0;
  createWindowMetaCommand.m_Visible.Value = true;
  createWindowMetaCommand.m_Hwnd.Value =
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->display);
  createWindowMetaCommand.m_Hinstance.Value =
      reinterpret_cast<uint64_t>(command.m_pCreateInfo.Value->surface);

  m_Recorder.Record(createWindowMetaCommand.m_Key,
                    new CreateWindowMetaSerializer(createWindowMetaCommand));
}
#endif

void CaptureCustomizationLayer::Pre(vkCreateBufferCommand& command) {
  if (!command.m_pCreateInfo.Value) {
    return;
  }
  s_BufferCreateInfo = *command.m_pCreateInfo.Value;
  s_BufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  command.m_pCreateInfo.Value = &s_BufferCreateInfo;
}

void CaptureCustomizationLayer::Pre(vkCreateImageCommand& command) {
  if (!command.m_pCreateInfo.Value) {
    return;
  }
  s_ImageCreateInfo = *command.m_pCreateInfo.Value;
  s_ImageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  command.m_pCreateInfo.Value = &s_ImageCreateInfo;
}

void CaptureCustomizationLayer::Post(vkCreateSwapchainKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS || !command.m_pCreateInfo.Value ||
      !command.m_pSwapchain.Value) {
    return;
  }
#ifdef VK_USE_PLATFORM_WIN32_KHR
  m_Manager.GetWindowTrackingService().StoreSwapchain(
      *command.m_pSwapchain.Value, command.m_pCreateInfo.Value->surface, command.m_ThreadId,
      static_cast<int32_t>(command.m_pCreateInfo.Value->imageExtent.width),
      static_cast<int32_t>(command.m_pCreateInfo.Value->imageExtent.height));
#endif
}

void CaptureCustomizationLayer::Post(vkDestroySwapchainKHRCommand& command) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
  m_Manager.GetWindowTrackingService().RemoveSwapchain(command.m_swapchain.Value);
#endif
}

void CaptureCustomizationLayer::Post(vkDestroySurfaceKHRCommand& command) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
  m_Manager.GetWindowTrackingService().RemoveSurface(command.m_surface.Value);
#endif
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
void CaptureCustomizationLayer::Pre(vkQueuePresentKHRCommand& command) {
  const VkPresentInfoKHR* presentInfo = command.m_pPresentInfo.Value;
  if (!presentInfo) {
    return;
  }
  m_Manager.GetWindowTrackingService().UpdateWindowsForPresent(command.m_ThreadId, *presentInfo);
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
  s_AllocateInfo.ExternalMemory = mapTrackingService.EnableExternalMemory(
      command.m_device.Key, command.m_pAllocateInfo.Value, s_AllocateInfo.HostPointerInfo);
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
                                              command.m_size.Value, command.m_ppData.Value);
}

void CaptureCustomizationLayer::Post(vkMapMemory2Command& command) {
  m_Manager.GetMapTrackingService().StoreData(
      command.m_pMemoryMapInfo.HandleKeys[0], command.m_pMemoryMapInfo.Value->offset,
      command.m_pMemoryMapInfo.Value->size, command.m_ppData.Value);
}

void CaptureCustomizationLayer::Post(vkMapMemory2KHRCommand& command) {
  m_Manager.GetMapTrackingService().StoreData(
      command.m_pMemoryMapInfo.HandleKeys[0], command.m_pMemoryMapInfo.Value->offset,
      command.m_pMemoryMapInfo.Value->size, command.m_ppData.Value);
}

void CaptureCustomizationLayer::Pre(vkUnmapMemoryCommand& command) {
  m_Manager.GetMapTrackingService().RemoveData(command.m_memory.Key);
}

void CaptureCustomizationLayer::Pre(vkUnmapMemory2Command& command) {
  m_Manager.GetMapTrackingService().RemoveData(command.m_pMemoryUnmapInfo.HandleKeys[0]);
}

void CaptureCustomizationLayer::Pre(vkUnmapMemory2KHRCommand& command) {
  m_Manager.GetMapTrackingService().RemoveData(command.m_pMemoryUnmapInfo.HandleKeys[0]);
}

void CaptureCustomizationLayer::Pre(vkQueueSubmitCommand& command) {
  m_Manager.GetMapTrackingService().SnapshotAllMapped();
}

void CaptureCustomizationLayer::Pre(vkQueueSubmit2Command& command) {
  m_Manager.GetMapTrackingService().SnapshotAllMapped();
}

void CaptureCustomizationLayer::Pre(vkQueueSubmit2KHRCommand& command) {
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
