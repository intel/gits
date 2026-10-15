// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"

#include <atomic>
#include <mutex>

namespace gits::opengl2 {

// Scopes every recorded HDC value onto a single, player-owned real HDC - see
// docs/development/opengl2-player-skeleton-plan.md's Decision 4 and
// modern-opengl-backend-architecture.md's "Capture native OS handles as opaque scalar
// values, defer remapping" decision, which anticipates exactly this: "derive a replay-time
// HDC from a replay-time window the same way the legacy backend's UpdateHDC() does, rather
// than by remapping the raw value directly."
//
// Deliberately does not key off the recorded value at all: every distinct recorded HDC maps
// to the same one live HDC (see the plan's Decision 1 - single player-owned window for the
// skeleton). Generalizing this into a real per-recorded-value window pool is the named
// follow-on once a real scenario needs a second simultaneous window - Resolve(HDC)'s
// signature is already shaped for that backward-compatible extension.
class HdcVirtualizationService {
public:
  static HdcVirtualizationService& Get();

  // Returns the one player-owned, real HDC for any recorded (opaque, process-foreign) HDC
  // value - creating the player's window and deriving its HDC on first call, then returning
  // the same HDC for every subsequent call regardless of which recorded value is passed. The
  // recorded value itself is never dereferenced or compared against a live handle.
  HDC Resolve(HDC recordedValue);

  // Sets a real pixel format on the player-owned HDC, retrying on a later call if an earlier
  // one failed - see docs/development/opengl2-player-skeleton-plan.md's Decision 9. Guarded by
  // a mutex plus an explicit success flag rather than std::call_once: call_once's "once" flag
  // is consumed as soon as the callable returns, regardless of whether ChoosePixelFormat/
  // SetPixelFormat inside it actually succeeded, which would permanently wedge the HDC after a
  // single transient failure - a later, potentially
  // compatible wglSetPixelFormat request could then never retry. Once a format has actually
  // been set, Win32 forbids setting a second one on the same HDC, so success latches
  // permanently; failure does not latch anything and simply allows the next call to retry.
  //
  // requestedPfd, if non-null, is a real, replayed wglSetPixelFormat command's captured
  // PIXELFORMATDESCRIPTOR - the request the app itself made at capture time, re-negotiated
  // against the real replay-time driver via ChoosePixelFormat/SetPixelFormat exactly the way
  // OpenGL/common/wglFunctions.cpp's SelectPixelFormat()/CwglSetPixelFormat::Run() re-negotiate
  // the legacy backend's own captured attribs, rather than reusing the app's original numeric
  // format index verbatim (meaningless on a different driver's format table).
  //
  // A null requestedPfd means "no real request is available, please pick a generic fallback" -
  // it is NOT a substitute for "the captured call's ppfd was itself null". Only
  // wglCreateContext(ARB)'s runner is meant to pass null here, defensively, for an app that
  // never called SetPixelFormat at all before creating its context - in that case a generic
  // RGBA/double-buffered/depth format is chosen instead, exactly as before this decision.
  // WglSetPixelFormatRunner must NOT forward a captured null ppfd straight through to this
  // parameter: a captured `wglSetPixelFormat(hdc,
  // format, nullptr)` is itself an invalid request that failed at capture (no format was set,
  // no state changed), and conflating that with "pick a fallback" would incorrectly set/latch
  // a real pixel format for a call that made no state change at capture - which can then block
  // a later, legitimately-formatted wglSetPixelFormat call, since Windows only allows setting a
  // format once per HDC. WglSetPixelFormatRunner checks command.m_ppfd.HasValue itself and
  // never calls this method at all when the captured ppfd was null, preserving that failure.
  //
  // Returns whether a pixel format is set on the real HDC once this call returns (true even
  // when this call itself was a no-op because an earlier call already succeeded).
  bool EnsurePixelFormatSet(const PIXELFORMATDESCRIPTOR* requestedPfd);

  // Grows the player's single skeleton window's client area to be at least (width, height),
  // never shrinks it - see docs/development/opengl2-player-skeleton-plan.md's Decision 1.
  // Resolve() (above) creates the window at (10, 10)/640x480 only as its OWN, hardcoded
  // fallback default for m_WindowX/Y/m_ClientWidth/m_ClientHeight/m_InitialVisible - see that
  // method's own comment (hdcVirtualizationService.cpp): this backend now captures the app's
  // REAL Win32 window geometry at record time (WindowGeometryTrackingService, unlike the
  // legacy OpenGL backend's GetWindowParams(), OpenGL/common/wglFunctions.cpp, which this
  // mirrors), and normally overwrites those defaults via SetGeometry() below before Resolve()
  // ever actually creates the window - see WindowGeometryTrackingLayer's own header comment
  // for the ordering guarantee that makes that the common case. GrowToFit() below is this
  // service's fallback for the remaining case: a stream recorded before that capture existed,
  // or one where WindowFromDC/GetClientRect failed at capture time, leaving Resolve() with only
  // its own hardcoded default to create the window at.
  //
  // GlViewportRunner (commandRunnersCustom.cpp) calls this for every recorded
  // glViewport(0, 0, width, height) - a full, origin-anchored viewport call is not guaranteed to
  // equal the app's actual window/backbuffer size (partial/split-screen viewports, or an
  // off-screen FBO that happens to also start at the origin, would under- or over-report it),
  // but it is the closest substitute signal already present in every OpenGL2 stream without a
  // capture-side/schema change, and real apps reliably issue at least one full-window
  // glViewport(0, 0, W, H) call against the default framebuffer (most commonly once at startup
  // and/or once per frame for the main pass) - see this repro's own evidence (a stream
  // recorded through this backend: window was stuck at the hardcoded 640x480 and its HUD, laid
  // out in screen-space for a much larger real resolution, appeared corrupted/overlapping until
  // the window actually grew to match). Only ever grows (never shrinks) so a later, smaller
  // partial-viewport call cannot undo an earlier, correct full-window one.
  //
  // DEMOTED TO A DEFENSIVE FALLBACK now that WindowGeometryMetaRunner/SetGeometry() below feed
  // this service the application's REAL, captured Win32 window geometry (see
  // WindowGeometryMetaCommand's own header comment, windowGeometryMetaTypes.h) - kept, not removed,
  // only for a stream recorded before that capture existed, or one where WindowFromDC/
  // GetClientRect failed at capture time (WindowGeometryTrackingService's own once-only
  // warning covers that case) - see this method's own remaining caller for the exact condition.
  // Enforced, not just documented: once SetGeometry() below has actually applied real geometry
  // at least once, this method permanently no-ops (see m_HasRealGeometry) - otherwise a later,
  // origin-anchored viewport call against something other than the default framebuffer (e.g. a
  // large off-screen FBO/shadow map bound at (0, 0)) would grow the window past its already-
  // correct real size, and nothing would ever shrink it back (SetGeometry() only fires on an
  // actual captured change, and the real application window itself never changed in that case).
  void GrowToFit(int width, int height);

