// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "hdcVirtualizationService.h"

#include "windowManager.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

// Hand-declared instead of including <windows.h>, which would redeclare RECT/PROC/etc.
// against glTypes.h's own self-contained declarations of those same names - see glTypes.h's
// header comment, and recorderWrappersAuto.cpp.mako's identical GetCurrentThreadId
// precedent for why this backend avoids <windows.h> in any translation unit that also needs
// glTypes.h. PIXELFORMATDESCRIPTOR itself is already declared by glTypes.h (WGL needs it too).
typedef struct HWND__* HWND;
extern "C" __declspec(dllimport) HDC __stdcall GetDC(HWND hWnd);
extern "C" __declspec(dllimport) int __stdcall ChoosePixelFormat(HDC hdc,
                                                                 const PIXELFORMATDESCRIPTOR* ppfd);
extern "C" __declspec(dllimport) BOOL
    __stdcall SetPixelFormat(HDC hdc, int format, const PIXELFORMATDESCRIPTOR* ppfd);
extern "C" __declspec(dllimport) int __stdcall GetPixelFormat(HDC hdc);
// GrowToFit()'s own one-time window-vs-client border measurement - see this file's own top
// comment for why <windows.h> itself is avoided here.
extern "C" __declspec(dllimport) BOOL __stdcall GetWindowRect(HWND hWnd, RECT* lpRect);
extern "C" __declspec(dllimport) BOOL __stdcall GetClientRect(HWND hWnd, RECT* lpRect);

