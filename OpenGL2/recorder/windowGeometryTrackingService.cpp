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

#include <utility>

namespace gits::opengl2 {

namespace {
std::mutex g_WarnOnceMutex;
bool g_WarnedWindowFromDcFailure = false;
bool g_WarnedGeometryQueryFailure = false;

void RecordSample(uint32_t threadId,
                  uint64_t key,
                  HWND hwnd,
                  const RECT* client,
                  const RECT* window,
                  bool visible,
                  std::vector<uint64_t> closedWindows) {
  WindowGeometryMetaCommand command(threadId);
  command.m_Key = key;
  command.m_Window.Value = reinterpret_cast<uint64_t>(hwnd);
  command.m_ClosedWindows.Data = std::move(closedWindows);
  command.m_ClosedWindows.HasValue = true;
  if (client != nullptr && window != nullptr) {
    // Matches window_handle::get_dimensions's own math (OpenGL/common/windowing.cpp) - the
    // legacy backend's equivalent capture: position is the window rect's origin offset by the
    // client rect's own (usually zero) origin, size is the client rect's extent (excludes the
    // title bar/border, matching what the player's client-area-sized window creation actually
    // wants - see HdcVirtualizationService's own m_ClientWidth/m_ClientHeight, which are
    // explicitly a client size, not a window size).
    command.m_X.Value = window->left + client->left;
    command.m_Y.Value = window->top + client->top;
    command.m_Width.Value = client->right - client->left;
    command.m_Height.Value = client->bottom - client->top;
    command.m_Visible.Value = visible;
  }
  CaptureManager::Get().RecordCommand(command, new WindowGeometryMetaSerializer(command));
}
} // namespace

WindowGeometryTrackingService& WindowGeometryTrackingService::Get() {
  static WindowGeometryTrackingService instance;
  return instance;
}

std::vector<uint64_t> WindowGeometryTrackingService::ClosedWindows(uint64_t sampled) {
  std::vector<uint64_t> closed;
  std::lock_guard<std::mutex> lock(m_WindowsMutex);
  if (sampled != 0) {
    m_Windows.insert(sampled);
  }
  for (uint64_t window : m_Windows) {
    const HWND hwnd = reinterpret_cast<HWND>(window);
    if (window != sampled && (IsWindow(hwnd) == 0 || IsWindowVisible(hwnd) == 0)) {
      closed.push_back(window);
    }
  }
  return closed;
}

void WindowGeometryTrackingService::Capture(uint32_t threadId, void* hdc, uint64_t geometryKey) {
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
    // Still recorded, as an unknown window: the player decides each swap from the sample taken
    // for it, and must not reuse an earlier window's decision.
    RecordSample(threadId, geometryKey, nullptr, nullptr, nullptr, false, ClosedWindows(0));
    return;
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
    // See the WindowFromDC-failure branch above.
    RecordSample(threadId, geometryKey, nullptr, nullptr, nullptr, false, ClosedWindows(0));
    return;
  }

  // IsWindowVisible's own contract only promises a nonzero return on success, never
  // specifically TRUE (1) - comparing with `== TRUE` would silently record a visible window as
  // hidden if it ever returned some other nonzero value, so normalize with a nonzero check
  // instead (`!= 0`, not `== TRUE`).
  const bool visible = IsWindowVisible(hwnd) != 0;

  // Unconditional - no cache, no skip-if-unchanged - see this method's own header comment
  // (windowGeometryTrackingService.h) for why a stateful dedup here is unsafe against a
  // smaller, not-yet-processed geometryKey still in flight on another thread.
  RecordSample(threadId, geometryKey, hwnd, &clientRect, &windowRect, visible,
               ClosedWindows(reinterpret_cast<uint64_t>(hwnd)));
}

} // namespace gits::opengl2
