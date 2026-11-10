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
  ~WindowManager() {}

  // Offscreen mode is opt-in per API: an API enables it only when its replay can run without a
  // native window (e.g. by creating window-less swapchains). While enabled, `CreatePlayerWindow`
  // returns null handles without logging an error, and the other window operations are no-ops
  // because no window is ever tracked. Callers must treat null handles as "no window".
  // Disabled by default so APIs that still require a window are unaffected.
  void SetOffscreen(bool offscreen) {
    m_Offscreen = offscreen;
  }
  bool IsOffscreen() const {
    return m_Offscreen;
  }

  // `visualId` is the X visual of an Xlib window, the screen's default one for 0. Other protocols
  // ignore it.
  std::pair<uint64_t, uint64_t> CreatePlayerWindow(WindowProtocol protocol,
                                                   int x,
                                                   int y,
                                                   int width,
                                                   int height,
                                                   bool visible,
                                                   uint64_t visualId = 0,
                                                   bool changeDisplayMode = true);
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
  bool m_Offscreen{false};
};

} // namespace windowing
} // namespace gits
