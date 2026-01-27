// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowService.h"

#include <unordered_map>
#include <stdexcept>

WindowService& WindowService::Get() {
  static WindowService s_Instance;
  return s_Instance;
}

// Create a window and associate it with the captureHandle
bool WindowService::Create(uintptr_t captureHandle, unsigned width, unsigned height) {
  if (m_Windows.find(captureHandle) != m_Windows.end()) {
    return false;
  }

  // Set DPI awareness once
  static bool dpiSet = false;
  if (!dpiSet) {
    SetProcessDPIAware();
    dpiSet = true;
  }

  WNDCLASSA wc = {};
  wc.lpfnWndProc = DefWindowProcA;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.lpszClassName = "WindowServiceClass";

  if (!RegisterClassA(&wc)) {
    return false;
  }

  // Calculate window size for desired client area
  RECT rect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

  HWND hwnd =
      CreateWindowExA(0, "WindowServiceClass", "GITS CCode", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                      CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr,
                      nullptr, GetModuleHandle(nullptr), nullptr);

  if (!hwnd) {
    return false;
  }

  ShowWindow(hwnd, SW_SHOW);
  UpdateWindow(hwnd);

  m_Windows[captureHandle] = hwnd;
  return true;
}

// Retrieve the HWND associated with the captureHandle
HWND WindowService::GetHandle(uintptr_t captureHandle) {
  auto it = m_Windows.find(captureHandle);
  if (it == m_Windows.end()) {
    throw std::runtime_error("Handle not found");
  }
  return it->second;
}
