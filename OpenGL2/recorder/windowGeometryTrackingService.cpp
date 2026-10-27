// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowGeometryTrackingService.h"

#include "captureManager.h"
#include "log.h"
#include "systemApi.h"
#include "windowGeometryMetaTypes.h"

#ifdef _WIN32
// Real <windows.h> is safe in this translation unit (unlike hdcVirtualizationService.cpp/
// mapTrackingWrappers.cpp, which hand-declare instead): this file only includes
// captureManager.h (deliberately glTypes.h-free, see that header's own comment) and
// windowGeometryMetaTypes.h (a hand-written meta-command with no gl.xml-derived fields, so no
// glTypes.h dependency either) - never commandsAuto.h/openglDriversAuto.h, which is what
// actually pulls in the conflicting glTypes.h declarations elsewhere in this backend. Callers
// (windowGeometryTrackingLayer.cpp, which DOES need commandsAuto.h for the typed Pre() hooks)
// pass their glTypes.h HDC values across this boundary as a plain `void*`, which is where the
// avoidance actually happens.
#include <windows.h>
#endif

#include <algorithm>
#include <utility>

namespace gits::opengl2 {

namespace {
#ifdef _WIN32
std::mutex g_WarnOnceMutex;
bool g_WarnedWindowFromDcFailure = false;
bool g_WarnedGeometryQueryFailure = false;

LPCSTR LifetimeProperty() {
  static const ATOM atom = GlobalAddAtomA("GITS_OpenGL2_WindowLifetime");
  return MAKEINTATOM(atom);
}

uint32_t Lifetime(HWND hwnd) {
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetPropA(hwnd, LifetimeProperty())));
}

uint64_t Identity(HWND hwnd, uint32_t lifetime) {
  return (static_cast<uint64_t>(lifetime) << 32) |
         static_cast<uint32_t>(reinterpret_cast<uintptr_t>(hwnd));
}
#endif

void SetPresent(WindowGeometryMetaCommand& command) {
  command.m_Windows.HasValue = true;
  command.m_X.HasValue = true;
  command.m_Y.HasValue = true;
  command.m_Width.HasValue = true;
  command.m_Height.HasValue = true;
  command.m_Visible.HasValue = true;
  command.m_DestroyedWindows.HasValue = true;
}
} // namespace

WindowGeometryTrackingService& WindowGeometryTrackingService::Get() {
  static WindowGeometryTrackingService instance;
  return instance;
}

#ifdef _WIN32
uint64_t WindowGeometryTrackingService::Track(void* window) {
  const HWND hwnd = static_cast<HWND>(window);
  std::lock_guard<std::mutex> lock(m_WindowsMutex);
  uint32_t lifetime = Lifetime(hwnd);
  if (lifetime == 0) {
    if (SetPropA(hwnd, LifetimeProperty(),
                 reinterpret_cast<HANDLE>(static_cast<uintptr_t>(m_LastLifetime + 1))) == 0) {
      return 0; // Destroyed before it could be tagged.
    }
    lifetime = ++m_LastLifetime;
  }
  const uint64_t identity = Identity(hwnd, lifetime);
  m_Windows.try_emplace(identity, reinterpret_cast<uint64_t>(hwnd));
  return identity;
}

WindowGeometryTrackingService::Sample WindowGeometryTrackingService::Query(void* hdc) {
  const HWND hwnd = WindowFromDC(static_cast<HDC>(hdc));
  if (hwnd == nullptr) {
    // Common and expected for an HDC that isn't a real, on-screen window DC (e.g. a
    // memory/bitmap DC, or an already-destroyed window) - not every wglMakeCurrent/
    // wglSwapBuffers call is guaranteed to have a resolvable HWND. Warn once, not per-call,
    // mirroring this backend's other warn-once services (e.g. ObjectNameMapService).
    std::lock_guard<std::mutex> lock(g_WarnOnceMutex);
    if (!g_WarnedWindowFromDcFailure) {
      g_WarnedWindowFromDcFailure = true;
      LOG_WARNING << "WindowGeometryTrackingService: WindowFromDC returned NULL for at least "
                     "one captured HDC - real window geometry could not be captured for it; "
                     "the player's glViewport-inferred fallback will apply on replay instead";
    }
    return {};
  }

  // The lifetime is taken before the geometry and checked again after it, so geometry queried
  // while the window was destroyed, or its HWND reused, is never recorded under either identity.
  const uint64_t identity = Track(hwnd);
  if (identity == 0) {
    return {};
  }
  return Measure(hwnd, identity).value_or(Sample{});
}