  // Sets the player's window geometry directly from a WindowGeometryMetaCommand decoded by
  // WindowGeometryMetaRunner (player/include/windowGeometryMetaRunner.h) - the real,
  // authoritative Win32 geometry captured at record time (see that command's own header
  // comment), replacing GrowToFit()'s glViewport-inferred substitute signal above wherever a
  // stream actually has one.
  //
  // Two cases, distinguished by whether Resolve() has created the player window yet:
  //   - Before Resolve() ever runs (the common case: WindowGeometryTrackingLayer captures in
  //     Pre(), giving its WindowGeometryMetaCommand a lower ordering key - and therefore an
  //     earlier replay position - than the wglMakeCurrent/wglCreateContext call that will
  //     first trigger Resolve(), see that layer's own header comment) - just updates
  //     m_ClientWidth/m_ClientHeight, the size Resolve() itself creates the window at, so the
  //     window is created at the REAL size immediately, with no growing.
  //   - After Resolve() has already created the window (a later resize/move, captured by
  //     WindowGeometryTrackingLayer's per-frame poll on wglSwapBuffers/wglSwapLayerBuffers) -
  //     resizes/repositions the already-live window directly. Unlike GrowToFit(), this can also
  //     SHRINK the window: this is real, authoritative captured geometry, not an inferred lower
  //     bound, so there is no risk of a smaller, non-representative call undoing a larger,
  //     correct one.
  //
  // x/y are the recorded window's screen position - see WindowManager::MoveWindow(), the same
  // shared primitive Vulkan's WindowService::MoveWin32Window() already delegates to. Live-
  // verified: before this parameter existed, a real
  // capture+replay round trip reproduced the correct SIZE (matching a mid-run resize) but the
  // player window still opened at Resolve()'s hardcoded (10, 10) instead of the app's actual
  // captured position - m_X/m_Y were encoded into WindowGeometryMetaCommand but
  // WindowGeometryMetaRunner::Run() never forwarded them here. Fixed by threading x/y through
  // exactly like width/height, immediately below.
  void SetGeometry(int x, int y, int width, int height, bool visible);

private:
  HdcVirtualizationService() = default;

  std::once_flag m_Init;
  std::mutex m_PixelFormatMutex;
  std::atomic<bool> m_PixelFormatSet{false};
  HDC m_RealHdc{};

  // Set once, in Resolve()'s std::call_once block, alongside m_RealHdc - see GrowToFit()'s own
  // comment for why these exist. m_BorderExtraWidth/Height is the one-time (title bar + frame)
  // window-vs-client-size delta measured right after window creation (WindowManager::
  // ResizeWindow()/PlayerWindowWin::Resize() resizes the whole window, not just its client area,
  // unlike CreatePlayerWindow()'s width/height, which IS a client size - see playerWindowWin.cpp's
  // constructor's own AdjustWindowRectEx call - so GrowToFit() must re-add this delta itself
  // before calling ResizeWindow(), or the client area would shrink by it on every resize).
  std::mutex m_ResizeMutex;
  // Set once by SetGeometry() (either branch) and never cleared - disables GrowToFit()'s
  // glViewport-inferred fallback for good once the application's REAL, authoritative Win32
  // geometry has actually been applied. Without this, a later origin-anchored viewport call
  // against something other than the default framebuffer (e.g. a large off-screen FBO/shadow
  // map bound at (0, 0)) would still grow the window past its already-correct real size - and
  // nothing would ever shrink it back, since SetGeometry() only fires on a genuine captured
  // change, and the captured application window itself never changed in that scenario.
  bool m_HasRealGeometry{false};
  uint64_t m_NativeDisplay{};
  uint64_t m_NativeWindow{};
  int m_BorderExtraWidth{0};
  int m_BorderExtraHeight{0};
  int m_ClientWidth{640};
  int m_ClientHeight{480};
  // Screen position Resolve()'s CreatePlayerWindow() call creates the window at - was
  // hardcoded (10, 10) until SetGeometry() below learned to also carry position (see that
  // method's own header comment for the live-verification finding that motivated this).
  int m_WindowX{10};
  int m_WindowY{10};
  // Read by Resolve()'s CreatePlayerWindow() call, written only by SetGeometry() before
  // Resolve() has run - see that method's own header comment.
  bool m_InitialVisible{true};
};

} // namespace gits::opengl2
