// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowTrackingService.h"
#include "captureManager.h"
#include "commandSerializersCustom.h"

#ifdef VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
#endif

namespace gits {
namespace vulkan {

WindowTrackingService::WindowTrackingService(stream::OrderingRecorder& recorder)
    : m_Recorder(recorder) {}

void WindowTrackingService::StoreSurface(VkSurfaceKHR surface,
                                         uint32_t displayProtocol,
                                         uint64_t hwnd,
                                         uint64_t hinstance,
                                         int32_t x,
                                         int32_t y,
                                         int32_t width,
                                         int32_t height,
                                         bool visible) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_SurfaceWindow[surface] = {displayProtocol, hwnd, hinstance};
  m_WindowState[hwnd] = {x, y, width, height, visible};
}

void WindowTrackingService::StoreSwapchain(VkSwapchainKHR swapchain,
                                           VkSurfaceKHR surface,
                                           uint32_t threadId,
                                           int32_t imageWidth,
                                           int32_t imageHeight) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_SurfaceWindow.find(surface);
  if (it == m_SurfaceWindow.end()) {
    return;
  }
  m_SwapchainSurface[swapchain] = surface;

  int32_t x{};
  int32_t y{};
  int32_t width{};
  int32_t height{};
  bool visible{};
  if (!QueryLiveWindowState(it->second, x, y, width, height, visible)) {
    auto stateIt = m_WindowState.find(it->second.Hwnd);
    if (stateIt == m_WindowState.end()) {
      return;
    }
    x = stateIt->second.X;
    y = stateIt->second.Y;
    visible = stateIt->second.Visible;
  }

  width = imageWidth;
  height = imageHeight;

  UpdateWindowMetaCommand updateWindowMetaCommand(threadId);
  updateWindowMetaCommand.m_Key = CaptureManager::Get().CreateCommandKey();
  updateWindowMetaCommand.m_Hwnd.Value = it->second.Hwnd;
  updateWindowMetaCommand.m_Hinstance.Value = it->second.Hinstance;
  updateWindowMetaCommand.m_X.Value = x;
  updateWindowMetaCommand.m_Y.Value = y;
  updateWindowMetaCommand.m_Width.Value = width;
  updateWindowMetaCommand.m_Height.Value = height;
  updateWindowMetaCommand.m_Visible.Value = visible;
  m_Recorder.Record(updateWindowMetaCommand.m_Key,
                    new UpdateWindowMetaSerializer(updateWindowMetaCommand));

  m_WindowState[it->second.Hwnd] = {x, y, width, height, visible};
}

void WindowTrackingService::RemoveSurface(VkSurfaceKHR surface) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_SurfaceWindow.find(surface);
  if (it == m_SurfaceWindow.end()) {
    return;
  }
  const uint64_t hwnd = it->second.Hwnd;
  m_SurfaceWindow.erase(it);
  for (const auto& entry : m_SurfaceWindow) {
    if (entry.second.Hwnd == hwnd) {
      return;
    }
  }
  m_WindowState.erase(hwnd);
}

void WindowTrackingService::RemoveSwapchain(VkSwapchainKHR swapchain) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_SwapchainSurface.erase(swapchain);
}

