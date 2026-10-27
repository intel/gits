// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "glxDrawableService.h"

#include "configurator.h"
#include "log.h"
#include "playerWindowX11Shared.h"
#include "windowManager.h"

#if defined GITS_PLATFORM_X11
#include <X11/Xlib.h>
#endif

#include <algorithm>
#include <stdexcept>

namespace gits::opengl2 {

namespace {
#if defined GITS_PLATFORM_X11
// Xlib's default handler ends the process, e.g. for a call naming an X resource of the recorded
// connection.
int LogXError(Display* display, XErrorEvent* event) {
  char text[256] = {};
  XGetErrorText(display, event->error_code, text, sizeof(text));
  LOG_WARNING << "GLX replay: X error " << text << " (request "
              << static_cast<int>(event->request_code) << "." << static_cast<int>(event->minor_code)
              << ")";
  return 0;
}
#endif
} // namespace

GlxDrawableService& GlxDrawableService::Get() {
  static GlxDrawableService instance;
  return instance;
}

Display* GlxDrawableService::GetDisplay() {
#if defined GITS_PLATFORM_X11
  static Display* const display = [] {
    Display* opened = windowing::GetPlayerX11Display();
    XSetErrorHandler(LogXError);
    return opened;
  }();
  return display;
#else
  return nullptr;
#endif
}

int GlxDrawableService::GetDefaultScreen() {
#if defined GITS_PLATFORM_X11
  return DefaultScreen(GetDisplay());
#else
  return 0;
#endif
}

uint64_t GlxDrawableService::Resolve(uint64_t recordedDrawable) {
  if (recordedDrawable == 0) {
    return 0;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  PlayerWindow& window = m_Windows[recordedDrawable];
  if (window.Drawable == 0) {
    Create(window);
  }
  return window.Drawable;
}

void GlxDrawableService::FitViewport(uint64_t liveDrawable, int width, int height) {
  if (liveDrawable == 0 || width <= 0 || height <= 0 ||
      Configurator::Get().common.player.forceWindowSize.enabled) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto it = std::find_if(m_Windows.begin(), m_Windows.end(), [&](const auto& entry) {
    return entry.second.Drawable == liveDrawable;
  });
  if (it == m_Windows.end()) {
    return;
  }
  PlayerWindow& window = it->second;
  if (window.HasGeometry) {
    return;
  }
  if (!window.Fitted) {
    window.Fitted = true;
    if (width == window.Width && height == window.Height) {
      return;
    }
    window.Width = width;
    window.Height = height;
  } else if (width <= window.Width && height <= window.Height) {
    return;
  } else {
    window.Width = (std::max)(window.Width, width);
    window.Height = (std::max)(window.Height, height);
  }
  windowing::WindowManager::Get().ResizeWindow(
      windowing::WindowProtocol::Xlib, window.NativeDisplay, window.Drawable,
      static_cast<uint32_t>(window.Width), static_cast<uint32_t>(window.Height));
}

void GlxDrawableService::SetGeometry(
    uint64_t recordedDrawable, int x, int y, int width, int height, bool visible) {
  if (recordedDrawable == 0 || width <= 0 || height <= 0) {
    return;
  }
  const auto& config = Configurator::Get().common.player;
  visible = visible || config.showWindowsWA;
  std::lock_guard<std::mutex> lock(m_Mutex);
  PlayerWindow& window = m_Windows[recordedDrawable];
  window.HasGeometry = true;
  if (window.Drawable == 0) {
    window.X = x;
    window.Y = y;
    window.Width = width;
    window.Height = height;
    window.Visible = visible;
    return;
  }
  auto& manager = windowing::WindowManager::Get();
  if (!config.forceWindowSize.enabled && (width != window.Width || height != window.Height)) {
    window.Width = width;
    window.Height = height;
    manager.ResizeWindow(windowing::WindowProtocol::Xlib, window.NativeDisplay, window.Drawable,
                         static_cast<uint32_t>(width), static_cast<uint32_t>(height));
  }
  if (!config.forceWindowPos.enabled && (x != window.X || y != window.Y)) {
    window.X = x;
    window.Y = y;
    manager.MoveWindow(windowing::WindowProtocol::Xlib, window.NativeDisplay, window.Drawable, x,
                       y);
  }
  if (visible != window.Visible) {
    window.Visible = visible;
    manager.SetWindowVisibility(windowing::WindowProtocol::Xlib, window.NativeDisplay,
                                window.Drawable, visible);
  }
}

void GlxDrawableService::RemoveWindows(const std::vector<uint64_t>& recordedWindows) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (uint64_t recordedWindow : recordedWindows) {
    const auto it = m_Windows.find(recordedWindow);
    if (it == m_Windows.end()) {
      continue;
    }
    if (it->second.Drawable != 0) {
      windowing::WindowManager::Get().SetWindowVisibility(
          windowing::WindowProtocol::Xlib, it->second.NativeDisplay, it->second.Drawable, false);
    }
    m_Windows.erase(it);
  }
}

void GlxDrawableService::Create(PlayerWindow& window) {
  GetDisplay();
  const auto& config = Configurator::Get().common.player;
  if (config.forceWindowPos.enabled) {
    window.X = static_cast<int>(config.forceWindowPos.x);
    window.Y = static_cast<int>(config.forceWindowPos.y);
  }
  if (config.forceWindowSize.enabled) {
    window.Width = static_cast<int>(config.forceWindowSize.width);
    window.Height = static_cast<int>(config.forceWindowSize.height);
  }
  const auto handles = windowing::WindowManager::Get().CreatePlayerWindow(
      windowing::WindowProtocol::Xlib, window.X, window.Y, window.Width, window.Height,
      window.Visible);
  if (handles.second == 0) {
    throw std::runtime_error("GlxDrawableService: player window creation failed");
  }
  window.NativeDisplay = handles.first;
  window.Drawable = handles.second;
}

} // namespace gits::opengl2
