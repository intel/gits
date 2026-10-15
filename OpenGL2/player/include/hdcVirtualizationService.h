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

private:
  HdcVirtualizationService() = default;

  std::once_flag m_Init;
  std::mutex m_PixelFormatMutex;
  std::atomic<bool> m_PixelFormatSet{false};
  HDC m_RealHdc{};
};

} // namespace gits::opengl2