std::optional<WindowGeometryTrackingService::Sample> WindowGeometryTrackingService::Measure(
    void* window, uint64_t identity) {
  const HWND hwnd = static_cast<HWND>(window);
  RECT clientRect{};
  RECT windowRect{};
  if (!GetClientRect(hwnd, &clientRect) || !GetWindowRect(hwnd, &windowRect)) {
    std::lock_guard<std::mutex> lock(g_WarnOnceMutex);
    if (!g_WarnedGeometryQueryFailure) {
      g_WarnedGeometryQueryFailure = true;
      LOG_WARNING << "WindowGeometryTrackingService: GetClientRect/GetWindowRect failed for "
                     "at least one resolved HWND - real window geometry could not be captured "
                     "for it; the player's glViewport-inferred fallback will apply on replay "
                     "instead";
    }
    return std::nullopt;
  }
  // IsWindowVisible's own contract only promises a nonzero return on success, never
  // specifically TRUE (1) - comparing with `== TRUE` would silently record a visible window as
  // hidden if it ever returned some other nonzero value, so normalize with a nonzero check
  // instead (`!= 0`, not `== TRUE`).
  const bool visible = IsWindowVisible(hwnd) != 0;
  if (Identity(hwnd, Lifetime(hwnd)) != identity) {
    return std::nullopt;
  }

  // Matches window_handle::get_dimensions's own math (OpenGL/common/windowing.cpp) - the
  // legacy backend's equivalent capture: position is the window rect's origin offset by the
  // client rect's own (usually zero) origin, size is the client rect's extent (excludes the
  // title bar/border, matching what the player's client-area-sized window creation actually
  // wants - see HdcVirtualizationService's own m_ClientWidth/m_ClientHeight, which are
  // explicitly a client size, not a window size).
  Sample sample;
  sample.Window = identity;
  sample.X = windowRect.left + clientRect.left;
  sample.Y = windowRect.top + clientRect.top;
  sample.Width = clientRect.right - clientRect.left;
  sample.Height = clientRect.bottom - clientRect.top;
  sample.Visible = visible;
  return sample;
}

WindowGeometryTrackingService::OtherWindows WindowGeometryTrackingService::ScanOtherWindows(
    const std::vector<uint64_t>& sampled) {
  OtherWindows others;
  std::lock_guard<std::mutex> lock(m_WindowsMutex);
  for (auto it = m_Windows.begin(); it != m_Windows.end();) {
    const uint64_t identity = it->first;
    const HWND hwnd = reinterpret_cast<HWND>(it->second);
    if (std::find(sampled.begin(), sampled.end(), identity) != sampled.end()) {
      ++it;
      continue;
    }
    if (IsWindow(hwnd) == 0 || Identity(hwnd, Lifetime(hwnd)) != identity) {
      others.Destroyed.push_back(identity);
      it = m_Windows.erase(it);
      continue;
    }
    if (const std::optional<Sample> sample = Measure(hwnd, identity)) {
      others.Samples.push_back(*sample);
    }
    ++it;
  }
  return others;
}
#else
// TODO: Track GLX drawables once GLX commands are captured.
uint64_t WindowGeometryTrackingService::Track(void*) {
  return 0;
}

WindowGeometryTrackingService::Sample WindowGeometryTrackingService::Query(void*) {
  return {};
}

std::optional<WindowGeometryTrackingService::Sample> WindowGeometryTrackingService::Measure(
    void*, uint64_t) {
  return std::nullopt;
}

WindowGeometryTrackingService::OtherWindows WindowGeometryTrackingService::ScanOtherWindows(
    const std::vector<uint64_t>&) {
  return {};
}
#endif

void WindowGeometryTrackingService::Capture(uint32_t threadId,
                                            std::initializer_list<void*> hdcs,
                                            uint64_t geometryKey) {
  const LastSystemErrorGuard lastErrorGuard;
  WindowGeometryMetaCommand command(threadId);
  command.m_Key = geometryKey;
  SetPresent(command);
  // Unconditional - no cache, no skip-if-unchanged - see this method's own header comment
  // (windowGeometryTrackingService.h) for why a stateful dedup here is unsafe against a
  // smaller, not-yet-processed geometryKey still in flight on another thread. A DC whose window
  // is unknown is still sampled, as window 0: the player decides each call from the samples
  // taken for it, and must not reuse an earlier call's.
  const auto add = [&command](const Sample& sample) {
    command.m_Windows.Data.push_back(sample.Window);
    command.m_X.Data.push_back(sample.X);
    command.m_Y.Data.push_back(sample.Y);
    command.m_Width.Data.push_back(sample.Width);
    command.m_Height.Data.push_back(sample.Height);
    command.m_Visible.Data.push_back(sample.Visible ? 1 : 0);
  };
  for (void* hdc : hdcs) {
    add(Query(hdc));
  }
  command.m_CallSamples.Value = static_cast<uint32_t>(hdcs.size());
  OtherWindows others = ScanOtherWindows(command.m_Windows.Data);
  for (const Sample& sample : others.Samples) {
    add(sample);
  }
  auto& captureManager = CaptureManager::Get();
  captureManager.RecordCommand(command, new WindowGeometryMetaSerializer(command));

  if (!others.Destroyed.empty()) {
    // geometryKey may predate uses of these windows recorded by other threads; a key reserved now,
    // after their destruction was seen, follows every such use.
    WindowGeometryMetaCommand destroyed(threadId);
    destroyed.m_Key = captureManager.CreateCommandKey();
    SetPresent(destroyed);
    destroyed.m_DestroyedWindows.Data = std::move(others.Destroyed);
    captureManager.RecordCommand(destroyed, new WindowGeometryMetaSerializer(destroyed));
  }
}

} // namespace gits::opengl2
