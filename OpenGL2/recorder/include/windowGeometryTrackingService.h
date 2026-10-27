// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>
#include <initializer_list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

struct _XDisplay;
typedef struct _XDisplay Display;

namespace gits::opengl2 {

// Captures the application's real Win32 window geometry (see WindowGeometryMetaCommand's own
// header comment, windowGeometryMetaTypes.h, for the full cross-backend rationale) and records a
// WindowGeometryMetaCommand for every successful capture. Owned/invoked by
// WindowGeometryTrackingLayer (windowGeometryTrackingLayer.h) from Pre() hooks on
// wglSetPixelFormat/wglMakeCurrent (first capture, mirroring the legacy OpenGL backend's own
// wglSetPixelFormat-time capture - see OpenGL/common/wglFunctions.cpp's CwglSetPixelFormat
// constructor) and wglSwapBuffers/wglSwapLayerBuffers (per-frame poll, mirroring Vulkan's
// WindowTrackingService::UpdateWindowsForPresent - Vulkan/recorder/windowTrackingService.h).
//
// Deliberately simpler than Vulkan's own WindowTrackingService: this backend has exactly one
// real application window to ever track (there is no VkSurfaceKHR/VkSwapchainKHR-shaped
// indirection - an HDC's owning HWND, resolved via WindowFromDC, is the only window identity
// OpenGL2 ever has), so there is no per-surface map - just one "capture and record" entry point
// used from every hook point. Samples themselves are never deduplicated: see Capture()'s own
// header comment below for why an earlier, stateful skip-if-unchanged version of this method
// was removed rather than fixed a third time. The only state kept is the set of sampled windows,
// to sample all of them at every capture and to report which were destroyed since the previous
// one (m_DestroyedWindows).
class WindowGeometryTrackingService {
public:
  static WindowGeometryTrackingService& Get();

  // `hdcs` are the real, live, capture-time HDCs of one WGL call (never recorded/replayed values -
  // this class is recorder-only): its draw and read DCs for wglMakeContextCurrentARB/EXT, its one
  // DC otherwise. Resolves each owning HWND via WindowFromDC and queries real Win32 geometry via
  // GetClientRect/GetWindowRect/IsWindowVisible, then unconditionally records one
  // WindowGeometryMetaCommand with a sample per HDC, followed by a sample of every other sampled
  // window - see below for why this no longer tries to dedup identical, unchanged samples the way
  // Vulkan's RecordUpdateIfChanged does.
  //
  // Sampled windows found destroyed are reported in a second WindowGeometryMetaCommand, under a
  // key reserved once their destruction was seen: `geometryKey` was reserved earlier, possibly
  // before other threads recorded their last uses of those windows.
  //
  // HISTORY - why there is no cache/skip-if-unchanged logic here despite two prior attempts at
  // one: an earlier version recorded a WindowGeometryMetaCommand only when the sample differed
  // from the last one actually recorded, to avoid spamming the stream with an identical record
  // on every single frame of a perfectly static window (a per-frame poll, wglSwapBuffers,
  // reaches this method every frame). That cache was keyed on `geometryKey` (see this class's
  // own git history) specifically so a call already holding a SMALLER, still-pending
  // `geometryKey` could never be wrongly suppressed just because a LARGER-keyed call happened
  // to reach the (contended, cross-thread) mutex first and cache its own sample - a real,
  // fixed bug in the very first version. But comparing only against "the largest sample
  // actually PROCESSED so far" is still unsafe in the other direction: a call is reached here
  // with `geometryKey` already reserved (CaptureManager::CreateCommandKeyPair()) well before
  // it ever competes for the mutex, so a SMALLER, not-yet-processed key can still be sitting
  // out there, about to record a DIFFERENT value, at the moment a LARGER key's matching sample
  // gets skipped as "unchanged." E.g.: key 1 records geometry A; key 3 then samples A (still
  // genuinely true at key 3's own real-time sampling moment) and is skipped as a duplicate of
  // the key-1 cache; only afterwards does a delayed key 2 finally run its own (real, correct)
  // sample and record geometry B. The stream now reads A at key 1, B at key 2, and nothing at
  // key 3 - but key 3's own hooked WGL call needed A to still be in effect at that point, not
  // B. Fixing this properly would mean deferring every skip/record decision until every
  // smaller, still-outstanding key has itself been resolved (an explicit in-flight-key
  // tracker, not merely a "highest key processed" cache) - real, but substantially more
  // machinery than this single-window service otherwise needs. Recording every successful
  // sample unconditionally, as done here now, sidesteps the whole class of bug at the cost of
  // the skip-when-unchanged optimization: a static window's per-frame poll now re-records
  // identical geometry every frame, which OrderingRecorder/the encoder can decode trivially -
  // a larger but always-correct stream beats a smaller, sometimes-wrong one.
  //
  // Records a sample with window 0 and no geometry (never throws, never logs more than once) if
  // an HDC's owning window cannot be resolved or queried - see .cpp for the specific, once-only
  // warning this emits. The player still needs it: it decides each call from the samples taken
  // for that call, and must not reuse an earlier call's decision. The player's
  // glViewport-inferred fallback (player/include/replayCustomizationLayer.h) still applies on
  // replay for a stream where this capture never succeeded.
  //
  // `geometryKey` must already be reserved by the caller - see
  // CaptureManager::CreateCommandKeyPair()'s own header comment (captureManager.h) for why
  // this key is minted as part of an atomic PAIR at the caller's own wrapper-entry point,
  // together with the hooked WGL command's own key, rather than freshly here: every caller
  // (WindowGeometryTrackingLayer's Pre() overrides, via
  // CaptureManager::TakePendingPreHookKey()) already has one ready by the time it calls this
  // method. This method takes ownership of `geometryKey` unconditionally from here on - on
  // EVERY return path, `geometryKey` is consumed by a WindowGeometryMetaCommand this method
  // records - never silently dropped (see CaptureManager::SkipCommandKey's own header comment
  // for why a dropped key would stall OrderingRecorder forever).
  void Capture(uint32_t threadId, std::initializer_list<void*> hdcs, uint64_t geometryKey);

