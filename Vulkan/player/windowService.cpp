// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowService.h"
#include "message_pump.h"
#include "configurator.h"
#include "commandsCustom.h"
#include "windowManager.h"

namespace gits {
namespace vulkan {

uint64_t WindowService::SetWindow(uint32_t protocol,
                                  uint64_t handle,
                                  uint64_t instance,
                                  int32_t x,
                                  int32_t y,
                                  int32_t width,
                                  int32_t height,
                                  bool visible) {
  // Workaround that forces all windows visible regardless of the recorded flag.
  // forceInvisibleWindows still wins downstream in CreateWin/WinVisibility
  // (message_pump.cpp).
  if (Configurator::Get().common.player.showWindowsWA) {
    visible = true;
  }

  auto it = m_WindowMap.find(handle);
  if (it != m_WindowMap.end()) {
    auto& state = it->second;
    auto& cfg = Configurator::Get().common.player;
    uint32_t wndWidth = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.width : width;
    uint32_t wndHeight = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.height : height;
    if (state.width != wndWidth || state.height != wndHeight) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
      if (protocol == CreateWindowMetaCommand::DisplayProtocol::WIN) {
        ResizeWin(reinterpret_cast<HWND>(state.playbackHandle), wndWidth, wndHeight);
      }
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
      if (protocol == CreateWindowMetaCommand::DisplayProtocol::XLIB) {
        ResizeXlibWindow(state.playbackHandle, m_InstanceMap[instance], wndWidth, wndHeight);
      }
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
      if (protocol == CreateWindowMetaCommand::DisplayProtocol::XCB) {
        ResizeXcbWindow(state.playbackHandle, m_InstanceMap[instance], wndWidth, wndHeight);
      }
#endif
      state.width = wndWidth;
      state.height = wndHeight;
    }
    if (state.visible != visible) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
      WinVisibility(reinterpret_cast<HWND>(state.playbackHandle), visible);
      state.visible = visible;
#endif
    }
    return state.playbackHandle;
  }

  auto& cfg = Configurator::Get().common.player;
  uint32_t wndPosX = cfg.forceWindowPos.enabled ? cfg.forceWindowPos.x : x;
  uint32_t wndPosY = cfg.forceWindowPos.enabled ? cfg.forceWindowPos.y : y;
  uint32_t wndWidth = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.width : width;
  uint32_t wndHeight = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.height : height;

  uint64_t currentHandle{};
  uint64_t currentInstance{};

  switch (protocol) {
  case CreateWindowMetaCommand::DisplayProtocol::WIN: {
#ifdef VK_USE_PLATFORM_WIN32_KHR
    HWND currentHWND = CreateWin(wndWidth, wndHeight, wndPosX, wndPosY, visible);
    WinTitle(currentHWND, "Vulkan-GITS");
    HINSTANCE hInstance =
        reinterpret_cast<HINSTANCE>(GetWindowLongPtr(currentHWND, GWLP_HINSTANCE));
    currentHandle = reinterpret_cast<uint64_t>(currentHWND);
    currentInstance = reinterpret_cast<uint64_t>(hInstance);
#endif
    break;
  }
  case CreateWindowMetaCommand::DisplayProtocol::XLIB: {
#ifdef VK_USE_PLATFORM_XLIB_KHR
    auto winParams = CreateXlibWindow(wndPosX, wndPosY, wndWidth, wndHeight, visible);
    currentHandle = winParams.first;
    currentInstance = winParams.second;
#endif
    break;
  }
  case CreateWindowMetaCommand::DisplayProtocol::XCB: {
#ifdef VK_USE_PLATFORM_XCB_KHR
    auto winParams = CreateXcbWindow(wndPosX, wndPosY, wndWidth, wndHeight, visible);
    currentHandle = winParams.first;
    currentInstance = winParams.second;
#endif
    break;
  }
  case CreateWindowMetaCommand::DisplayProtocol::WAYLAND: {
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    auto winParams = CreateWaylandWindow(wndPosX, wndPosY, wndWidth, wndHeight, visible);
    currentHandle = winParams.first;
    currentInstance = winParams.second;
#endif
    break;
  }
  }

