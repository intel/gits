// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowService.h"
#include "message_pump.h"
#include "gits.h"
#include "playerManager.h"

namespace gits {
namespace DirectX {

HWND WindowService::createWindow(HWND captureHwnd, int width, int height) {

  captureWindow_ = captureHwnd;
  int wndWidth = gits::Config::Get().common.player.forceWindowSize.enabled
                     ? gits::Config::Get().common.player.forceWindowSize.width
                     : width;
  int wndHeight = gits::Config::Get().common.player.forceWindowSize.enabled
                      ? gits::Config::Get().common.player.forceWindowSize.height
                      : height;
  int wndPosX = gits::Config::Get().common.player.forceWindowPos.enabled
                    ? gits::Config::Get().common.player.forceWindowPos.x
                    : 10;
  int wndPosY = gits::Config::Get().common.player.forceWindowPos.enabled
                    ? gits::Config::Get().common.player.forceWindowPos.y
                    : 10;

  currentWindow_ = CreateWin(wndWidth, wndHeight, wndPosX, wndPosY, true);
  WinTitle(currentWindow_, "DX12-GITS");
  return currentWindow_;
}

HWND WindowService::getCurrentHwnd(HWND captureHwnd) {
  GITS_ASSERT(captureHwnd == captureWindow_);
  return currentWindow_;
}

} // namespace DirectX
} // namespace gits
