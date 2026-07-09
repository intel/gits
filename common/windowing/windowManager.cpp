// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowManager.h"

#include "playerWindow.h"
#include "platform.h"

#include <algorithm>
#include <memory>

namespace gits {
namespace windowing {

std::unique_ptr<PlayerWindow> CreatePlayerWindowXlib(
    int x, int y, int width, int height, bool visible);
std::unique_ptr<PlayerWindow> CreatePlayerWindowXcb(
    int x, int y, int width, int height, bool visible);
std::unique_ptr<PlayerWindow> CreatePlayerWindowWayland(
    int x, int y, int width, int height, bool visible);

WindowManager& WindowManager::Get() {
  static WindowManager manager;
  return manager;
}

std::pair<uint64_t, uint64_t> WindowManager::CreatePlayerWindow(
    WindowProtocol protocol, int x, int y, int width, int height, bool visible) {
#if defined GITS_PLATFORM_X11
  std::unique_ptr<PlayerWindow> window;

  switch (protocol) {
  case WindowProtocol::Xlib:
    window = CreatePlayerWindowXlib(x, y, width, height, visible);
    break;
  case WindowProtocol::Xcb:
    window = CreatePlayerWindowXcb(x, y, width, height, visible);
    break;
  case WindowProtocol::Wayland:
    window = CreatePlayerWindowWayland(x, y, width, height, visible);
    break;
  case WindowProtocol::Win:
  default:
    break;
  }

  if (!window) {
    return std::make_pair(0ULL, 0ULL);
  }

  auto handles = window->NativeHandles();
  m_Windows.push_back({protocol, std::move(window)});
  return handles;
#else
  (void)protocol;
  (void)x;
  (void)y;
  (void)width;
  (void)height;
  (void)visible;
  return std::make_pair(0ULL, 0ULL);
#endif
}

void WindowManager::ResizeWindow(WindowProtocol protocol,
                                 uint64_t nativeDisplay,
                                 uint64_t nativeWindow,
                                 uint32_t width,
                                 uint32_t height) {
  (void)protocol;

  auto it = std::find_if(m_Windows.begin(), m_Windows.end(),
                         [nativeDisplay, nativeWindow](const auto& w) {
                           auto handles = w.window->NativeHandles();
                           return handles.first == nativeDisplay && handles.second == nativeWindow;
                         });

  if (it != m_Windows.end()) {
    it->window->Resize(width, height);
  }
}

void WindowManager::SetTitle(const std::string& title) {
  for (auto& window : m_Windows) {
    window.window->SetTitle(title);
  }
}

std::vector<WindowEvent> WindowManager::PollEvents() {
  std::vector<WindowEvent> events;
  for (auto& window : m_Windows) {
    auto windowEvents = window.window->Poll();
    events.insert(events.end(), windowEvents.begin(), windowEvents.end());
  }
  return events;
}

void WindowManager::DestroyAllWindows() {
  m_Windows.clear();
}

} // namespace windowing
} // namespace gits
