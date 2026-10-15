// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// A small, hand-written Pre-side Layer (see layerAuto.h/layerBase.h and
// CaptureManager::GetPreLayers() - this is the first concrete Pre layer this backend has; only
// EncoderLayer, a Post layer, existed before) that drives WindowGeometryTrackingService
// (windowGeometryTrackingService.h) from the WGL calls whose HDC gives access to the
// application's real window - see that service's own header comment for the full rationale
// and the exact hook points mirrored (legacy OpenGL's wglSetPixelFormat-time capture, Vulkan's
// per-present poll).
//
// Runs in Pre(), before the real driver dispatch AND before EncoderLayer's own Post-side
// serialization of the WGL call this hook is attached to. Running in Pre() alone is NOT
// enough to give the resulting WindowGeometryMetaCommand a lower ordering key than the hooked
// WGL command's own, though: every generated wrapper (recorderWrappersAuto.cpp.mako) reserves
// and assigns its command's ordering key BEFORE invoking any Pre layer, precisely so
// concurrent top-level calls on different threads get keys in dispatch order. By the time
// Pre(wglMakeCurrentCommand&) below runs, `command.m_Key` is therefore already reserved - so a
// naive CaptureManager::CreateCommandKey() call from inside WindowGeometryTrackingService's own
// capture method would always give the geometry command a LARGER key than `command`'s own,
// not a smaller one; and a naive
// "skip the old key, mint a fresh one, later" fix for THAT (this class's own git history) is
// itself racy against a concurrent thread's own key reservation - see
// CaptureManager::CreateCommandKeyPair()'s own header comment (captureManager.h) for exactly
// why.
//
// This matters on replay two ways. First: HdcVirtualizationService::Resolve() (which creates
// the player's window, today at a hardcoded 640x480) is only ever triggered by a
// wglCreateContext(ARB)/wglMakeCurrent runner - the very first WindowGeometryMetaCommand for a
// given window must reach WindowGeometryMetaRunner (player/include/windowGeometryMetaRunner.h)
// before Resolve() ever runs for it, so Resolve() can create the window at the REAL size
// immediately, with no growing. Second: a resize/move/visibility change sampled for the SAME
// present it is hooked on (wglSwapBuffers/wglSwapLayerBuffers) must reach
// WindowGeometryMetaRunner no later than that present - otherwise the change applies one frame
// late. Both cases need the exact same fix, and all commands below get it the exact same
// way: generator_recorder.py's PRE_HOOK_KEY_PAIR_COMMANDS lists all of them, so each one's own
// Wrapper_<cmd> (recorderWrappersAuto.cpp.mako) reserves its own ordering key TOGETHER with a
// second, spare key - via CaptureManager::CreateCommandKeyPair(), one atomic operation, before
// either key is ever exposed to another thread - and hands the spare key to this layer via
// CaptureManager::SetPendingPreHookKey(). Each Pre() override below consumes it via
// TakePendingPreHookKey() and passes it straight into
// WindowGeometryTrackingService::Capture(), which takes ownership of it from there:
// a WindowGeometryMetaCommand always gets recorded under it (guaranteed smaller than the hooked
// command's own key, by construction of the pair), with window 0 when the window could not be
// queried - see that method's own header comment (windowGeometryTrackingService.h).
#include "commandsAuto.h"
#include "layerAuto.h"

namespace gits::opengl2 {

class WindowGeometryTrackingLayer : public Layer {
public:
  WindowGeometryTrackingLayer() : Layer("WindowGeometryTrackingLayer") {}

  // Bring Layer's generic Pre(Command&)/Post(Command&) (and every OTHER typed overload this
  // class does not itself override) back into this class's own overload set - mirrors
  // layerAuto.h.mako's generated `Layer` class doing the exact same
  // `using LayerBase::Pre;`/`using LayerBase::Post;` for the identical reason (see that
  // header's own comment): declaring even one same-named Pre/Post below hides ALL of Layer's
  // Pre/Post overloads per C++'s member name-hiding-across-scopes rule, not just the ones this
  // class doesn't redeclare. Without this, this class's own Pre(Command&) calls below (each
  // typed override's required fallback into the generic hook, matching every generated Pre/
  // Post body's own shape) fail to compile - overload resolution only sees this class's own
  // typed overloads, none of which accept a plain Command&.
  using Layer::Post;
  using Layer::Pre;

  // First-capture hook points - mirrors the legacy OpenGL backend's own trigger (see
  // OpenGL/common/wglFunctions.cpp's CwglSetPixelFormat/CwglChoosePixelFormat constructors,
  // and WindowGeometryMetaCommand's own header comment (windowGeometryMetaTypes.h) for
  // wglMakeCurrent as an equally valid, often-earlier trigger for an app that sets its pixel
  // format on one HDC/thread but only later makes it current).
  void Pre(wglSetPixelFormatCommand& command) override;
  void Pre(wglMakeCurrentCommand& command) override;
  // One sample per DC, so the player binds the draw and the read DC before resolving either.
  void Pre(wglMakeContextCurrentARBCommand& command) override;
  void Pre(wglMakeContextCurrentEXTCommand& command) override;
  // Context creation, so the player creates the context on the window its HDC belongs to even
  // when the application got a fresh DC for that window after setting its pixel format.
  void Pre(wglCreateContextCommand& command) override;
  void Pre(wglCreateContextAttribsARBCommand& command) override;

  // Per-frame poll hook points - mirrors Vulkan's WindowTrackingService::UpdateWindowsForPresent
  // (called on every vkQueuePresentKHR); catches a real resize/move/visibility change that
  // happens after the initial capture above, without needing a WM_SIZE-level hook (OpenGL2 has
  // no window-message plumbing of its own to hook - see this layer's own investigation).
  // Unconditionally records a fresh sample every frame - see
  // WindowGeometryTrackingService::Capture's own header comment for why an unchanged-sample
  // skip is not safe here. Gets the exact same pre-reserved pair-of-keys treatment as the
  // first-capture hooks above - see this class's own header comment.
  void Pre(wglSwapBuffersCommand& command) override;
  void Pre(wglSwapLayerBuffersCommand& command) override;
};

} // namespace gits::opengl2
