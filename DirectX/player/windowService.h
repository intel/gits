// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <windows.h>
#include <unordered_map>

namespace gits {
namespace DirectX {

class WindowService {
public:
  HWND CreatePlayerWindow(HWND captureHwnd, int width, int height);
  HWND GetCurrentHwnd(HWND captureHwnd);

private:
  std::unordered_map<HWND, HWND> m_WindowMap;
};

} // namespace DirectX
} // namespace gits
