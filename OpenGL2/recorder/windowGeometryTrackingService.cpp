// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowGeometryTrackingService.h"

#include "captureManager.h"
#include "log.h"
#include "windowGeometryMetaTypes.h"

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

namespace gits::opengl2 {

namespace {
std::mutex g_WarnOnceMutex;
bool g_WarnedWindowFromDcFailure = false;
bool g_WarnedGeometryQueryFailure = false;
} // namespace

WindowGeometryTrackingService& WindowGeometryTrackingService::Get() {
  static WindowGeometryTrackingService instance;
  return instance;
}

bool WindowGeometryTrackingService::CaptureIfChanged(uint32_t threadId, void* hdc) {
  HWND hwnd = WindowFromDC(static_cast<HDC>(hdc));
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
                     "GlViewportRunner's glViewport-inferred fallback will apply on replay "
                     "instead";
    }
    return false;
  }

  RECT clientRect{};
  RECT windowRect{};
  if (!GetClientRect(hwnd, &clientRect) || !GetWindowRect(hwnd, &windowRect)) {
    std::lock_guard<std::mutex> lock(g_WarnOnceMutex);
    if (!g_WarnedGeometryQueryFailure) {
      g_WarnedGeometryQueryFailure = true;
      LOG_WARNING << "WindowGeometryTrackingService: GetClientRect/GetWindowRect failed for "
                     "at least one resolved HWND - real window geometry could not be captured "
                     "for it; GlViewportRunner's glViewport-inferred fallback will apply on "
                     "replay instead";
    }
    return false;
  }

  // Matches window_handle::get_dimensions's own math (OpenGL/common/windowing.cpp) - the
  // legacy backend's equivalent capture: position is the window rect's origin offset by the
  // client rect's own (usually zero) origin, size is the client rect's extent (excludes the
  // title bar/border, matching what the player's client-area-sized window creation actually
  // wants - see HdcVirtualizationService's own m_ClientWidth/m_ClientHeight, which are
  // explicitly a client size, not a window size).
  const int32_t x = windowRect.left + clientRect.left;
  const int32_t y = windowRect.top + clientRect.top;
  const int32_t width = clientRect.right - clientRect.left;
  const int32_t height = clientRect.bottom - clientRect.top;
  // IsWindowVisible's own contract only promises a nonzero return on success, never
  // specifically TRUE (1) - comparing with `== TRUE` would silently record a visible window as
  // hidden if it ever returned some other nonzero value, so normalize with a nonzero check
  // instead (`!= 0`, not `== TRUE`).
  const bool visible = IsWindowVisible(hwnd) != 0;

  std::lock_guard<std::mutex> lock(m_Mutex);
  if (m_HaveLast && m_LastX == x && m_LastY == y && m_LastWidth == width &&
      m_LastHeight == height && m_LastVisible == visible) {
    return false; // Unchanged since the last recorded capture - mirrors Vulkan's
                  // RecordUpdateIfChanged, so a per-frame poll (wglSwapBuffers) does not
                  // re-record an identical geometry on every single frame of a static window.
  }
  m_HaveLast = true;
  m_LastX = x;
  m_LastY = y;
  m_LastWidth = width;
  m_LastHeight = height;
  m_LastVisible = visible;

  WindowGeometryMetaCommand command(threadId);
  command.m_Key = CaptureManager::Get().CreateCommandKey();
  command.m_X.Value = x;
  command.m_Y.Value = y;
  command.m_Width.Value = width;
  command.m_Height.Value = height;
  command.m_Visible.Value = visible;
  CaptureManager::Get().RecordCommand(command, new WindowGeometryMetaSerializer(command));
  return true;
}

} // namespace gits::opengl2
