// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowGeometryTrackingLayer.h"

#include "captureManager.h"
#include "windowGeometryTrackingService.h"

namespace gits::opengl2 {

// command.m_hdc.Value (an opaque HDC = struct HDC__* per glTypes.h) converts implicitly to
// void* at this call - see windowGeometryTrackingService.h's own header comment for why the
// service's own API takes a plain void* instead of HDC: it needs real <windows.h> internally,
// which cannot coexist in the same translation unit as this file's own commandsAuto.h/
// glTypes.h include.
//
// This command is one of generator_recorder.py's PRE_HOOK_KEY_PAIR_COMMANDS
// (recorderWrappersAuto.cpp.mako), so its wrapper already reserved TWO keys as one atomic
// pair before this Pre() hook ever ran, and handed the spare (smaller) one off via
// CaptureManager::SetPendingPreHookKey() - see that table's own header comment and
// CaptureManager::CreateCommandKeyPair()'s own header comment (captureManager.h) for why this
// is the only race-free way to guarantee the geometry command below decodes/replays BEFORE
// this WGL call. TakePendingPreHookKey() consumes it exactly once per call - Capture()
// unconditionally takes ownership of it from here on (a WindowGeometryMetaCommand is always
// recorded under it, with window 0 when the window could not be queried - see that method's own
// header comment, windowGeometryTrackingService.h).
// Never skip taking it, even though the value itself is unused here beyond being handed off -
// TakePendingPreHookKey()'s own assert is what actually catches a wrapper/layer table mismatch
// (a command reserving a pair with nothing to consume it, or vice versa) - see that method's
// own header comment.
void WindowGeometryTrackingLayer::Pre(wglSetPixelFormatCommand& command) {
  WindowGeometryTrackingService::Get().Capture(command.m_ThreadId, {command.m_hdc.Value},
                                               CaptureManager::Get().TakePendingPreHookKey());
  Pre(static_cast<Command&>(command));
}

void WindowGeometryTrackingLayer::Pre(wglMakeCurrentCommand& command) {
  // A detach call (wglMakeCurrent(NULL, NULL) or wglMakeCurrent(hdc, NULL) after the app is
  // done with a context) still carries a real, capturable HDC in the common case - only a
  // fully-null recorded HDC (the detach's own "no context" form) has nothing to resolve a
  // window from, and Capture() already records an unknown-window sample for a HDC whose
  // WindowFromDC fails, so no extra null check is needed here beyond what that function
  // already does internally.
  //
  // See Pre(wglSetPixelFormatCommand&) above for where this pre-hook key comes from.
  WindowGeometryTrackingService::Get().Capture(command.m_ThreadId, {command.m_hDc.Value},
                                               CaptureManager::Get().TakePendingPreHookKey());
  Pre(static_cast<Command&>(command));
}

void WindowGeometryTrackingLayer::Pre(wglMakeContextCurrentARBCommand& command) {
  // One sample per DC, the draw DC's first, in one command.
  WindowGeometryTrackingService::Get().Capture(command.m_ThreadId,
                                               {command.m_hDrawDC.Value, command.m_hReadDC.Value},
                                               CaptureManager::Get().TakePendingPreHookKey());
  Pre(static_cast<Command&>(command));
}

void WindowGeometryTrackingLayer::Pre(wglMakeContextCurrentEXTCommand& command) {
  WindowGeometryTrackingService::Get().Capture(command.m_ThreadId,
                                               {command.m_hDrawDC.Value, command.m_hReadDC.Value},
                                               CaptureManager::Get().TakePendingPreHookKey());
  Pre(static_cast<Command&>(command));
}

void WindowGeometryTrackingLayer::Pre(wglCreateContextCommand& command) {
  WindowGeometryTrackingService::Get().Capture(command.m_ThreadId, {command.m_hDc.Value},
                                               CaptureManager::Get().TakePendingPreHookKey());
  Pre(static_cast<Command&>(command));
}

void WindowGeometryTrackingLayer::Pre(wglCreateContextAttribsARBCommand& command) {
  WindowGeometryTrackingService::Get().Capture(command.m_ThreadId, {command.m_hDC.Value},
                                               CaptureManager::Get().TakePendingPreHookKey());
  Pre(static_cast<Command&>(command));
}

void WindowGeometryTrackingLayer::Pre(wglSwapBuffersCommand& command) {
  // Per-frame poll, not a "first capture" hook - the context (and therefore the player
  // window/HdcVirtualizationService::Resolve()) must already be current for this call to be
  // valid at all, so there is no default-geometry race to guard against here. This command is
  // still listed in PRE_HOOK_KEY_PAIR_COMMANDS, though (see Pre(wglSetPixelFormatCommand&)
  // above for where this pre-hook key comes from): a resize/move/visibility change sampled for
  // THIS present must decode/replay no later than the swap itself, or it would apply one frame
  // late - the same atomic-pair guarantee that protects the two first-capture hooks above
  // applies here for that reason.
  WindowGeometryTrackingService::Get().Capture(command.m_ThreadId, {command.m_hdc.Value},
                                               CaptureManager::Get().TakePendingPreHookKey());
  Pre(static_cast<Command&>(command));
}

void WindowGeometryTrackingLayer::Pre(wglSwapLayerBuffersCommand& command) {
  // See Pre(wglSwapBuffersCommand&) above - same per-frame poll, same pre-hook key contract.
  WindowGeometryTrackingService::Get().Capture(command.m_ThreadId, {command.m_hdc.Value},
                                               CaptureManager::Get().TakePendingPreHookKey());
  Pre(static_cast<Command&>(command));
}

void WindowGeometryTrackingLayer::Pre(glXMakeCurrentCommand& command) {
  WindowGeometryTrackingService::Get().CaptureGlx(command.m_ThreadId, command.m_dpy.Value,
                                                  command.m_drawable.Value,
                                                  CaptureManager::Get().TakePendingPreHookKey());
  Pre(static_cast<Command&>(command));
}

void WindowGeometryTrackingLayer::Pre(glXSwapBuffersCommand& command) {
  // See Pre(wglSwapBuffersCommand&) above.
  WindowGeometryTrackingService::Get().CaptureGlx(command.m_ThreadId, command.m_dpy.Value,
                                                  command.m_drawable.Value,
                                                  CaptureManager::Get().TakePendingPreHookKey());
  Pre(static_cast<Command&>(command));
}

} // namespace gits::opengl2
