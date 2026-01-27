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

class WindowService {
public:
  static WindowService& Get();

  // Creates a window associated with the given captureHandle
  bool Create(uintptr_t captureHandle, unsigned width, unsigned height);

  // Retrieves the HWND associated with the given captureHandle
  HWND GetHandle(uintptr_t captureHandle);

private:
  WindowService() = default;
  ~WindowService() = default;

  // Prevent copying and assignment
  WindowService(const WindowService&) = delete;
  WindowService& operator=(const WindowService&) = delete;

  std::unordered_map<uintptr_t, HWND> m_Windows;
};
