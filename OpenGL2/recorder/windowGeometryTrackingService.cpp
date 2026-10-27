// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowGeometryTrackingService.h"

#include "captureManager.h"
#include "glxDrawableIdentityService.h"
#include "log.h"
#include "systemApi.h"
#include "windowGeometryMetaTypes.h"

#ifdef _WIN32
// Real <windows.h> is safe in this translation unit (unlike hdcVirtualizationService.cpp/
// mapTrackingWrappers.cpp, which hand-declare instead): this file only includes
// captureManager.h (deliberately glTypes.h-free, see that header's own comment) and
// windowGeometryMetaTypes.h (a hand-written meta-command with no gl.xml-derived fields, so no
// glTypes.h dependency either) - never commandsAuto.h/openglDriversAuto.h, which is what
// actually pulls in the conflicting glTypes.h declarations elsewhere in this backend. Callers
// (windowGeometryTrackingLayer.cpp, which DOES need commandsAuto.h for the typed Pre() hooks)
// pass their glTypes.h HDC values across this boundary as a plain `void*`, which is where the
// avoidance actually happens.
#include <windows.h>
#else
// For the same reason, Xlib is safe here too.
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>

#include <cstdlib>
#endif

#include <algorithm>
#include <utility>

namespace gits::opengl2 {

namespace {
#ifdef _WIN32
std::mutex g_WarnOnceMutex;
bool g_WarnedWindowFromDcFailure = false;
bool g_WarnedGeometryQueryFailure = false;

LPCSTR LifetimeProperty() {
  static const ATOM atom = GlobalAddAtomA("GITS_OpenGL2_WindowLifetime");
  return MAKEINTATOM(atom);
}

uint32_t Lifetime(HWND hwnd) {
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetPropA(hwnd, LifetimeProperty())));
}

uint64_t Identity(HWND hwnd, uint32_t lifetime) {
  return (static_cast<uint64_t>(lifetime) << 32) |
         static_cast<uint32_t>(reinterpret_cast<uintptr_t>(hwnd));
}
#endif

void SetPresent(WindowGeometryMetaCommand& command) {
  command.m_Windows.HasValue = true;
  command.m_X.HasValue = true;
  command.m_Y.HasValue = true;
  command.m_Width.HasValue = true;
  command.m_Height.HasValue = true;
  command.m_Visible.HasValue = true;
  command.m_DestroyedWindows.HasValue = true;
}
} // namespace

WindowGeometryTrackingService& WindowGeometryTrackingService::Get() {
  static WindowGeometryTrackingService instance;
  return instance;
}

#ifdef _WIN32
uint64_t WindowGeometryTrackingService::Track(void* window) {
  const HWND hwnd = static_cast<HWND>(window);
  std::lock_guard<std::mutex> lock(m_WindowsMutex);
  uint32_t lifetime = Lifetime(hwnd);
  if (lifetime == 0) {
    if (SetPropA(hwnd, LifetimeProperty(),
                 reinterpret_cast<HANDLE>(static_cast<uintptr_t>(m_LastLifetime + 1))) == 0) {
      return 0; // Destroyed before it could be tagged.
    }
    lifetime = ++m_LastLifetime;
  }
  const uint64_t identity = Identity(hwnd, lifetime);
  m_Windows.try_emplace(identity, reinterpret_cast<uint64_t>(hwnd));
  return identity;
}

WindowGeometryTrackingService::Sample WindowGeometryTrackingService::Query(void* hdc) {
  const HWND hwnd = WindowFromDC(static_cast<HDC>(hdc));
  if (hwnd == nullptr) {
    // Common and expected for an HDC that isn't a real, on-screen window DC (e.g. a
    // memory/bitmap DC, or an already-destroyed window) - not every wglMakeCurrent/
    // wglSwapBuffers call is guaranteed to have a resolvable HWND. Warn once, not per-call,
    // mirroring this backend's other warn-once services (e.g. ObjectNameMapService).
    std::lock_guard<std::mutex> lock(g_WarnOnceMutex);
    if (!g_WarnedWindowFromDcFailure) {
      g_WarnedWindowFromDcFailure = true;
      LOG_WARNING << "WindowGeometryTrackingService: WindowFromDC returned NULL for at least "
                     "one captured HDC - real window geometry could not be captured for it; "
                     "the player's glViewport-inferred fallback will apply on replay instead";
    }
    return {};
  }

  // The lifetime is taken before the geometry and checked again after it, so geometry queried
  // while the window was destroyed, or its HWND reused, is never recorded under either identity.
  const uint64_t identity = Track(hwnd);
  if (identity == 0) {
    return {};
  }
  return Measure(hwnd, identity).value_or(Sample{});
}