  // Records a WindowGeometryMetaCommand under `geometryKey` with a sample of each distinct
  // non-zero drawable of `drawables` (GLXDrawables on the application's `display`: the draw
  // drawable first, then the read one), identified by its GlxDrawableIdentityService identity. A
  // GLX window is sampled as the X window it was created for. A sample has window 0 when its
  // drawable is not an X window, e.g. a GLX pixmap or pbuffer. Releases `geometryKey` when every
  // drawable is 0, as a context detach has no drawable to sample.
  //
  // Sampled X windows of `display`'s server found destroyed are reported as Capture() reports
  // them.
  void CaptureGlx(uint32_t threadId,
                  Display* display,
                  std::initializer_list<uint64_t> drawables,
                  uint64_t geometryKey);
  // Tracks the X window `window` a GLX window was created for, until it is destroyed.
  void AddGlxWindow(Display* display, uint64_t glxWindow, uint64_t window);
  void RemoveGlxWindow(Display* display, uint64_t glxWindow);

private:
  WindowGeometryTrackingService() = default;

  struct Sample {
    uint64_t Window{};
    int32_t X{};
    int32_t Y{};
    int32_t Width{};
    int32_t Height{};
    bool Visible{};
    // Set for an X window, whose destruction is reported.
    bool XWindow{};
  };
  struct OtherWindows {
    std::vector<Sample> Samples;
    std::vector<uint64_t> Destroyed;
  };

  // Adds `window` (an HWND) to the sampled windows and returns its identity, or 0 if it is
  // destroyed. A window's identity is its HWND in the low 32 bits (Win32 handles are 32-bit
  // values) and its lifetime in the high 32 bits: the first sample of a window tags it with a new
  // lifetime through a window property, so a window that reuses a destroyed window's HWND gets a
  // new identity, and the destroyed one is reported destroyed.
  uint64_t Track(void* window);

  // The sample of `hdc`'s window, with window 0 when it cannot be identified and queried.
  Sample Query(void* hdc);

  // The sample of `window` (an HWND) under `identity`, or none if its geometry cannot be queried
  // or its lifetime changed meanwhile. Takes no lock.
  std::optional<Sample> Measure(void* window, uint64_t identity);

  // The sample of `drawable`, with window 0 when it is not an X window.
  Sample QueryGlx(Display* display, uint64_t drawable);

  // The sampled X windows of `display`'s server other than `sampled` destroyed since the previous
  // scan, which are forgotten.
  std::vector<uint64_t> ScanGlxWindows(Display* display, const std::vector<uint64_t>& sampled);

  void RecordDestroyed(uint32_t threadId, std::vector<uint64_t> destroyed);

  // Samples of the sampled windows other than `sampled`, and those destroyed since the previous
  // scan, which are forgotten. Captures reach the mutex out of key order, so a sample replayed out
  // of real-time order is corrected by the next capture. Destruction is final, and is reported
  // once.
  OtherWindows ScanOtherWindows(const std::vector<uint64_t>& sampled);

  std::mutex m_WindowsMutex;
  // Capture-time HWND or XID of every sampled window not yet reported destroyed, by identity.
  std::unordered_map<uint64_t, uint64_t> m_Windows;
  uint32_t m_LastLifetime{0};
  std::mutex m_GlxWindowsMutex;
  // The X window of every live GLX window, by the GLX window's identity.
  std::unordered_map<uint64_t, uint64_t> m_GlxWindows;
};

} // namespace gits::opengl2
