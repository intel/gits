// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "systemApi.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <unistd.h>
#endif

namespace gits::opengl2 {

#ifdef _WIN32

uint32_t CurrentThreadId() {
  return static_cast<uint32_t>(GetCurrentThreadId());
}

uint32_t GetLastSystemError() {
  return static_cast<uint32_t>(GetLastError());
}

void SetLastSystemError(uint32_t error) {
  SetLastError(static_cast<DWORD>(error));
}

bool GetWindowClientSize(void* dc, int32_t& width, int32_t& height) {
  const HWND window = WindowFromDC(static_cast<HDC>(dc));
  RECT rect{};
  if (window == nullptr || GetClientRect(window, &rect) == 0) {
    return false;
  }
  width = rect.right - rect.left;
  height = rect.bottom - rect.top;
  return true;
}

#else

uint32_t CurrentThreadId() {
  return static_cast<uint32_t>(gettid());
}

uint32_t GetLastSystemError() {
  return static_cast<uint32_t>(errno);
}

void SetLastSystemError(uint32_t error) {
  errno = static_cast<int>(error);
}

bool GetWindowClientSize(void* /*dc*/, int32_t& /*width*/, int32_t& /*height*/) {
  // TODO: Query the drawable's size once GLX drawables are captured.
  return false;
}

#endif

} // namespace gits::opengl2