std::optional<WindowGeometryTrackingService::Sample> WindowGeometryTrackingService::Measure(
    void* window, uint64_t identity) {
  const HWND hwnd = static_cast<HWND>(window);
  RECT clientRect{};
  RECT windowRect{};
  if (!GetClientRect(hwnd, &clientRect) || !GetWindowRect(hwnd, &windowRect)) {
    std::lock_guard<std::mutex> lock(g_WarnOnceMutex);
    if (!g_WarnedGeometryQueryFailure) {
      g_WarnedGeometryQueryFailure = true;
      LOG_WARNING << "WindowGeometryTrackingService: GetClientRect/GetWindowRect failed for "
                     "at least one resolved HWND - real window geometry could not be captured "
                     "for it; the player's glViewport-inferred fallback will apply on replay "
                     "instead";
    }
    return std::nullopt;
  }
  // IsWindowVisible's own contract only promises a nonzero return on success, never
  // specifically TRUE (1) - comparing with `== TRUE` would silently record a visible window as
  // hidden if it ever returned some other nonzero value, so normalize with a nonzero check
  // instead (`!= 0`, not `== TRUE`).
  const bool visible = IsWindowVisible(hwnd) != 0;
  if (Identity(hwnd, Lifetime(hwnd)) != identity) {
    return std::nullopt;
  }

  // Matches window_handle::get_dimensions's own math (OpenGL/common/windowing.cpp) - the
  // legacy backend's equivalent capture: position is the window rect's origin offset by the
  // client rect's own (usually zero) origin, size is the client rect's extent (excludes the
  // title bar/border, matching what the player's client-area-sized window creation actually
  // wants - see HdcVirtualizationService's own m_ClientWidth/m_ClientHeight, which are
  // explicitly a client size, not a window size).
  Sample sample;
  sample.Window = identity;
  sample.X = windowRect.left + clientRect.left;
  sample.Y = windowRect.top + clientRect.top;
  sample.Width = clientRect.right - clientRect.left;
  sample.Height = clientRect.bottom - clientRect.top;
  sample.Visible = visible;
  return sample;
}

WindowGeometryTrackingService::OtherWindows WindowGeometryTrackingService::ScanOtherWindows(
    const std::vector<uint64_t>& sampled) {
  OtherWindows others;
  std::lock_guard<std::mutex> lock(m_WindowsMutex);
  for (auto it = m_Windows.begin(); it != m_Windows.end();) {
    const uint64_t identity = it->first;
    const HWND hwnd = reinterpret_cast<HWND>(it->second);
    if (std::find(sampled.begin(), sampled.end(), identity) != sampled.end()) {
      ++it;
      continue;
    }
    if (IsWindow(hwnd) == 0 || Identity(hwnd, Lifetime(hwnd)) != identity) {
      others.Destroyed.push_back(identity);
      it = m_Windows.erase(it);
      continue;
    }
    if (const std::optional<Sample> sample = Measure(hwnd, identity)) {
      others.Samples.push_back(*sample);
    }
    ++it;
  }
  return others;
}

WindowGeometryTrackingService::Sample WindowGeometryTrackingService::QueryGlx(Display*, uint64_t) {
  return {};
}

std::vector<uint64_t> WindowGeometryTrackingService::ScanGlxWindows(Display*,
                                                                    const std::vector<uint64_t>&) {
  return {};
}
#else
// WGL windows exist only on Windows.
uint64_t WindowGeometryTrackingService::Track(void*) {
  return 0;
}

WindowGeometryTrackingService::Sample WindowGeometryTrackingService::Query(void*) {
  return {};
}

std::optional<WindowGeometryTrackingService::Sample> WindowGeometryTrackingService::Measure(
    void*, uint64_t) {
  return std::nullopt;
}

WindowGeometryTrackingService::OtherWindows WindowGeometryTrackingService::ScanOtherWindows(
    const std::vector<uint64_t>&) {
  return {};
}

