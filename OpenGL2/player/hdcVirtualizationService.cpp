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
    m_WindowVisible = m_InitialVisible;
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
            // shrink it back (SetGeometry() fires for every successfully-captured sample - see
            // WindowGeometryTrackingService::Capture - not just an actual position/size/
            // visibility change, but the captured window itself never changed here either
            // way).
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

void HdcVirtualizationService::SetGeometry(uint64_t window,
                                           int x,
                                           int y,
                                           int width,
                                           int height,
                                           bool visible,
                                           const std::vector<uint64_t>& closedWindows) {
  // Every recorded window is aliased onto this one player window (see this class's header
  // comment), so samples from the application's main window arrive interleaved with samples from
  // other windows of the same process (for example a small hidden or zero-area
  // helper, sampled once per frame between the main window's samples). Only the main window's
  // samples are applied, including its own hides and minimizes. The main window is the first one
  // shown (visible, positive area); before that every positive-area sample is applied, since an
  // application's main window is commonly sized while still hidden, before its first show.
  //
  // The main window keeps the role until it is reported destroyed or hidden (closedWindows,
  // from any later sample) or its own sample is hidden; the next window sampled shown then
  // takes over. How often other windows present never moves the role: a shown secondary window
  // may present any number of times between main window frames.
  std::lock_guard<std::mutex> lock(m_ResizeMutex);
  if (m_MainWindow != 0 &&
      std::find(closedWindows.begin(), closedWindows.end(), m_MainWindow) != closedWindows.end()) {
    m_MainWindow = 0;
  }
  m_SampledWindow = window;
  if (window == 0) {
    return; // Not captured: the swap it was taken for is presented, and geometry is unchanged.
  }
  const WindowSample sample{x, y, width, height, visible};
  if (m_MainWindow == 0) {
    if (sample.IsShown()) {
      m_MainWindow = window;
      m_HadMainWindow = true;
      ApplySample(sample);
    } else if (m_HadMainWindow) {
      // Waiting for a replacement main window: other windows' samples stay ignored, as while the
      // previous main window held the role.
    } else if (sample.HasUsableSize()) {
      ApplySample(sample);
    } else if (visible) {
      ApplyVisibility(true);
    }
    return;
  }
  if (window == m_MainWindow) {
    ApplySample(sample);
    if (!sample.Visible) {
      m_MainWindow = 0;
    }
  }
}

bool HdcVirtualizationService::ShouldSuppressSwap() {
  std::lock_guard<std::mutex> lock(m_ResizeMutex);
  // WindowGeometryTrackingLayer records a sample for every swap, immediately before it, so the
  // pending sample is this swap's own. Taking it keeps the decision per swap.
  const uint64_t window = m_SampledWindow;
  m_SampledWindow = 0;
  if (window == 0 || window == m_MainWindow) {
    return false;
  }
  // Another window's back buffer is the shared player back buffer holding the main window's
  // frame in progress (or garbage); presenting it would replace the main window's last frame
  // until its next swap. Before any main window every swap is presented; between a main window
  // closing and its replacement being shown, the last main window frame stays on screen.
  return m_MainWindow != 0 || m_HadMainWindow;
}

void HdcVirtualizationService::ApplySample(const WindowSample& sample) {
  if (sample.HasUsableSize()) {
    m_HasRealGeometry = true;
  }
  if (m_NativeWindow == 0) {
    // Resolve() creates the window later, from these values.
    if (sample.HasUsableSize()) {
      m_WindowX = sample.X;
      m_WindowY = sample.Y;
      m_ClientWidth = sample.Width;
      m_ClientHeight = sample.Height;
    }
    m_InitialVisible = sample.Visible;
    return;
  }
  auto& windowManager = windowing::WindowManager::Get();
  if (sample.HasUsableSize()) {
    if (sample.X != m_WindowX || sample.Y != m_WindowY) {
      m_WindowX = sample.X;
      m_WindowY = sample.Y;
      windowManager.MoveWindow(windowing::WindowProtocol::Win, m_NativeDisplay, m_NativeWindow,
                               m_WindowX, m_WindowY);
    }
    if (sample.Width != m_ClientWidth || sample.Height != m_ClientHeight) {
      m_ClientWidth = sample.Width;
      m_ClientHeight = sample.Height;
      windowManager.ResizeWindow(windowing::WindowProtocol::Win, m_NativeDisplay, m_NativeWindow,
                                 static_cast<uint32_t>(m_ClientWidth + m_BorderExtraWidth),
                                 static_cast<uint32_t>(m_ClientHeight + m_BorderExtraHeight));
    }
  }
  ApplyVisibility(sample.Visible);
}