bool WindowTrackingService::QueryLiveWindowState(const SurfaceWindow& window,
                                                 int32_t& x,
                                                 int32_t& y,
                                                 int32_t& width,
                                                 int32_t& height,
                                                 bool& visible) const {
  switch (window.Protocol) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
  case CreateWindowMetaCommand::DisplayProtocol::WIN: {
    HWND hwnd = reinterpret_cast<HWND>(window.Hwnd);
    RECT clientRect{};
    if (GetClientRect(hwnd, &clientRect) == FALSE) {
      return false;
    }
    width = clientRect.right - clientRect.left;
    height = clientRect.bottom - clientRect.top;
    visible = IsWindowVisible(hwnd) == TRUE;
    RECT windowRect{};
    if (GetWindowRect(hwnd, &windowRect) == FALSE) {
      return false;
    }
    x = windowRect.left;
    y = windowRect.top;
    return true;
  }
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
  case CreateWindowMetaCommand::DisplayProtocol::XLIB: {
    auto* display = reinterpret_cast<Display*>(window.Hwnd);
    auto nativeWindow = reinterpret_cast<Window>(window.Hinstance);
    XWindowAttributes attrs{};
    if (!XGetWindowAttributes(display, nativeWindow, &attrs)) {
      return false;
    }
    x = attrs.x;
    y = attrs.y;
    width = attrs.width;
    height = attrs.height;
    visible = attrs.map_state == IsViewable;
    Window child{};
    int rootX{};
    int rootY{};
    if (XTranslateCoordinates(display, nativeWindow, attrs.root, 0, 0, &rootX, &rootY, &child)) {
      x = rootX;
      y = rootY;
    }
    return true;
  }
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
  case CreateWindowMetaCommand::DisplayProtocol::XCB: {
    auto* connection = reinterpret_cast<xcb_connection_t*>(window.Hwnd);
    auto nativeWindow = static_cast<xcb_window_t>(window.Hinstance);
    xcb_get_geometry_cookie_t geomCookie = xcb_get_geometry(connection, nativeWindow);
    xcb_get_geometry_reply_t* geom = xcb_get_geometry_reply(connection, geomCookie, nullptr);
    if (!geom) {
      return false;
    }
    x = geom->x;
    y = geom->y;
    width = geom->width;
    height = geom->height;
    const xcb_window_t rootWindow = geom->root;
    free(geom);

    if (rootWindow != 0) {
      xcb_translate_coordinates_cookie_t translateCookie =
          xcb_translate_coordinates(connection, nativeWindow, rootWindow, 0, 0);
      xcb_translate_coordinates_reply_t* translated =
          xcb_translate_coordinates_reply(connection, translateCookie, nullptr);
      if (translated) {
        x = translated->dst_x;
        y = translated->dst_y;
        free(translated);
      }
    }

    xcb_get_window_attributes_cookie_t attrCookie =
        xcb_get_window_attributes(connection, nativeWindow);
    xcb_get_window_attributes_reply_t* attr =
        xcb_get_window_attributes_reply(connection, attrCookie, nullptr);
    if (!attr) {
      return false;
    }
    visible = attr->map_state == XCB_MAP_STATE_VIEWABLE;
    free(attr);
    return true;
  }
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  case CreateWindowMetaCommand::DisplayProtocol::WAYLAND:
    return false;
#endif
  default:
    return false;
  }
}

void WindowTrackingService::RecordUpdateIfChanged(uint32_t threadId,
                                                  const SurfaceWindow& window,
                                                  int32_t x,
                                                  int32_t y,
                                                  int32_t width,
                                                  int32_t height,
                                                  bool visible) {
  auto stateIt = m_WindowState.find(window.Hwnd);
  if (stateIt != m_WindowState.end() && stateIt->second.X == x && stateIt->second.Y == y &&
      stateIt->second.Width == width && stateIt->second.Height == height &&
      stateIt->second.Visible == visible) {
    return;
  }

  UpdateWindowMetaCommand updateWindowMetaCommand(threadId);
  updateWindowMetaCommand.m_Key = CaptureManager::Get().CreateCommandKey();
  updateWindowMetaCommand.m_Hwnd.Value = window.Hwnd;
  updateWindowMetaCommand.m_Hinstance.Value = window.Hinstance;
  updateWindowMetaCommand.m_X.Value = x;
  updateWindowMetaCommand.m_Y.Value = y;
  updateWindowMetaCommand.m_Width.Value = width;
  updateWindowMetaCommand.m_Height.Value = height;
  updateWindowMetaCommand.m_Visible.Value = visible;
  m_Recorder.Record(updateWindowMetaCommand.m_Key,
                    new UpdateWindowMetaSerializer(updateWindowMetaCommand));

  m_WindowState[window.Hwnd] = {x, y, width, height, visible};
}

void WindowTrackingService::UpdateWindowsForPresent(uint32_t threadId,
                                                    const VkPresentInfoKHR& presentInfo) {
  // Poll the live window state on every present and record an UpdateWindowMetaCommand
  // whenever it changed. Called from CaptureCustomizationLayer::Pre(vkQueuePresentKHRCommand&)
  // so the meta command is ordered before the present in the stream.
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (uint32_t i = 0; i < presentInfo.swapchainCount; ++i) {
    auto swapchainIt = m_SwapchainSurface.find(presentInfo.pSwapchains[i]);
    if (swapchainIt == m_SwapchainSurface.end()) {
      continue;
    }
    auto windowIt = m_SurfaceWindow.find(swapchainIt->second);
    if (windowIt == m_SurfaceWindow.end()) {
      continue;
    }

    int32_t x{};
    int32_t y{};
    int32_t width{};
    int32_t height{};
    bool visible{};
    if (!QueryLiveWindowState(windowIt->second, x, y, width, height, visible)) {
      continue;
    }

    RecordUpdateIfChanged(threadId, windowIt->second, x, y, width, height, visible);
  }
}

} // namespace vulkan
} // namespace gits
