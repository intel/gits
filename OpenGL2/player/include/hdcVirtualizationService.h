// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits::opengl2 {

// Maps recorded HDC values onto real DCs of player-owned windows, one window per recorded window
// lifetime (the identity a WindowGeometryMetaCommand carries), as the legacy backend and the
// DirectX and Vulkan players do. Each window gets its own pixel format, geometry and visibility,
// so a throwaway window an application uses only to load WGL extensions cannot impose its pixel
// format on the window the application renders to.
//
// The recorder takes a geometry sample right before every wglSetPixelFormat, wglCreateContext,
// wglCreateContextAttribsARB, wglMakeCurrent, wglSwapBuffers and wglSwapLayerBuffers, and one per
// DC before wglMakeContextCurrentARB/EXT, with nothing recorded in between, so their runners bind
// the call's recorded HDCs to the sampled windows (BindSampledWindow(), BindSampledWindows()).
// A DC used before any binding stays on the window it was first
// used on: the most recently bound one or, with no window known at all (a stream recorded
// without geometry samples, or one whose samples could not identify their window), one shared
// fallback window, sized by GrowToFit(). wglSetPixelFormat always rebinds, since applications
// reuse DC values across windows; the other calls move a DC only where its contexts can follow
// (see BindSampledWindow()).
//
// A player window is shown while its recorded window is visible with a positive client area, so
// zero-area helper windows stay hidden. The fallback window starts hidden and is shown once a
// sampled call uses it, so a query issued before the first binding does not show it.
// windowMode, forceWindowSize, forceWindowPos and showWindowsWA apply to every window, as in the
// other players.
//
// A destroyed recorded window's identity is never used again: samples of it replayed after its
// destruction was reported are ignored, and its player window stays hidden. Like the other
// players, player windows live until the end of replay.
//
// Recorded values are never dereferenced or compared against live handles.
class HdcVirtualizationService {
public:
  static HdcVirtualizationService& Get();

  // The real DC of the player window the recorded DC is bound to, creating the window on first
  // use. A recorded pbuffer DC (wglGetPbufferDCARB/EXT) resolves to the real DC of its replayed
  // pbuffer instead - see PbufferService (pbufferService.h) - or to nullptr if replay failed to
  // get that DC, so the calls using it fail as they would on any invalid DC.
  HDC Resolve(HDC recordedValue);

  // Binds a recorded DC to the window of the geometry sample replayed right before the call
  // using it, and consumes that sample. Called by the runners of the sampled calls, before
  // Resolve(); a null DC only consumes. A sample with no window (0), or of a destroyed window,
  // leaves the binding unchanged.
  //
  // Unless `settingPixelFormat`, a DC whose window already has a pixel format moves only to a
  // window that has one too, since its contexts were created for that format. When that window
  // is the fallback and the sampled window has no player window yet, the fallback becomes the
  // sampled window's player window instead, taking its recorded geometry.
  void BindSampledWindow(HDC recordedValue, bool settingPixelFormat = false);

  // BindSampledWindow() for wglMakeContextCurrentARB/EXT, whose draw and read DCs each have a
  // sample, the draw DC's first.
  void BindSampledWindows(HDC drawDc, HDC readDc);

  // The capture-time pixel format attributes a PixelFormatMetaCommand delivers right before its
  // wglSetPixelFormat or wglCreatePbufferARB/EXT, keyed by recorded thread. Take returns an empty
  // list when the thread has none pending.
  void SetPendingPixelFormat(uint32_t threadId, std::vector<int32_t> attributes);
  std::vector<int32_t> TakePendingPixelFormat(uint32_t threadId);

  // Chooses a format on `realDc` with captured {WGL_*_ARB attribute, value} pairs using
  // wglChoosePixelFormatARB, or returns 0 when there are none or none matches.
  static int ChooseCapturedPixelFormat(HDC realDc, const std::vector<int32_t>& captured);

