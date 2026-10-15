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
// Pre(wglMakeCurrentCommand&) below runs, `command.m_Key` is therefore already reserved and
// smaller than whatever CreateCommandKey() would hand out next - so a naive
// CaptureManager::CreateCommandKey() call from inside CaptureIfChanged() would always give the
// geometry command a LARGER key, not a smaller one.
//
// This matters on replay two ways. First: HdcVirtualizationService::Resolve() (which creates
// the player's window, today at a hardcoded 640x480) is only ever triggered by a
// wglCreateContext(ARB)/wglMakeCurrent runner - the very first WindowGeometryMetaCommand for a
// given window must reach WindowGeometryMetaRunner (player/include/windowGeometryMetaRunner.h)
// before Resolve() ever runs for it, so Resolve() can create the window at the REAL size
// immediately, with no growing. Second: a resize/move/visibility change sampled for the SAME
// present it is hooked on (wglSwapBuffers/wglSwapLayerBuffers) must reach
// WindowGeometryMetaRunner no later than that present - otherwise the change applies one frame
// late. Both cases need the exact same fix: every Pre() override below pushes the HOOKED
// command's key later, via CaptureManager::UpdateCommandKey() - mirroring DirectX's identical
// idiom (see that method's own header comment) - whenever CaptureIfChanged() actually recorded
// a geometry command. That leaves the geometry command at its original (smaller) key and the
// hooked WGL command at a fresh, strictly later one.
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
  // four typed overloads, none of which accept a plain Command&.
  using Layer::Post;
  using Layer::Pre;

  // First-capture hook points - mirrors the legacy OpenGL backend's own trigger (see
  // OpenGL/common/wglFunctions.cpp's CwglSetPixelFormat/CwglChoosePixelFormat constructors,
  // and WindowGeometryMetaCommand's own header comment (windowGeometryMetaTypes.h) for
  // wglMakeCurrent as an equally valid, often-earlier trigger for an app that sets its pixel
  // format on one HDC/thread but only later makes it current).
  void Pre(wglSetPixelFormatCommand& command) override;
  void Pre(wglMakeCurrentCommand& command) override;

  // Per-frame poll hook points - mirrors Vulkan's WindowTrackingService::UpdateWindowsForPresent
  // (called on every vkQueuePresentKHR); catches a real resize/move/visibility change that
  // happens after the initial capture above, without needing a WM_SIZE-level hook (OpenGL2 has
  // no window-message plumbing of its own to hook - see this layer's own investigation, and
  // WindowGeometryTrackingService::CaptureIfChanged's own no-op-when-unchanged behavior, which
  // keeps this cheap even though it runs every frame). Also push the key later on an actual
  // change, exactly like the first-capture hooks above - see this class's own header comment.
  void Pre(wglSwapBuffersCommand& command) override;
  void Pre(wglSwapLayerBuffersCommand& command) override;
};

} // namespace gits::opengl2
