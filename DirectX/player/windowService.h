// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <windows.h>

namespace gits {
namespace DirectX {

class WindowService {
public:
  HWND createWindow(HWND captureHwnd, int width, int height);
  HWND getCurrentHwnd(HWND captureHwnd);

private:
  HWND currentWindow_{NULL};
  HWND captureWindow_{NULL};
};

} // namespace DirectX
} // namespace gits
