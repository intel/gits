// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "hdcVirtualizationService.h"

#include "windowManager.h"

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

namespace gits::opengl2 {

HdcVirtualizationService& HdcVirtualizationService::Get() {
  static HdcVirtualizationService instance;
  return instance;
}

HDC HdcVirtualizationService::Resolve(HDC /*recordedValue*/) {
  std::call_once(m_Init, [this] {
    // Position/size are arbitrary for this single-window skeleton - see Decision 1's scope.
    auto handles = windowing::WindowManager::Get().CreatePlayerWindow(
        windowing::WindowProtocol::Win, 10, 10, 640, 480, true);
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
  });
  return m_RealHdc;
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
    PIXELFORMATDESCRIPTOR genericPfd{};
    const PIXELFORMATDESCRIPTOR* pfd = requestedPfd;
    if (pfd == nullptr) {
      // Defensive fallback: no enabled command ever gave us a real request (either the app
      // never called SetPixelFormat, or it did but SetPixelFormat is not enabled for some
      // other reason) - reconstruct the same generic RGBA/double-buffered/depth guess this
      // method itself used unconditionally before Decision 9.
      constexpr DWORD kPfdDoubleBuffer = 0x00000001;
      constexpr DWORD kPfdDrawToWindow = 0x00000004;
      constexpr DWORD kPfdSupportOpenGL = 0x00000020;
      constexpr BYTE kPfdTypeRgba = 0;
      genericPfd.nSize = sizeof(genericPfd);
      genericPfd.nVersion = 1;
      genericPfd.dwFlags = kPfdDrawToWindow | kPfdSupportOpenGL | kPfdDoubleBuffer;
      genericPfd.iPixelType = kPfdTypeRgba;
      genericPfd.cColorBits = 32;
      genericPfd.cDepthBits = 24;
      genericPfd.cStencilBits = 8;
      pfd = &genericPfd;
    }
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
