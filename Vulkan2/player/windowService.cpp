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

namespace gits {
namespace vulkan {

uint64_t WindowService::SetWindow(uint64_t handle,
                                  uint64_t instance,
                                  int32_t x,
                                  int32_t y,
                                  int32_t width,
                                  int32_t height,
                                  bool visible) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
  uint64_t currentHandle{};
  uint64_t currentInstance{};
  auto it = m_WindowMap.find(handle);
  if (it != m_WindowMap.end()) {
    auto& state = it->second;
    auto& cfg = Configurator::Get().common.player;
    uint32_t wndWidth = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.width : width;
    uint32_t wndHeight = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.height : height;
    if (state.width != wndWidth || state.height != wndHeight) {
      ResizeWin(reinterpret_cast<HWND>(state.playbackHandle), wndWidth, wndHeight);
      state.width = wndWidth;
      state.height = wndHeight;
    }
    if (state.visible != visible) {
      WinVisibility(reinterpret_cast<HWND>(state.playbackHandle), visible);
      state.visible = visible;
    }
    return state.playbackHandle;
  }

  auto& cfg = Configurator::Get().common.player;
  uint32_t wndPosX = cfg.forceWindowPos.enabled ? cfg.forceWindowPos.x : x;
  uint32_t wndPosY = cfg.forceWindowPos.enabled ? cfg.forceWindowPos.y : y;
  uint32_t wndWidth = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.width : width;
  uint32_t wndHeight = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.height : height;

  HWND currentHWND = CreateWin(wndWidth, wndHeight, wndPosX, wndPosY, visible);
  WinTitle(currentHWND, "Vulkan-GITS");
  HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(currentHWND, GWLP_HINSTANCE));
  currentHandle = reinterpret_cast<uint64_t>(currentHWND);
  currentInstance = reinterpret_cast<uint64_t>(hInstance);

  m_WindowMap[handle] = {currentHandle, wndWidth, wndHeight, visible};
  m_InstanceMap[instance] = currentInstance;

  return currentHandle;
#endif

  return 0;
}

void WindowService::UpdateWindow(uint64_t handle, int32_t width, int32_t height, bool visible) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
  auto it = m_WindowMap.find(handle);
  if (it != m_WindowMap.end()) {
    auto& cfg = Configurator::Get().common.player;
    uint32_t wndWidth = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.width : width;
    uint32_t wndHeight = cfg.forceWindowSize.enabled ? cfg.forceWindowSize.height : height;

    auto& state = it->second;
    if (state.width != wndWidth || state.height != wndHeight) {
      ResizeWin(reinterpret_cast<HWND>(state.playbackHandle), wndWidth, wndHeight);
      state.width = wndWidth;
      state.height = wndHeight;
    }
    if (state.visible != visible) {
      WinVisibility(reinterpret_cast<HWND>(state.playbackHandle), visible);
      state.visible = visible;
    }
  }
#endif
}

uint64_t WindowService::GetCurrentWindowHandle(uint64_t captureWindow) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
  auto it = m_WindowMap.find(captureWindow);
  if (it != m_WindowMap.end()) {
    return it->second.playbackHandle;
  }
#endif
  return 0;
}

uint64_t WindowService::GetCurrentInstance(uint64_t captureInstance) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
  auto it = m_InstanceMap.find(captureInstance);
  if (it != m_InstanceMap.end()) {
    return it->second;
  }
#endif
  return 0;
}

} // namespace vulkan
} // namespace gits