namespace gits::opengl2 {

HdcVirtualizationService& HdcVirtualizationService::Get() {
  static HdcVirtualizationService instance;
  return instance;
}

HDC HdcVirtualizationService::Resolve(HDC /*recordedValue*/) {
  std::call_once(m_Init, [this] {
    // Position/size default to an arbitrary (10, 10)/640x480 for this single-window skeleton -
    // see Decision 1's scope - but are overwritten by SetGeometry() (m_WindowX/Y/Width/Height)
    // below whenever a WindowGeometryMetaCommand reaches WindowGeometryMetaRunner before this
    // call_once first runs (the common case - see SetGeometry()'s own header comment), so the
    // window is normally created at the REAL captured position/size immediately, with no
    // growing/moving. GrowToFit() (see that method's own header comment) remains the fallback
    // for streams with no such command.
    auto handles = windowing::WindowManager::Get().CreatePlayerWindow(
        windowing::WindowProtocol::Win, m_WindowX, m_WindowY, m_ClientWidth, m_ClientHeight,
        m_InitialVisible);
    HWND hwnd = reinterpret_cast<HWND>(handles.second);
    // CreatePlayerWindow() already logs its own LOG_ERROR and returns {0, 0} on failure (see
    // WindowManager::CreatePlayerWindow()) - but a null HWND must not be allowed to reach
    // GetDC() here: on Windows, GetDC(NULL) does not fail, it returns a DC for the ENTIRE
    // SCREEN, so a failed player-window creation would otherwise silently redirect every
    // subsequent pixel-format/context/rendering operation onto the desktop instead of
    // visibly failing. Fail loudly and immediately
    // instead, once, for the one player-owned HDC every replayed HDC value resolves to.
    if (hwnd == nullptr) {
      throw std::runtime_error(
          "HdcVirtualizationService: player window creation failed - cannot derive a real "
          "HDC for replay (see the preceding LOG_ERROR from WindowManager::CreatePlayerWindow "
          "for the underlying cause)");
    }
    m_RealHdc = GetDC(hwnd);
    if (m_RealHdc == nullptr) {
      throw std::runtime_error(
          "HdcVirtualizationService: GetDC() failed on the player window - cannot derive a "
          "real HDC for replay");
    }
    // GrowToFit()'s own bookkeeping - see that method's and m_BorderExtraWidth/Height's header
    // comments. Measured once, right after creation, rather than re-derived from window-style
    // flags on every resize: cheaper, and self-correcting against whatever style
    // PlayerWindowWin actually used (border-on/off, exclusive fullscreen, ...) instead of
    // duplicating that logic here.
    m_NativeDisplay = handles.first;
    m_NativeWindow = handles.second;
    RECT windowRect{};
    RECT clientRect{};
    if (GetWindowRect(hwnd, &windowRect) && GetClientRect(hwnd, &clientRect)) {
      m_BorderExtraWidth =
          (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
      m_BorderExtraHeight =
          (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);
    }
  });
  return m_RealHdc;
}

void HdcVirtualizationService::GrowToFit(int width, int height) {
  if (width <= 0 || height <= 0) {
    return; // Not a real, usable size - e.g. a minimized-window glViewport(0, 0, 0, 0) some
            // apps issue; growing to zero/negative would be a no-op at best, UB at worst.
  }
  std::lock_guard<std::mutex> lock(m_ResizeMutex);
  if (m_HasRealGeometry) {
    return; // A WindowGeometryMetaCommand has already supplied the application's REAL,
            // authoritative geometry (see SetGeometry() below and m_HasRealGeometry's own
            // declaration comment) - this glViewport-inferred fallback must stay disabled from
            // then on, or an origin-anchored viewport call for something other than the default
            // framebuffer (e.g. a large off-screen FBO/shadow map bound at (0, 0)) would grow
            // the window past its real, already-correct size, with no later geometry command to
            // shrink it back (SetGeometry() only fires on an actual position/size/visibility
            // CHANGE - see WindowGeometryTrackingService::CaptureIfChanged - and the captured
            // window itself never changed here).
  }
  if (m_NativeWindow == 0) {
    return; // Resolve() hasn't created the window yet - can't happen in practice (a GL context
            // must already be current for any glViewport call to reach this runner at all, which
            // means Resolve() must already have run), but this method must not be the one to
            // trigger window creation with a still-unknown pixel format/HDC.
  }
  if (width <= m_ClientWidth && height <= m_ClientHeight) {
    return; // Already big enough on both axes - see this method's own declaration comment for
            // why this never shrinks.
  }
  m_ClientWidth = std::max(m_ClientWidth, width);
  m_ClientHeight = std::max(m_ClientHeight, height);
  windowing::WindowManager::Get().ResizeWindow(
      windowing::WindowProtocol::Win, m_NativeDisplay, m_NativeWindow,
      static_cast<uint32_t>(m_ClientWidth + m_BorderExtraWidth),
      static_cast<uint32_t>(m_ClientHeight + m_BorderExtraHeight));
}

void HdcVirtualizationService::SetGeometry(int x, int y, int width, int height, bool visible) {
  // Unlike GrowToFit()'s inferred lower bound, this is real, authoritative captured geometry
  // (a WindowGeometryMetaCommand), so an unusable width/height (e.g. a legitimately
  // zero-sized/minimized Win32 client area) must NOT discard the rest of it: position and
  // visibility are independent pieces of state and are always applied below. Only the resize
  // itself is guarded defensively - an actual zero/negative ResizeWindow call would be a
  // no-op at best, UB at worst (see GrowToFit()'s identical size guard).
  const bool hasUsableSize = width > 0 && height > 0;
  std::lock_guard<std::mutex> lock(m_ResizeMutex);
  // Latches permanently once a real geometry command has actually been applied (either
  // branch below) - see m_HasRealGeometry's own declaration comment and GrowToFit()'s new
  // guard against it. Set unconditionally here, before either branch, since both are equally
  // "real, authoritative captured geometry" as far as GrowToFit()'s fallback is concerned,
  // regardless of whether the size half of it was usable.
  m_HasRealGeometry = true;
  if (m_NativeWindow == 0) {
    // Resolve() hasn't created the window yet - the common case (see this method's own
    // declaration comment): just update the position/size/visibility Resolve() itself will
    // create the window at, rather than trying to move/resize a window that does not exist yet.
    m_WindowX = x;
    m_WindowY = y;
    if (hasUsableSize) {
      m_ClientWidth = width;
      m_ClientHeight = height;
    }
    m_InitialVisible = visible;
    return;
  }
  // Window already exists - a later resize/move captured by WindowGeometryTrackingLayer's
  // per-frame poll. Unlike GrowToFit(), this sets the size directly (can shrink) since it is
  // real, authoritative captured geometry, not an inferred lower bound.
  m_WindowX = x;
  m_WindowY = y;
  windowing::WindowManager::Get().MoveWindow(windowing::WindowProtocol::Win, m_NativeDisplay,
                                             m_NativeWindow, m_WindowX, m_WindowY);
  if (hasUsableSize) {
    m_ClientWidth = width;
    m_ClientHeight = height;
    windowing::WindowManager::Get().ResizeWindow(
        windowing::WindowProtocol::Win, m_NativeDisplay, m_NativeWindow,
        static_cast<uint32_t>(m_ClientWidth + m_BorderExtraWidth),
        static_cast<uint32_t>(m_ClientHeight + m_BorderExtraHeight));
  }
  windowing::WindowManager::Get().SetWindowVisibility(windowing::WindowProtocol::Win,
                                                      m_NativeDisplay, m_NativeWindow, visible);
}

bool HdcVirtualizationService::EnsurePixelFormatSet(const PIXELFORMATDESCRIPTOR* requestedPfd) {
  if (m_PixelFormatSet.load(std::memory_order_acquire)) {
    return true; // Already succeeded on an earlier call - Win32 forbids setting a second
                 // pixel format on the same HDC, so there is nothing left to (re)try.
  }
  std::lock_guard<std::mutex> lock(m_PixelFormatMutex);
  if (m_PixelFormatSet.load(std::memory_order_relaxed)) {
    return true; // Another thread won the race and already succeeded while this thread waited
                 // for the lock.
  }
  {
    // wglCreateContext(ARB) fails on an HDC with no pixel format set - a bare GetDC() is not
    // yet a "valid device context" for GL purposes, and Win32 forbids calling SetPixelFormat
    // more than once on the same HDC once one has actually been set (m_PixelFormatSet, checked
    // above, is exactly that "at most one successful call" latch - see this method's own
    // declaration comment for why it is not a std::call_once). PFD_* flag values and
    // PFD_TYPE_RGBA are from <wingdi.h>, hand-inlined for the same reason the rest of this file
    // avoids <windows.h> - see OpenGL/common/ptbl_wglLibrary.cpp's ChoosePixelFormat/
    // SetPixelFormat pair for the legacy backend's equivalent helper-window setup.
    constexpr DWORD kPfdDoubleBuffer = 0x00000001;
    constexpr DWORD kPfdDrawToWindow = 0x00000004;
    constexpr DWORD kPfdSupportOpenGL = 0x00000020;
    constexpr BYTE kPfdTypeRgba = 0;
    constexpr BYTE kPfdTypeColorIndex = 1;

    PIXELFORMATDESCRIPTOR genericPfd{};
    const PIXELFORMATDESCRIPTOR* pfd = requestedPfd;
    if (pfd == nullptr) {
      // Defensive fallback: no enabled command ever gave us a real request (either the app
      // never called SetPixelFormat, or it did but SetPixelFormat is not enabled for some
      // other reason) - reconstruct the same generic RGBA/double-buffered/depth guess this
      // method itself used unconditionally before Decision 9.
      genericPfd.nSize = sizeof(genericPfd);
      genericPfd.nVersion = 1;
      genericPfd.dwFlags = kPfdDrawToWindow | kPfdSupportOpenGL | kPfdDoubleBuffer;
      genericPfd.iPixelType = kPfdTypeRgba;
      genericPfd.cColorBits = 32;
      genericPfd.cDepthBits = 24;
      genericPfd.cStencilBits = 8;
      pfd = &genericPfd;
    }
    // Black-screen investigation fix: an app's *own* wglSetPixelFormat call is not a reliable
    // source of the flags that actually matter for replay, for two independent reasons, so a
    // captured `requestedPfd` is sanitized into a local, corrected copy rather than passed to
    // ChoosePixelFormat/SetPixelFormat verbatim:
    //   1. On real Windows, only `ppfd->iPixelType`/`cColorBits`/depth-ish fields are loosely
    //      cross-checked by SetPixelFormat - dwFlags is essentially unvalidated by the OS, and
    //      the *real* PFD_DOUBLEBUFFER request happens earlier, via wglChoosePixelFormat(ARB) -
    //      both currently unsupported/skipped commands (see ReportUnsupportedCommand's "degraded
    //      fidelity" warning for either). Apps commonly pass a sparse/reused descriptor to
    //      SetPixelFormat itself, since the OS mostly only honors the numeric format index. A
    //      real repro against a stream recorded through this backend captured exactly this:
    //      dwFlags=0 (no PFD_DOUBLEBUFFER bit), which ChoosePixelFormat/SetPixelFormat honored
    //      literally, silently picking a single-buffered format - wglSwapBuffers then kept
    //      returning TRUE (nothing crashes) while GL_FRONT and GL_BACK are the same buffer,
    //      so real, correctly-rendered frames (confirmed present via glReadPixels) never reach
    //      the window's visible surface: a solid black window for the stream's entire duration.
    //   2. Even when a captured descriptor's *bytes* look wrong altogether (e.g. an iPixelType
    //      outside the only two legal values, 0=RGBA/1=COLORINDEX) - plausible on some captures
    //      of this same sparse-descriptor pattern - blindly forwarding it to ChoosePixelFormat
    //      risks an equally arbitrary match.
    // This player is always driving one player-owned, actually-visible window (see this
    // method's own Resolve() and Decision 1's single-window scope), so PFD_DRAW_TO_WINDOW/
    // PFD_SUPPORT_OPENGL/PFD_DOUBLEBUFFER are always genuine replay-time requirements
    // regardless of what the app captured - OR them in rather than trusting dwFlags verbatim.
    PIXELFORMATDESCRIPTOR sanitizedPfd = *pfd;
    sanitizedPfd.dwFlags |= kPfdDrawToWindow | kPfdSupportOpenGL | kPfdDoubleBuffer;
    if (sanitizedPfd.iPixelType != kPfdTypeRgba && sanitizedPfd.iPixelType != kPfdTypeColorIndex) {
      sanitizedPfd.iPixelType = kPfdTypeRgba;
    }
    pfd = &sanitizedPfd;
    // Re-negotiate the request against the real replay-time driver instead of reusing the
    // app's own captured format index (*ipfd - meaningless on a different driver's format
    // table) - mirrors OpenGL/common/wglFunctions.cpp's SelectPixelFormat() GDI fallback path
    // (ChoosePixelFormat+SetPixelFormat with a reconstructed PIXELFORMATDESCRIPTOR), simplified
    // to skip that function's ARB-attrib round trip since a full descriptor is already in hand.
    const int format = ChoosePixelFormat(m_RealHdc, pfd);
    if (format != 0 && SetPixelFormat(m_RealHdc, format, pfd)) {
      // Only latch success once SetPixelFormat has actually succeeded - a failed
      // ChoosePixelFormat/SetPixelFormat leaves m_PixelFormatSet false, so a later call with a
      // different (possibly compatible) requestedPfd can still retry instead of being
      // permanently wedged.
      m_PixelFormatSet.store(true, std::memory_order_release);
    }
  }
  return GetPixelFormat(m_RealHdc) != 0;
}

} // namespace gits::opengl2
