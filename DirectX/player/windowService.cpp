// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowService.h"
#include "configurator.h"
#include "log.h"
#include "windowManager.h"

namespace gits {
namespace DirectX {

static void ApplyForcedPlayerWindowSize(int& width, int& height) {
  const auto& forceWindowSize = Configurator::Get().common.player.forceWindowSize;
  if (forceWindowSize.enabled) {
    width = static_cast<int>(forceWindowSize.width);
    height = static_cast<int>(forceWindowSize.height);
  }
}

HWND WindowService::CreatePlayerWindow(HWND captureHwnd, int width, int height) {
  int wndWidth = width;
  int wndHeight = height;
  ApplyForcedPlayerWindowSize(wndWidth, wndHeight);

  auto it = m_WindowMap.find(captureHwnd);
  if (it != m_WindowMap.end()) {
    ResizePlayerWindow(it->second, width, height);
    return it->second;
  }

  const auto& forceWindowPos = Configurator::Get().common.player.forceWindowPos;
  int posX = forceWindowPos.enabled ? forceWindowPos.x : 10;
  int posY = forceWindowPos.enabled ? forceWindowPos.y : 10;

  auto handles = windowing::WindowManager::Get().CreatePlayerWindow(
      windowing::WindowProtocol::Win, posX, posY, wndWidth, wndHeight, true);
  HWND newWindow = reinterpret_cast<HWND>(handles.second);
  windowing::WindowManager::Get().SetTitle("DX12-GITS");
  m_WindowMap[captureHwnd] = newWindow;
  return newWindow;
}

void WindowService::ResizePlayerWindow(HWND playerHwnd, int width, int height) {
  if (playerHwnd == nullptr) {
    return;
  }

  int wndWidth = width;
  int wndHeight = height;
  ApplyForcedPlayerWindowSize(wndWidth, wndHeight);
  if (wndWidth <= 0 || wndHeight <= 0) {
    return;
  }

  const uint64_t hinstance = reinterpret_cast<uint64_t>(GetModuleHandle(nullptr));
  windowing::WindowManager::Get().ResizeWindow(windowing::WindowProtocol::Win, hinstance,
                                               reinterpret_cast<uint64_t>(playerHwnd), wndWidth,
                                               wndHeight);
}

HWND WindowService::GetCurrentHwnd(HWND captureHwnd) {
  auto it = m_WindowMap.find(captureHwnd);
  if (it == m_WindowMap.end()) {
    LOG_WARNING << "Cannot find window for hWnd from capture: " << std::hex << captureHwnd
                << std::dec;
    return 0;
  }
  return it->second;
}

} // namespace DirectX
} // namespace gits
