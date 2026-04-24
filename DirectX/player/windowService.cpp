// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowService.h"
#include "playerManager.h"
#include "message_pump.h"
#include "configurator.h"

namespace gits {
namespace DirectX {

HWND WindowService::CreatePlayerWindow(HWND captureHwnd, int width, int height) {

  int wndWidth = Configurator::Get().common.player.forceWindowSize.enabled
                     ? Configurator::Get().common.player.forceWindowSize.width
                     : width;
  int wndHeight = Configurator::Get().common.player.forceWindowSize.enabled
                      ? Configurator::Get().common.player.forceWindowSize.height
                      : height;
  int wndPosX = Configurator::Get().common.player.forceWindowPos.enabled
                    ? Configurator::Get().common.player.forceWindowPos.x
                    : 10;
  int wndPosY = Configurator::Get().common.player.forceWindowPos.enabled
                    ? Configurator::Get().common.player.forceWindowPos.y
                    : 10;

  auto it = m_WindowMap.find(captureHwnd);
  if (it != m_WindowMap.end()) {
    ResizeWin(it->second, wndWidth, wndHeight);
    return it->second;
  }

  HWND newWindow = CreateWin(wndWidth, wndHeight, wndPosX, wndPosY, true);
  WinTitle(newWindow, "DX12-GITS");
  m_WindowMap[captureHwnd] = newWindow;
  return newWindow;
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