WindowGeometryTrackingService::Sample WindowGeometryTrackingService::QueryGlx(Display* display,
                                                                              uint64_t drawable) {
  // The replies carry the errors, so a drawable that is not an X drawable, such as a GLX pixmap
  // or pbuffer, or not a window, never reaches the application's X error handler.
  xcb_connection_t* connection = XGetXCBConnection(display);
  const auto window = static_cast<xcb_window_t>(drawable);
  const xcb_get_geometry_cookie_t geometryCookie = xcb_get_geometry(connection, window);
  const xcb_get_window_attributes_cookie_t attributesCookie =
      xcb_get_window_attributes(connection, window);
  xcb_generic_error_t* error = nullptr;
  xcb_get_geometry_reply_t* geometry = xcb_get_geometry_reply(connection, geometryCookie, &error);
  std::free(error);
  error = nullptr;
  xcb_get_window_attributes_reply_t* attributes =
      xcb_get_window_attributes_reply(connection, attributesCookie, &error);
  std::free(error);
  error = nullptr;
  Sample sample;
  if (geometry != nullptr && attributes == nullptr) {
    // An X pixmap, of which the player creates one of the same size.
    sample.Window = drawable;
    sample.Width = geometry->width;
    sample.Height = geometry->height;
  } else if (geometry != nullptr) {
    // The root coordinates of the window itself, not of a window manager's frame, whose extents
    // differ between window managers. The player places its window there, see
    // GlxDrawableService.
    xcb_translate_coordinates_reply_t* origin = xcb_translate_coordinates_reply(
        connection, xcb_translate_coordinates(connection, window, geometry->root, 0, 0), &error);
    std::free(error);
    if (origin != nullptr) {
      sample.Window = drawable;
      sample.X = origin->dst_x;
      sample.Y = origin->dst_y;
      sample.Width = geometry->width;
      sample.Height = geometry->height;
      sample.Visible = attributes->map_state == XCB_MAP_STATE_VIEWABLE;
      sample.XWindow = true;
    }
    std::free(origin);
  }
  std::free(geometry);
  std::free(attributes);
  return sample;
}

std::vector<uint64_t> WindowGeometryTrackingService::ScanGlxWindows(
    Display* display, const std::vector<uint64_t>& sampled) {
  const uint32_t server = GlxDrawableIdentityService::Get().Server(display);
  std::vector<std::pair<uint64_t, uint64_t>> windows;
  {
    std::lock_guard<std::mutex> lock(m_WindowsMutex);
    for (const auto& [identity, window] : m_Windows) {
      if (identity >> 32 == server &&
          std::find(sampled.begin(), sampled.end(), identity) == sampled.end()) {
        windows.emplace_back(identity, window);
      }
    }
  }
  if (windows.empty()) {
    return {};
  }

  // Window attributes, unlike geometry, are not found for a pixmap that reuses a window's XID.
  xcb_connection_t* connection = XGetXCBConnection(display);
  std::vector<xcb_get_window_attributes_cookie_t> cookies;
  cookies.reserve(windows.size());
  for (const auto& [identity, window] : windows) {
    cookies.push_back(xcb_get_window_attributes(connection, static_cast<xcb_window_t>(window)));
  }
  std::vector<uint64_t> destroyed;
  for (size_t index = 0; index < windows.size(); ++index) {
    xcb_generic_error_t* error = nullptr;
    xcb_get_window_attributes_reply_t* attributes =
        xcb_get_window_attributes_reply(connection, cookies[index], &error);
    if (error != nullptr && error->error_code == XCB_WINDOW) {
      destroyed.push_back(windows[index].first);
    }
    std::free(attributes);
    std::free(error);
  }

  std::lock_guard<std::mutex> lock(m_WindowsMutex);
  std::erase_if(destroyed, [this](uint64_t identity) { return m_Windows.erase(identity) == 0; });
  return destroyed;
}
#endif

