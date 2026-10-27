// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pixelFormatCaptureLayer.h"

#include "captureManager.h"
#include "glEnumsAuto.h"
#include "log.h"
#include "openglDriversAuto.h"
#include "pixelFormatMetaTypes.h"
#include "systemApi.h"

#include <atomic>
#include <span>

#ifdef _WIN32
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
#endif

namespace gits::opengl2 {

namespace {
// The legacy backend's GetPixelFormatParams() set (OpenGL/common/wglFunctions.cpp) minus the
// window/bitmap/swap-method attributes a pbuffer does not need, plus render-to-texture binding.
constexpr int kPbufferPixelFormatAttributes[] = {
    WGL_DRAW_TO_PBUFFER_ARB,     WGL_SUPPORT_OPENGL_ARB, WGL_ACCELERATION_ARB,
    WGL_PIXEL_TYPE_ARB,          WGL_DOUBLE_BUFFER_ARB,  WGL_RED_BITS_ARB,
    WGL_GREEN_BITS_ARB,          WGL_BLUE_BITS_ARB,      WGL_ALPHA_BITS_ARB,
    WGL_ACCUM_BITS_ARB,          WGL_DEPTH_BITS_ARB,     WGL_STENCIL_BITS_ARB,
    WGL_SAMPLE_BUFFERS_ARB,      WGL_SAMPLES_ARB,        WGL_BIND_TO_TEXTURE_RGB_ARB,
    WGL_BIND_TO_TEXTURE_RGBA_ARB};

// The same set for a window, which needs neither pbuffer drawing nor texture binding, plus the
// stereo and auxiliary buffers its descriptor also describes, and the swap method, which decides
// what the back buffer holds after a swap.
constexpr int kWindowPixelFormatAttributes[] = {
    WGL_DRAW_TO_WINDOW_ARB, WGL_SUPPORT_OPENGL_ARB, WGL_ACCELERATION_ARB,   WGL_PIXEL_TYPE_ARB,
    WGL_DOUBLE_BUFFER_ARB,  WGL_STEREO_ARB,         WGL_RED_BITS_ARB,       WGL_GREEN_BITS_ARB,
    WGL_BLUE_BITS_ARB,      WGL_ALPHA_BITS_ARB,     WGL_ACCUM_BITS_ARB,     WGL_DEPTH_BITS_ARB,
    WGL_STENCIL_BITS_ARB,   WGL_AUX_BUFFERS_ARB,    WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB,
    WGL_SWAP_METHOD_ARB};

// Records the meta command even when nothing could be queried: its key must be used.
void RecordPixelFormat(uint64_t key,
                       uint32_t threadId,
                       HDC hdc,
                       int pixelFormat,
                       std::span<const int> queried,
                       bool pbuffer) {
  // The queries, logging and recording below may set the thread's last error, which the
  // application may read after its own call.
  const LastSystemErrorGuard lastErrorGuard;
  PixelFormatMetaCommand meta(threadId);
  meta.m_Key = key;
  meta.m_Attributes.HasValue = true;
  auto getAttributes = g_DispatchTable.wglGetPixelFormatAttribivARB.load(std::memory_order_acquire);
  // Before any context is current, an unresolved extension function cannot be resolved.
  const bool resolvable =
      getAttributes != &ResolveAndCall_wglGetPixelFormatAttribivARB ||
      g_DispatchTable.wglGetCurrentContext.load(std::memory_order_acquire)() != nullptr;
  // One attribute per query: the whole query fails if any attribute belongs to an extension the
  // driver lacks (WGL_ARB_multisample, WGL_ARB_render_texture).
  for (int attribute : queried) {
    int value = 0;
    if (resolvable && getAttributes(hdc, pixelFormat, 0, 1, &attribute, &value)) {
      meta.m_Attributes.Data.push_back(attribute);
      meta.m_Attributes.Data.push_back(value);
    }
  }
  if (pbuffer && meta.m_Attributes.Data.empty()) {
    static std::atomic<bool> warned{false};
    if (!warned.exchange(true)) {
      LOG_WARNING << "PixelFormatCaptureLayer: wglGetPixelFormatAttribivARB failed for a "
                     "pbuffer pixel format - replay will reuse its format index";
    }
  }
  CaptureManager::Get().RecordCommand(meta, new PixelFormatMetaSerializer(meta));
}

// Resolves wglGetPixelFormatAttribivARB while the context a make-current call attached is
// current; see Pre(wglSetPixelFormatCommand&).
void ResolveAttributeQuery(bool succeeded, HGLRC context, HDC dc) {
  auto getAttributes = g_DispatchTable.wglGetPixelFormatAttribivARB.load(std::memory_order_acquire);
  if (!succeeded || context == nullptr ||
      getAttributes != &ResolveAndCall_wglGetPixelFormatAttribivARB) {
    return;
  }
  const LastSystemErrorGuard lastErrorGuard;
  // The format index is ignored for this attribute.
  const int attribute = WGL_NUMBER_PIXEL_FORMATS_ARB;
  int value = 0;
  getAttributes(dc, 1, 0, 1, &attribute, &value);
}
} // namespace

void PixelFormatCaptureLayer::Pre(wglCreatePbufferARBCommand& command) {
  RecordPixelFormat(CaptureManager::Get().TakePendingPreHookKey(), command.m_ThreadId,
                    command.m_hDC.Value, command.m_iPixelFormat.Value,
                    kPbufferPixelFormatAttributes, true);
  Pre(static_cast<Command&>(command));
}

void PixelFormatCaptureLayer::Pre(wglCreatePbufferEXTCommand& command) {
  RecordPixelFormat(CaptureManager::Get().TakePendingPreHookKey(), command.m_ThreadId,
                    command.m_hDC.Value, command.m_iPixelFormat.Value,
                    kPbufferPixelFormatAttributes, true);
  Pre(static_cast<Command&>(command));
}

// The descriptor Post() records cannot express multisampling, so replay chooses the window's
// format from these attributes; without them (no context was ever current, say), from the
// descriptor.
//
// Applications commonly set the format of the window they render to with no context current,
// after a throwaway context loaded the WGL extensions, when wglGetPixelFormatAttribivARB can no
// longer be resolved. Each successful make-current call resolves it while a context is current.
void PixelFormatCaptureLayer::Pre(wglSetPixelFormatCommand& command) {
  RecordPixelFormat(CaptureManager::Get().TakePendingPixelFormatKey(), command.m_ThreadId,
                    command.m_hdc.Value, command.m_ipfd.Value, kWindowPixelFormatAttributes, false);
  Pre(static_cast<Command&>(command));
}

void PixelFormatCaptureLayer::Post(wglMakeCurrentCommand& command) {
  ResolveAttributeQuery(command.m_Return.Value != 0, command.m_newContext.Value,
                        command.m_hDc.Value);
  Post(static_cast<Command&>(command));
}

void PixelFormatCaptureLayer::Post(wglMakeContextCurrentARBCommand& command) {
  ResolveAttributeQuery(command.m_Return.Value != 0, command.m_hglrc.Value,
                        command.m_hDrawDC.Value);
  Post(static_cast<Command&>(command));
}

void PixelFormatCaptureLayer::Post(wglMakeContextCurrentEXTCommand& command) {
  ResolveAttributeQuery(command.m_Return.Value != 0, command.m_hglrc.Value,
                        command.m_hDrawDC.Value);
  Post(static_cast<Command&>(command));
}

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
// CaptureLayerManager::LoadLayers()) serializes the command onto the wire. This means the stream
// now carries the format the driver ACTUALLY selected - self-consistent by construction (Windows
// itself produced it), so replay-time sanitization no longer needs to guess or force
// PFD_DOUBLEBUFFER at all; it only needs to add PFD_DRAW_TO_WINDOW/PFD_SUPPORT_OPENGL, which
// this player's own single always-visible window genuinely requires regardless of capture (see
// EnsurePixelFormatSet's own updated comment).
void PixelFormatCaptureLayer::Post(wglSetPixelFormatCommand& command) {
#ifdef _WIN32
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
#endif
  Post(static_cast<Command&>(command));
}

} // namespace gits::opengl2
