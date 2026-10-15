// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pixelFormatCaptureLayer.h"

#include "log.h"

// Hand-declared instead of including <windows.h>, which would redeclare HDC/PIXELFORMATDESCRIPTOR
// etc. against glTypes.h's own self-contained declarations of those same names (pulled in here
// transitively via layerAuto.h -> commandsAuto.h) - see glTypes.h's own header comment and
// hdcVirtualizationService.cpp's (player-side) identical precedent for why this backend avoids
// <windows.h> in any translation unit that also needs glTypes.h. GetPixelFormat/
// DescribePixelFormat are ordinary gdi32.dll exports; calling them through this hand-declared
// prototype (rather than <windows.h>'s own) is safe since PIXELFORMATDESCRIPTOR/HDC/UINT/BOOL/
// int are identical, ABI-compatible types either way.
extern "C" __declspec(dllimport) int __stdcall GetPixelFormat(HDC hdc);
extern "C" __declspec(dllimport) int __stdcall DescribePixelFormat(HDC hdc,
                                                                   int format,
                                                                   UINT nBytes,
                                                                   PIXELFORMATDESCRIPTOR* ppfd);

namespace gits::opengl2 {

// Background (see HdcVirtualizationService::EnsurePixelFormatSet's own header comment for
// the original "black-screen investigation" this responds to): that method used to sanitize a
// captured wglSetPixelFormat request by unconditionally OR-ing PFD_DOUBLEBUFFER (among other
// flags) into whatever dwFlags the app itself had captured. That blindly turns a genuinely
// single-buffered capture into a double-buffered replay - breaking an app that relies on its
// initial draw buffer being GL_FRONT and never calls SwapBuffers, whose real output silently
// becomes invisible once replayed into GL_BACK - and can request an outright invalid
// combination when the captured flags include PFD_SUPPORT_GDI (mutually exclusive with
// PFD_DOUBLEBUFFER on Windows' generic implementation).
//
// Root cause: an app's OWN `ppfd` argument to SetPixelFormat is not a reliable source of the
// format's real buffering semantics in the first place - a real repro against a captured
// application's stream captured a sparse descriptor with dwFlags == 0 (see
// EnsurePixelFormatSet's own comment), and the OS barely validates dwFlags at all, so
// capturing that byte-for-byte and later trying to "fix it up" at replay time (by forcing bits
// on) can never be fully correct in either
// direction. The actually authoritative source is the driver's own resolution of the format
// that got set - queryable, right after a successful SetPixelFormat, via
// GetPixelFormat()+DescribePixelFormat() on the very same (real, capture-time) HDC.
//
// Fix: read that authoritative descriptor back here, immediately after the real dispatch (every
// generated wrapper calls the real driver function unconditionally before its Post-layer loop -
// see recorderWrappersAuto.cpp.mako's own header comment), and replace the app's own captured
// `command.m_ppfd` with it before EncoderLayer (always the last-registered Post layer - see
// CaptureManager::LoadLayers()) serializes the command onto the wire. This means the stream now
// carries the format the driver ACTUALLY selected - self-consistent by construction (Windows
// itself produced it), so replay-time sanitization no longer needs to guess or force
// PFD_DOUBLEBUFFER at all; it only needs to add PFD_DRAW_TO_WINDOW/PFD_SUPPORT_OPENGL, which
// this player's own single always-visible window genuinely requires regardless of capture (see
// EnsurePixelFormatSet's own updated comment).
void PixelFormatCaptureLayer::Post(wglSetPixelFormatCommand& command) {
  if (command.m_Return.Value != 0 && command.m_ppfd.HasValue) {
    const int realFormat = GetPixelFormat(command.m_hdc.Value);
    if (realFormat != 0) {
      PIXELFORMATDESCRIPTOR realPfd{};
      realPfd.nSize = sizeof(realPfd);
      realPfd.nVersion = 1;
      if (DescribePixelFormat(command.m_hdc.Value, realFormat, sizeof(realPfd), &realPfd) != 0) {
        command.m_ppfd.Data.assign(1, realPfd);
      } else {
        LOG_WARNING << "PixelFormatCaptureLayer: DescribePixelFormat failed for format "
                    << realFormat
                    << " right after a successful SetPixelFormat - falling back to the "
                       "application's own captured PIXELFORMATDESCRIPTOR, which may be "
                       "sparse/incomplete";
      }
    } else {
      LOG_WARNING << "PixelFormatCaptureLayer: GetPixelFormat returned 0 right after a "
                     "successful SetPixelFormat - falling back to the application's own "
                     "captured PIXELFORMATDESCRIPTOR, which may be sparse/incomplete";
    }
  }
  Post(static_cast<Command&>(command));
}

} // namespace gits::opengl2