  m_WindowMap[handle] = {currentHandle,
                         static_cast<int32_t>(wndPosX),
                         static_cast<int32_t>(wndPosY),
                         wndWidth,
                         wndHeight,
                         visible};
  m_InstanceMap[instance] = currentInstance;

  return currentHandle;
}

void WindowService::UpdateWindow(
    uint64_t handle, int32_t x, int32_t y, int32_t width, int32_t height, bool visible) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
  if (Configurator::Get().common.player.showWindowsWA) {
    visible = true;
  }
  auto it = m_WindowMap.find(handle);
  if (it != m_WindowMap.end()) {
    auto& cfg = Configurator::Get().common.player;
    int32_t wndPosX = cfg.forceWindowPos.enabled ? cfg.forceWindowPos.x : x;
    int32_t wndPosY = cfg.forceWindowPos.enabled ? cfg.forceWindowPos.y : y;
    uint32_t wndWidth = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.width : width;
    uint32_t wndHeight = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.height : height;

    auto& state = it->second;
    if (state.width != wndWidth || state.height != wndHeight) {
      ResizeWin(reinterpret_cast<HWND>(state.playbackHandle), wndWidth, wndHeight);
      state.width = wndWidth;
      state.height = wndHeight;
    }
    if (state.x != wndPosX || state.y != wndPosY) {
      MoveWin(reinterpret_cast<HWND>(state.playbackHandle), wndPosX, wndPosY);
      state.x = wndPosX;
      state.y = wndPosY;
    }
    if (state.visible != visible) {
      WinVisibility(reinterpret_cast<HWND>(state.playbackHandle), visible);
      state.visible = visible;
    }
  }
#endif
}

uint64_t WindowService::GetCurrentWindowHandle(uint64_t captureWindow) {
  auto it = m_WindowMap.find(captureWindow);
  if (it != m_WindowMap.end()) {
    return it->second.playbackHandle;
  }
  return 0;
}

uint64_t WindowService::GetCurrentInstance(uint64_t captureInstance) {
  auto it = m_InstanceMap.find(captureInstance);
  if (it != m_InstanceMap.end()) {
    return it->second;
  }
  return 0;
}

#ifdef GITS_PLATFORM_LINUX
std::pair<uint64_t, uint64_t> WindowService::CreateXlibWindow(
    int32_t x, int32_t y, int32_t width, int32_t height, bool visible) {
  return windowing::WindowManager::Get().CreatePlayerWindow(windowing::WindowProtocol::Xlib, x, y,
                                                            width, height, visible);
}

void WindowService::ResizeXlibWindow(uint64_t display,
                                     uint64_t window,
                                     uint32_t width,
                                     uint32_t height) {
  windowing::WindowManager::Get().ResizeWindow(windowing::WindowProtocol::Xlib, display, window,
                                               width, height);
}

std::pair<uint64_t, uint64_t> WindowService::CreateXcbWindow(
    int32_t x, int32_t y, int32_t width, int32_t height, bool visible) {
  return windowing::WindowManager::Get().CreatePlayerWindow(windowing::WindowProtocol::Xcb, x, y,
                                                            width, height, visible);
}

void WindowService::ResizeXcbWindow(uint64_t connection,
                                    uint64_t window,
                                    uint32_t width,
                                    uint32_t height) {
  windowing::WindowManager::Get().ResizeWindow(windowing::WindowProtocol::Xcb, connection, window,
                                               width, height);
}

std::pair<uint64_t, uint64_t> WindowService::CreateWaylandWindow(
    int32_t x, int32_t y, int32_t width, int32_t height, bool visible) {
  return windowing::WindowManager::Get().CreatePlayerWindow(windowing::WindowProtocol::Wayland, x,
                                                            y, width, height, visible);
}
#endif

} // namespace vulkan
} // namespace gits