void HdcVirtualizationService::ApplyVisibility(bool visible) {
  if (m_NativeWindow == 0) {
    m_InitialVisible = visible;
    return;
  }
  if (visible != m_WindowVisible) {
    m_WindowVisible = visible;
    windowing::WindowManager::Get().SetWindowVisibility(windowing::WindowProtocol::Win,
                                                        m_NativeDisplay, m_NativeWindow, visible);
  }
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
    // Black-screen investigation fix, later corrected by a real-buffering-semantics review
    // finding: an app's *own* wglSetPixelFormat call used to be captured byte-for-byte and was
    // not a reliable source of the flags that actually matter for replay - a real repro against
    // a captured application's stream recorded through this backend captured a sparse
    // descriptor with dwFlags=0 (no PFD_DOUBLEBUFFER bit), which ChoosePixelFormat/
    // SetPixelFormat honored literally, silently picking a single-buffered format -
    // wglSwapBuffers then kept returning TRUE
    // (nothing crashes) while GL_FRONT and GL_BACK are the same buffer, so real,
    // correctly-rendered frames (confirmed present via glReadPixels) never reached the window's
    // visible surface: a solid black window for the stream's entire duration.
    //
    // The FIRST fix for that (now superseded) unconditionally forced PFD_DOUBLEBUFFER on here,
    // for every captured request - which broke the opposite, equally real case: a genuinely
    // single-buffered application that relies on its initial draw buffer being GL_FRONT and
    // never calls SwapBuffers at all. Forcing PFD_DOUBLEBUFFER made its initial draw buffer
    // GL_BACK instead, so ITS real output silently became invisible on replay - and, separately,
    // forcing PFD_DOUBLEBUFFER on top of a captured PFD_SUPPORT_GDI bit requests an outright
    // invalid combination (the two are mutually exclusive in Windows' generic implementation).
    //
    // The actual, durable fix lives on the RECORDER side instead (recorder/
    // pixelFormatCaptureLayer.cpp's PixelFormatCaptureLayer::Post(wglSetPixelFormatCommand&)):
    // rather than trusting the app's own (possibly sparse) `ppfd` argument, it reads back the
    // driver's own resolution of the format that was ACTUALLY set - via GetPixelFormat()+
    // DescribePixelFormat() on the real capture-time HDC, right after a successful
    // SetPixelFormat - and captures THAT instead. That descriptor is self-consistent by
    // construction (Windows itself produced it: real single- vs. double-buffering, no
    // PFD_SUPPORT_GDI/PFD_DOUBLEBUFFER conflict, valid iPixelType), so `requestedPfd` here no
    // longer needs its dwFlags/iPixelType "corrected" - only PFD_DRAW_TO_WINDOW/
    // PFD_SUPPORT_OPENGL still need forcing on, since this player is always driving one
    // player-owned, actually-visible window (see this method's own Resolve() and Decision 1's
    // single-window scope) regardless of what the app's own window/format looked like - a
    // genuine replay-time requirement no captured descriptor can be trusted to already satisfy
    // (e.g. an older stream recorded before this fix, whose captured descriptor is still the
    // app's own, possibly-sparse one; or nullptr's own genericPfd fallback above, which chooses
    // PFD_DOUBLEBUFFER deliberately, having no real captured request to preserve at all).
    PIXELFORMATDESCRIPTOR sanitizedPfd = *pfd;
    sanitizedPfd.dwFlags |= kPfdDrawToWindow | kPfdSupportOpenGL;
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