void WindowGeometryTrackingService::CaptureGlx(uint32_t threadId,
                                               Display* display,
                                               std::initializer_list<uint64_t> drawables,
                                               uint64_t geometryKey) {
  std::vector<uint64_t> sampled;
  for (uint64_t drawable : drawables) {
    if (drawable != 0 && std::find(sampled.begin(), sampled.end(), drawable) == sampled.end()) {
      sampled.push_back(drawable);
    }
  }
  auto& captureManager = CaptureManager::Get();
  if (sampled.empty()) {
    captureManager.SkipCommandKey(geometryKey);
    return;
  }
  const LastSystemErrorGuard lastErrorGuard;
  auto& identities = GlxDrawableIdentityService::Get();
  WindowGeometryMetaCommand command(threadId);
  command.m_Key = geometryKey;
  SetPresent(command);
  command.m_CallSamples.Value = static_cast<uint32_t>(sampled.size());
  for (uint64_t drawable : sampled) {
    {
      std::lock_guard<std::mutex> lock(m_GlxWindowsMutex);
      if (const auto it = m_GlxWindows.find(identities.Identity(display, drawable));
          it != m_GlxWindows.end()) {
        drawable = it->second;
      }
    }
    Sample sample = QueryGlx(display, drawable);
    sample.Window = identities.Identity(display, sample.Window);
    if (sample.XWindow) {
      std::lock_guard<std::mutex> lock(m_WindowsMutex);
      m_Windows.try_emplace(sample.Window, drawable);
    }
    command.m_Windows.Data.push_back(sample.Window);
    command.m_X.Data.push_back(sample.X);
    command.m_Y.Data.push_back(sample.Y);
    command.m_Width.Data.push_back(sample.Width);
    command.m_Height.Data.push_back(sample.Height);
    command.m_Visible.Data.push_back(sample.Visible ? 1 : 0);
  }
  captureManager.RecordCommand(command, new WindowGeometryMetaSerializer(command));
  RecordDestroyed(threadId, ScanGlxWindows(display, command.m_Windows.Data));
}

void WindowGeometryTrackingService::RecordDestroyed(uint32_t threadId,
                                                    std::vector<uint64_t> destroyed) {
  if (destroyed.empty()) {
    return;
  }
  // The capture's key may predate uses of these windows recorded by other threads; a key reserved
  // now, after their destruction was seen, follows every such use.
  auto& captureManager = CaptureManager::Get();
  WindowGeometryMetaCommand command(threadId);
  command.m_Key = captureManager.CreateCommandKey();
  SetPresent(command);
  command.m_DestroyedWindows.Data = std::move(destroyed);
  captureManager.RecordCommand(command, new WindowGeometryMetaSerializer(command));
}

void WindowGeometryTrackingService::AddGlxWindow(Display* display,
                                                 uint64_t glxWindow,
                                                 uint64_t window) {
  const uint64_t identity = GlxDrawableIdentityService::Get().Identity(display, glxWindow);
  std::lock_guard<std::mutex> lock(m_GlxWindowsMutex);
  m_GlxWindows[identity] = window;
}

void WindowGeometryTrackingService::RemoveGlxWindow(Display* display, uint64_t glxWindow) {
  const uint64_t identity = GlxDrawableIdentityService::Get().Identity(display, glxWindow);
  std::lock_guard<std::mutex> lock(m_GlxWindowsMutex);
  m_GlxWindows.erase(identity);
}

void WindowGeometryTrackingService::Capture(uint32_t threadId,
                                            std::initializer_list<void*> hdcs,
                                            uint64_t geometryKey) {
  const LastSystemErrorGuard lastErrorGuard;
  WindowGeometryMetaCommand command(threadId);
  command.m_Key = geometryKey;
  SetPresent(command);
  // Unconditional - no cache, no skip-if-unchanged - see this method's own header comment
  // (windowGeometryTrackingService.h) for why a stateful dedup here is unsafe against a
  // smaller, not-yet-processed geometryKey still in flight on another thread. A DC whose window
  // is unknown is still sampled, as window 0: the player decides each call from the samples
  // taken for it, and must not reuse an earlier call's.
  const auto add = [&command](const Sample& sample) {
    command.m_Windows.Data.push_back(sample.Window);
    command.m_X.Data.push_back(sample.X);
    command.m_Y.Data.push_back(sample.Y);
    command.m_Width.Data.push_back(sample.Width);
    command.m_Height.Data.push_back(sample.Height);
    command.m_Visible.Data.push_back(sample.Visible ? 1 : 0);
  };
  for (void* hdc : hdcs) {
    add(Query(hdc));
  }
  command.m_CallSamples.Value = static_cast<uint32_t>(hdcs.size());
  OtherWindows others = ScanOtherWindows(command.m_Windows.Data);
  for (const Sample& sample : others.Samples) {
    add(sample);
  }
  CaptureManager::Get().RecordCommand(command, new WindowGeometryMetaSerializer(command));
  RecordDestroyed(threadId, std::move(others.Destroyed));
}

} // namespace gits::opengl2
