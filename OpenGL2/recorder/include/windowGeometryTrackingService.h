// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_set>
#include <vector>

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
// to report which of them are destroyed or hidden at every sample (m_ClosedWindows).
class WindowGeometryTrackingService {
public:
  static WindowGeometryTrackingService& Get();

  // `hdc` is a real, live, capture-time HDC (never a recorded/replayed value - this class is
  // recorder-only). Resolves its owning HWND via WindowFromDC and queries real Win32 geometry
  // via GetClientRect/GetWindowRect/IsWindowVisible, then unconditionally records a
  // WindowGeometryMetaCommand for every successful query - see below for why this no longer
  // tries to dedup identical, unchanged samples the way Vulkan's RecordUpdateIfChanged does.
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
  // `hdc`'s owning window cannot be resolved or queried - see .cpp for the specific, once-only
  // warning this emits. The player still needs it: it decides each swap from the sample taken
  // for that swap, and must not reuse an earlier window's decision. GlViewportRunner's own
  // glViewport-inferred fallback (player/include/commandRunnersCustom.h) still applies on replay
  // for a stream where this capture never succeeded.
  //
  // `geometryKey` must already be reserved by the caller - see
  // CaptureManager::CreateCommandKeyPair()'s own header comment (captureManager.h) for why
  // this key is minted as part of an atomic PAIR at the caller's own wrapper-entry point,
  // together with the hooked WGL command's own key, rather than freshly here: every caller
  // (WindowGeometryTrackingLayer's four Pre() overrides, via
  // CaptureManager::TakePendingPreHookKey()) already has one ready by the time it calls this
  // method. This method takes ownership of `geometryKey` unconditionally from here on - on
  // EVERY return path, `geometryKey` is consumed by a WindowGeometryMetaCommand this method
  // records - never silently dropped (see CaptureManager::SkipCommandKey's own header comment
  // for why a dropped key would stall OrderingRecorder forever).
  void Capture(uint32_t threadId, void* hdc, uint64_t geometryKey);

private:
  WindowGeometryTrackingService() = default;

  // Every window sampled so far, other than `sampled` (0: none), that is destroyed or hidden now;
  // then adds `sampled` to them. Captures reach the mutex out of key order, so nothing here is a
  // one-time transition: each sample reports the full current set, and a report replayed out of
  // real-time order is corrected by the next sample.
  std::vector<uint64_t> ClosedWindows(uint64_t sampled);

  std::mutex m_WindowsMutex;
  // Capture-time HWND of every sampled window. Never pruned: a capture stalled with a smaller key
  // could otherwise re-show a destroyed window on replay with no later sample reporting it.
  std::unordered_set<uint64_t> m_Windows;
};

} // namespace gits::opengl2
