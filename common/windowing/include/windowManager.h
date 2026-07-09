// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "playerWindow.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace gits {
namespace windowing {

enum class WindowProtocol {
  Win,
  Xlib,
  Xcb,
  Wayland
};

class WindowManager {
public:
  static WindowManager& Get();
  ~WindowManager() = default;

  std::pair<uint64_t, uint64_t> CreatePlayerWindow(
      WindowProtocol protocol, int x, int y, int width, int height, bool visible);
  void ResizeWindow(WindowProtocol protocol,
                    uint64_t nativeDisplay,
                    uint64_t nativeWindow,
                    uint32_t width,
                    uint32_t height);
  void MoveWindow(
      WindowProtocol protocol, uint64_t nativeDisplay, uint64_t nativeWindow, int32_t x, int32_t y);
  void SetWindowVisibility(WindowProtocol protocol,
                           uint64_t nativeDisplay,
                           uint64_t nativeWindow,
                           bool visible);
  void SetTitle(const std::string& title);
  std::vector<WindowEvent> PollEvents();
  void DestroyAllWindows();

private:
  WindowManager() = default;
  WindowManager(const WindowManager& other) = delete;
  WindowManager& operator=(const WindowManager& other) = delete;

  PlayerWindow* FindPlayerWindow(WindowProtocol protocol,
                                 uint64_t nativeDisplay,
                                 uint64_t nativeWindow);

  struct ManagedWindow {
    WindowProtocol protocol{};
    std::unique_ptr<PlayerWindow> window;
  };

  std::vector<ManagedWindow> m_Windows;
};

} // namespace windowing
} // namespace gits
