// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>
#include <mutex>

namespace gits::opengl2 {

// Captures the application's real Win32 window geometry (see WindowGeometryMetaCommand's own
// header comment, windowGeometryMetaTypes.h, for the full cross-backend rationale) and records a
// WindowGeometryMetaCommand whenever it changes. Owned/invoked by WindowGeometryTrackingLayer
// (windowGeometryTrackingLayer.h) from Pre() hooks on wglSetPixelFormat/wglMakeCurrent (first
// capture, mirroring the legacy OpenGL backend's own wglSetPixelFormat-time capture - see
// OpenGL/common/wglFunctions.cpp's CwglSetPixelFormat constructor) and wglSwapBuffers/
// wglSwapLayerBuffers (per-frame poll, mirroring Vulkan's WindowTrackingService::
// UpdateWindowsForPresent - Vulkan/recorder/windowTrackingService.h).
//
// Deliberately simpler than Vulkan's own WindowTrackingService: this backend has exactly one
// real application window to ever track (there is no VkSurfaceKHR/VkSwapchainKHR-shaped
// indirection - an HDC's owning HWND, resolved via WindowFromDC, is the only window identity
// OpenGL2 ever has), so there is no per-surface map, just one last-recorded state and one
// "record if changed" entry point used from every hook point.
class WindowGeometryTrackingService {
public:
  static WindowGeometryTrackingService& Get();

  // `hdc` is a real, live, capture-time HDC (never a recorded/replayed value - this class is
  // recorder-only). Resolves its owning HWND via WindowFromDC, queries real Win32 geometry via
  // GetClientRect/GetWindowRect/IsWindowVisible, and records a WindowGeometryMetaCommand only
  // if this is the first successful capture or the geometry actually changed since the last
  // one - mirrors Vulkan's RecordUpdateIfChanged's own no-op-when-unchanged behavior, so a
  // per-frame poll (wglSwapBuffers) does not spam the stream with an identical record on every
  // single frame of a perfectly static window.
  //
  // Silently does nothing (never throws, never logs more than once) if `hdc`'s owning window
  // cannot be resolved or queried - see .cpp for the specific, once-only warning this emits;
  // GlViewportRunner's own glViewport-inferred fallback (player/include/commandRunnersCustom.h)
  // still applies on replay for a stream where this capture never succeeded.
  //
  // Returns true iff a WindowGeometryMetaCommand was actually recorded (first successful
  // capture, or a real change since the last one). Every caller (WindowGeometryTrackingLayer's
  // four Pre() overrides) needs this to know whether it must push its own hooked command's key
  // later via CaptureManager::UpdateCommandKey() - see those call sites' own comments for why.
  bool CaptureIfChanged(uint32_t threadId, void* hdc);

private:
  WindowGeometryTrackingService() = default;

  std::mutex m_Mutex;
  bool m_HaveLast{false};
  int32_t m_LastX{};
  int32_t m_LastY{};
  int32_t m_LastWidth{};
  int32_t m_LastHeight{};
  bool m_LastVisible{};
};

} // namespace gits::opengl2
