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
// CaptureIfChanged()'s bool return says whether it actually recorded a WindowGeometryMetaCommand
// (using a key minted right now, before `command`'s own key is touched). When it did, push
// `command`'s key later via CaptureManager::UpdateCommandKey() so the geometry command it just
// recorded is guaranteed to decode/replay BEFORE this WGL call - never skip this when
// CaptureIfChanged returned true, or OrderingRecorder would replay the WGL call (and any
// HdcVirtualizationService::Resolve() it triggers, for these two "first capture" hooks; or a
// visibility/position/size change that was sampled for this very present, for
// wglSwapBuffers/wglSwapLayerBuffers below) using the stale prior geometry instead.
void WindowGeometryTrackingLayer::Pre(wglSetPixelFormatCommand& command) {
  if (WindowGeometryTrackingService::Get().CaptureIfChanged(command.m_ThreadId,
                                                            command.m_hdc.Value)) {
    CaptureManager::Get().UpdateCommandKey(command);
  }
  Pre(static_cast<Command&>(command));
}

void WindowGeometryTrackingLayer::Pre(wglMakeCurrentCommand& command) {
  // A detach call (wglMakeCurrent(NULL, NULL) or wglMakeCurrent(hdc, NULL) after the app is
  // done with a context) still carries a real, capturable HDC in the common case - only a
  // fully-null recorded HDC (the detach's own "no context" form) has nothing to resolve a
  // window from, and CaptureIfChanged already no-ops safely on a HDC whose WindowFromDC fails,
  // so no extra null check is needed here beyond what that function already does internally.
  //
  // See Pre(wglSetPixelFormatCommand&) above for why the key is pushed later exactly when
  // CaptureIfChanged returns true.
  if (WindowGeometryTrackingService::Get().CaptureIfChanged(command.m_ThreadId,
                                                            command.m_hDc.Value)) {
    CaptureManager::Get().UpdateCommandKey(command);
  }
  Pre(static_cast<Command&>(command));
}

void WindowGeometryTrackingLayer::Pre(wglSwapBuffersCommand& command) {
  // Per-frame poll, not a "first capture" hook - the context (and therefore the player
  // window/HdcVirtualizationService::Resolve()) must already be current for this call to be
  // valid at all, so there is no default-geometry race to guard against here. The key still
  // needs pushing later on an actual change, though (see Pre(wglSetPixelFormatCommand&)
  // above's general comment): the wrapper reserves this swap call's own key before invoking
  // Pre() (recorderWrappersAuto.cpp.mako), so a resize/move/visibility change sampled for THIS
  // present would otherwise replay with a LARGER key than the swap - one frame late - unless
  // pushed later here exactly like the first-capture hooks.
  if (WindowGeometryTrackingService::Get().CaptureIfChanged(command.m_ThreadId,
                                                            command.m_hdc.Value)) {
    CaptureManager::Get().UpdateCommandKey(command);
  }
  Pre(static_cast<Command&>(command));
}

void WindowGeometryTrackingLayer::Pre(wglSwapLayerBuffersCommand& command) {
  // See Pre(wglSwapBuffersCommand&) above - same per-frame poll, same reason to push the key
  // later on an actual change.
  if (WindowGeometryTrackingService::Get().CaptureIfChanged(command.m_ThreadId,
                                                            command.m_hdc.Value)) {
    CaptureManager::Get().UpdateCommandKey(command);
  }
  Pre(static_cast<Command&>(command));
}

} // namespace gits::opengl2