  // Sets a pixel format on the player window the recorded DC is bound to, at most once per window
  // (Win32 allows one SetPixelFormat per window). A failed attempt latches nothing, so a later
  // request can retry.
  //
  // requestedPfd is a replayed wglSetPixelFormat command's captured PIXELFORMATDESCRIPTOR, which
  // the recorder takes from DescribePixelFormat of the format actually set (see
  // PixelFormatCaptureLayer), and `attributes` that format's captured attributes. The format is
  // re-negotiated on the replay driver, as the legacy backend's SelectPixelFormat() does, since
  // the captured format index means nothing in another driver's format table: from the
  // attributes, which include multisampling, or with ChoosePixelFormat from the descriptor when
  // there are none or they match no format.
  //
  // Null means no request is available: wglCreateContext(ARB)'s runners pass it, defensively, for
  // an application that never set a pixel format before creating its context, and a generic
  // RGBA/double-buffered/depth format is chosen. WglSetPixelFormatRunner never passes null: a
  // captured wglSetPixelFormat(hdc, format, nullptr) failed at capture and must not set a format
  // that would block the window's real request.
  //
  // Returns whether the window has a pixel format once this call returns.
  bool EnsurePixelFormatSet(HDC recordedValue,
                            const PIXELFORMATDESCRIPTOR* requestedPfd,
                            const std::vector<int32_t>& attributes = {});

  // Grows the fallback window's client area to at least (width, height), never shrinking it, if
  // `drawDc` (the real current draw DC) is the fallback's. GlViewportRunner calls this for every
  // glViewport(0, 0, width, height): for a stream without geometry samples, an origin-anchored
  // viewport is the closest available signal for the application's window size. Windows
  // identified by samples take their size from those samples only.
  void GrowToFit(HDC drawDc, int width, int height);

  // One sample of a WindowGeometryMetaCommand: a recorded window identity (m_Windows) and its
  // recorded screen position, client size and visibility.
  struct WindowSample {
    uint64_t Window{};
    int X{};
    int Y{};
    int Width{};
    int Height{};
    bool Visible{};
  };

  // Applies a WindowGeometryMetaCommand. Each sample's window is recorded before its player
  // window exists, or applied to it once it does. Like the DirectX and Vulkan players, a window
  // is created with its recorded geometry even when its client area is empty; once it exists, an
  // empty client area (a minimized window, say) hides it instead of resizing it. A sample of
  // window 0, one the recorder could not capture, changes no window, as does a sample of a window
  // already reported destroyed. The first `callSamples` (m_CallSamples), if any, become the
  // samples the next sampled call binds; the rest are of other windows, and only update them.
  // `destroyedWindows` (m_DestroyedWindows) hide their player windows for good, and a DC first
  // used after its window was destroyed no longer inherits that window.
  void SetGeometry(const std::vector<WindowSample>& samples,
                   size_t callSamples,
                   const std::vector<uint64_t>& destroyedWindows);

private:
  HdcVirtualizationService() = default;

  struct PlayerWindow {
    HDC RealHdc{};
    uint64_t NativeDisplay{};
    uint64_t NativeWindow{};
    bool PixelFormatSet{false};
    // Recorded windows take their geometry from a sample, so these only place the fallback window.
    int X{10};
    int Y{10};
    int ClientWidth{640};
    int ClientHeight{480};
    // A sample gave the window a positive client area.
    bool HasGeometry{false};
    bool Shown{false};
    // Window-vs-client size delta measured right after creation: WindowManager::ResizeWindow()
    // sizes the whole window, while CreatePlayerWindow() takes a client size.
    int BorderExtraWidth{0};
    int BorderExtraHeight{0};
  };

  // m_Mutex must be held by the following.
  void Bind(HDC recordedValue, bool pbufferDc, uint64_t window, bool settingPixelFormat);
  PlayerWindow& WindowFor(HDC recordedValue);
  void AdoptFallback(uint64_t window);
  void Create(PlayerWindow& window);
  void MeasureBorder(PlayerWindow& window);
  void ApplyGeometry(PlayerWindow& window, int x, int y, int clientWidth, int clientHeight);
  void Resize(PlayerWindow& window, int clientWidth, int clientHeight);
  void ApplyVisibility(PlayerWindow& window, bool shown);

  std::mutex m_Mutex;
  // Keyed by recorded window identity; 0 is the fallback window.
  std::unordered_map<uint64_t, PlayerWindow> m_Windows;
  std::unordered_map<HDC, uint64_t> m_DcWindows;
  uint64_t m_LastBoundWindow{0};
  // Windows of the samples taken for the next sampled call, one per DC (0: unknown).
  std::vector<uint64_t> m_SampledWindows;
  // Identities reported destroyed.
  std::unordered_set<uint64_t> m_DestroyedWindows;
  std::unordered_map<uint32_t, std::vector<int32_t>> m_PendingPixelFormats;
};

} // namespace gits::opengl2
