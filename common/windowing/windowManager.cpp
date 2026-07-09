// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowManager.h"

#include "playerWindow.h"
#include "log.h"
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
std::unique_ptr<PlayerWindow> CreatePlayerWindowWin(
    int x, int y, int width, int height, bool visible);

namespace {

const char* WindowProtocolName(WindowProtocol protocol) {
  switch (protocol) {
  case WindowProtocol::Xlib:
    return "Xlib";
  case WindowProtocol::Xcb:
    return "Xcb";
  case WindowProtocol::Wayland:
    return "Wayland";
  case WindowProtocol::Win:
    return "Win";
  }
  return "unknown";
}

} // namespace

WindowManager& WindowManager::Get() {
  static WindowManager manager;
  return manager;
}

PlayerWindow* WindowManager::FindPlayerWindow(WindowProtocol protocol,
                                              uint64_t nativeDisplay,
                                              uint64_t nativeWindow) {
  auto it = std::find_if(m_Windows.begin(), m_Windows.end(),
                         [protocol, nativeDisplay, nativeWindow](const auto& w) {
                           if (w.protocol != protocol) {
                             return false;
                           }
                           auto handles = w.window->NativeHandles();
                           return handles.first == nativeDisplay && handles.second == nativeWindow;
                         });
  return it != m_Windows.end() ? it->window.get() : nullptr;
}

std::pair<uint64_t, uint64_t> WindowManager::CreatePlayerWindow(
    WindowProtocol protocol, int x, int y, int width, int height, bool visible) {
  std::unique_ptr<PlayerWindow> window;
  switch (protocol) {
#if defined GITS_PLATFORM_WINDOWS
  case WindowProtocol::Win:
    window = CreatePlayerWindowWin(x, y, width, height, visible);
    break;
#endif
#if defined GITS_PLATFORM_X11
  case WindowProtocol::Xlib:
    window = CreatePlayerWindowXlib(x, y, width, height, visible);
    break;
  case WindowProtocol::Xcb:
    window = CreatePlayerWindowXcb(x, y, width, height, visible);
    break;
  case WindowProtocol::Wayland:
    window = CreatePlayerWindowWayland(x, y, width, height, visible);
    break;
#endif
  default:
    break;
  }

  if (!window) {
    LOG_ERROR << "Failed to create player window (protocol: " << WindowProtocolName(protocol)
              << ")";
    return std::make_pair(0ULL, 0ULL);
  }

  auto handles = window->NativeHandles();
  m_Windows.push_back({protocol, std::move(window)});
  return handles;
}

void WindowManager::ResizeWindow(WindowProtocol protocol,
                                 uint64_t nativeDisplay,
                                 uint64_t nativeWindow,
                                 uint32_t width,
                                 uint32_t height) {
  if (PlayerWindow* window = FindPlayerWindow(protocol, nativeDisplay, nativeWindow)) {
    window->Resize(width, height);
  }
}

void WindowManager::MoveWindow(
    WindowProtocol protocol, uint64_t nativeDisplay, uint64_t nativeWindow, int32_t x, int32_t y) {
  if (PlayerWindow* window = FindPlayerWindow(protocol, nativeDisplay, nativeWindow)) {
    window->SetPosition(x, y);
  }
}

void WindowManager::SetWindowVisibility(WindowProtocol protocol,
                                        uint64_t nativeDisplay,
                                        uint64_t nativeWindow,
                                        bool visible) {
  if (PlayerWindow* window = FindPlayerWindow(protocol, nativeDisplay, nativeWindow)) {
    window->SetVisibility(visible);
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
