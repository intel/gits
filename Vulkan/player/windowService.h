// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <unordered_map>

namespace gits {
namespace vulkan {

class WindowService {
public:
  uint64_t SetWindow(uint32_t protocol,
                     uint64_t handle,
                     uint64_t instance,
                     int32_t x,
                     int32_t y,
                     int32_t width,
                     int32_t height,
                     bool visible);
  void UpdateWindow(
      uint64_t handle, int32_t x, int32_t y, int32_t width, int32_t height, bool visible);
  uint64_t GetCurrentWindowHandle(uint64_t captureWindow);
  uint64_t GetCurrentInstance(uint64_t captureInstance);

private:
#ifdef GITS_PLATFORM_WINDOWS
  std::pair<uint64_t, uint64_t> CreateWin32Window(
      int32_t x, int32_t y, int32_t width, int32_t height, bool visible);
  void ResizeWin32Window(uint64_t hinstance, uint64_t hwnd, uint32_t width, uint32_t height);
  void MoveWin32Window(uint64_t hinstance, uint64_t hwnd, int32_t x, int32_t y);
  void SetWin32WindowVisibility(uint64_t hinstance, uint64_t hwnd, bool visible);
#endif
#ifdef GITS_PLATFORM_LINUX
  std::pair<uint64_t, uint64_t> CreateXlibWindow(
      int32_t x, int32_t y, int32_t width, int32_t height, bool visible);
  void ResizeXlibWindow(uint64_t display, uint64_t window, uint32_t width, uint32_t height);
  std::pair<uint64_t, uint64_t> CreateXcbWindow(
      int32_t x, int32_t y, int32_t width, int32_t height, bool visible);
  void ResizeXcbWindow(uint64_t connection, uint64_t window, uint32_t width, uint32_t height);
  std::pair<uint64_t, uint64_t> CreateWaylandWindow(
      int32_t x, int32_t y, int32_t width, int32_t height, bool visible);
#endif

  struct WindowState {
    uint64_t playbackHandle{};
    uint64_t playbackInstance{};
    int32_t x{};
    int32_t y{};
    uint32_t width{};
    uint32_t height{};
    bool visible{};
  };

  std::unordered_map<uint64_t, WindowState> m_WindowMap;
  std::unordered_map<uint64_t, uint64_t> m_InstanceMap;
};

} // namespace vulkan
} // namespace gits
