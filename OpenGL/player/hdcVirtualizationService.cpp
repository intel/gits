// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "hdcVirtualizationService.h"

#include "configurator.h"
#include "glEnumsAuto.h"
#include "openglDriversAuto.h"
#include "pbufferService.h"
#include "windowManager.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>

#ifdef _WIN32
// Hand-declared instead of including <windows.h>, which would redeclare RECT/PROC/etc.
// against glTypes.h's own self-contained declarations of those same names - see glTypes.h's
// header comment for why this backend avoids <windows.h> in any translation unit that also needs
// glTypes.h. PIXELFORMATDESCRIPTOR itself is already declared by glTypes.h (WGL needs it too).
typedef struct HWND__* HWND;
extern "C" __declspec(dllimport) HDC __stdcall GetDC(HWND hWnd);
extern "C" __declspec(dllimport) int __stdcall ChoosePixelFormat(HDC hdc,
                                                                 const PIXELFORMATDESCRIPTOR* ppfd);
extern "C" __declspec(dllimport) BOOL
    __stdcall SetPixelFormat(HDC hdc, int format, const PIXELFORMATDESCRIPTOR* ppfd);
extern "C" __declspec(dllimport) int __stdcall GetPixelFormat(HDC hdc);
extern "C" __declspec(dllimport) BOOL __stdcall GetWindowRect(HWND hWnd, RECT* lpRect);
extern "C" __declspec(dllimport) BOOL __stdcall GetClientRect(HWND hWnd, RECT* lpRect);
#endif

namespace gits::opengl {

namespace {
const auto& PlayerConfig() {
  return Configurator::Get().common.player;
}
} // namespace

HdcVirtualizationService& HdcVirtualizationService::Get() {
  static HdcVirtualizationService instance;
  return instance;
}

HDC HdcVirtualizationService::Resolve(HDC recordedValue) {
  if (const std::optional<HDC> pbufferDc = PbufferService::Get().FindDc(recordedValue)) {
    return *pbufferDc;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  return WindowFor(recordedValue).RealHdc;
}

void HdcVirtualizationService::BindSampledWindow(HDC recordedValue, bool settingPixelFormat) {
  const bool pbufferDc = PbufferService::Get().FindDc(recordedValue).has_value();
  std::lock_guard<std::mutex> lock(m_Mutex);
  const std::vector<uint64_t> samples = std::exchange(m_SampledWindows, {});
  Bind(recordedValue, pbufferDc, samples.empty() ? 0 : samples.front(), settingPixelFormat);
}

void HdcVirtualizationService::BindSampledWindows(HDC drawDc, HDC readDc) {
  const bool pbufferDrawDc = PbufferService::Get().FindDc(drawDc).has_value();
  const bool pbufferReadDc = PbufferService::Get().FindDc(readDc).has_value();
  std::lock_guard<std::mutex> lock(m_Mutex);
  const std::vector<uint64_t> samples = std::exchange(m_SampledWindows, {});
  const bool sampled = samples.size() == 2;
  Bind(drawDc, pbufferDrawDc, sampled ? samples[0] : 0, false);
  Bind(readDc, pbufferReadDc, sampled ? samples[1] : 0, false);
}

void HdcVirtualizationService::Bind(HDC recordedValue,
                                    bool pbufferDc,
                                    uint64_t window,
                                    bool settingPixelFormat) {
  if (recordedValue == nullptr || pbufferDc) {
    return;
  }
  if (m_DestroyedWindows.contains(window)) {
    window = 0;
  }
  const auto bound = m_DcWindows.find(recordedValue);
  const uint64_t current = bound != m_DcWindows.end() ? bound->second : m_LastBoundWindow;
  if (window == 0) {
    if (current == 0) {
      ApplyVisibility(m_Windows[0], true);
    }
    return;
  }
  if (window != current && !settingPixelFormat) {
    const auto currentWindow = m_Windows.find(current);
    const auto target = m_Windows.find(window);
    const bool currentHasFormat =
        currentWindow != m_Windows.end() && currentWindow->second.PixelFormatSet;
    const bool targetHasFormat = target != m_Windows.end() && target->second.PixelFormatSet;
    if (currentHasFormat && !targetHasFormat) {
      if (current != 0 || (target != m_Windows.end() && target->second.NativeWindow != 0)) {
        return;
      }
      AdoptFallback(window);
    }
  }
  m_DcWindows[recordedValue] = window;
  m_LastBoundWindow = window;
}

void HdcVirtualizationService::AdoptFallback(uint64_t window) {
  auto fallback = m_Windows.extract(0);
  PlayerWindow& target = m_Windows[window];
  const PlayerWindow sampled = target;
  target = fallback.mapped();
  target.HasGeometry = sampled.HasGeometry;
  for (auto& [dc, boundWindow] : m_DcWindows) {
    if (boundWindow == 0) {
      boundWindow = window;
    }
  }
  if (sampled.HasGeometry) {
    ApplyGeometry(target, sampled.X, sampled.Y, sampled.ClientWidth, sampled.ClientHeight);
  }
  ApplyVisibility(target, sampled.Shown);
}

HdcVirtualizationService::PlayerWindow& HdcVirtualizationService::WindowFor(HDC recordedValue) {
  uint64_t key = m_LastBoundWindow;
  if (recordedValue != nullptr) {
    key = m_DcWindows.try_emplace(recordedValue, m_LastBoundWindow).first->second;
  }
  PlayerWindow& window = m_Windows[key];
  if (window.NativeWindow == 0) {
    Create(window);
  }
  return window;
}

void HdcVirtualizationService::Create(PlayerWindow& window) {
#ifndef _WIN32
  // TODO: Create X11 player windows once GLX replay is supported.
  throw std::runtime_error(
      "HdcVirtualizationService: replay of WGL windows is not supported on this platform");
#else
  const auto& config = PlayerConfig();
  const bool shown = window.Shown || config.showWindowsWA;
  const int x =
      config.forceWindowPos.enabled ? static_cast<int>(config.forceWindowPos.x) : window.X;
  const int y =
      config.forceWindowPos.enabled ? static_cast<int>(config.forceWindowPos.y) : window.Y;
  const int width = config.forceWindowSize.enabled ? static_cast<int>(config.forceWindowSize.width)
                                                   : window.ClientWidth;
  const int height = config.forceWindowSize.enabled
                         ? static_cast<int>(config.forceWindowSize.height)
                         : window.ClientHeight;
  auto handles = windowing::WindowManager::Get().CreatePlayerWindow(windowing::WindowProtocol::Win,
                                                                    x, y, width, height, shown);
  HWND hwnd = reinterpret_cast<HWND>(handles.second);
  // CreatePlayerWindow() already logs its own error and returns {0, 0} on failure. GetDC(NULL)
  // would return a DC for the entire screen and silently redirect rendering onto the desktop.
  if (hwnd == nullptr) {
    throw std::runtime_error(
        "HdcVirtualizationService: player window creation failed - cannot derive a real "
        "HDC for replay (see the preceding LOG_ERROR from WindowManager::CreatePlayerWindow "
        "for the underlying cause)");
  }
  window.RealHdc = GetDC(hwnd);
  if (window.RealHdc == nullptr) {
    throw std::runtime_error(
        "HdcVirtualizationService: GetDC() failed on the player window - cannot derive a "
        "real HDC for replay");
  }
  window.NativeDisplay = handles.first;
  window.NativeWindow = handles.second;
  MeasureBorder(window);
#endif
}

void HdcVirtualizationService::MeasureBorder(PlayerWindow& window) {
#ifdef _WIN32
  const HWND hwnd = reinterpret_cast<HWND>(window.NativeWindow);
  RECT windowRect{};
  RECT clientRect{};
  if (GetWindowRect(hwnd, &windowRect) && GetClientRect(hwnd, &clientRect)) {
    window.BorderExtraWidth =
        (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
    window.BorderExtraHeight =
        (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);
  }
#else
  (void)window;
#endif
}

void HdcVirtualizationService::Resize(PlayerWindow& window, int clientWidth, int clientHeight) {
  window.ClientWidth = clientWidth;
  window.ClientHeight = clientHeight;
  if (window.NativeWindow == 0 || PlayerConfig().forceWindowSize.enabled) {
    return; // Create() uses the new size, unless forceWindowSize overrides it.
  }
  windowing::WindowManager::Get().ResizeWindow(
      windowing::WindowProtocol::Win, window.NativeDisplay, window.NativeWindow,
      static_cast<uint32_t>(clientWidth + window.BorderExtraWidth),
      static_cast<uint32_t>(clientHeight + window.BorderExtraHeight));
}

void HdcVirtualizationService::GrowToFit(HDC drawDc, int width, int height) {
  if (drawDc == nullptr || width <= 0 || height <= 0) {
    return; // No current DC, or an unusable size, e.g. a minimized window's glViewport(0, 0, 0, 0).
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto fallback = m_Windows.find(0);
  if (fallback == m_Windows.end() || fallback->second.NativeWindow == 0 ||
      fallback->second.RealHdc != drawDc) {
    return;
  }
  PlayerWindow& window = fallback->second;
  if (width <= window.ClientWidth && height <= window.ClientHeight) {
    return;
  }
  Resize(window, (std::max)(window.ClientWidth, width), (std::max)(window.ClientHeight, height));
}

void HdcVirtualizationService::SetGeometry(const std::vector<WindowSample>& samples,
                                           size_t callSamples,
                                           const std::vector<uint64_t>& destroyedWindows) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (const uint64_t destroyed : destroyedWindows) {
    if (destroyed == 0 || !m_DestroyedWindows.insert(destroyed).second) {
      continue;
    }
    if (m_LastBoundWindow == destroyed) {
      m_LastBoundWindow = 0;
    }
    const auto it = m_Windows.find(destroyed);
    if (it != m_Windows.end()) {
      ApplyVisibility(it->second, false);
    }
  }
  callSamples = (std::min)(callSamples, samples.size());
  if (callSamples != 0) {
    m_SampledWindows.clear();
    for (size_t i = 0; i < callSamples; ++i) {
      const uint64_t window = samples[i].Window;
      m_SampledWindows.push_back(m_DestroyedWindows.contains(window) ? 0 : window);
    }
  }
  for (const WindowSample& sample : samples) {
    if (sample.Window == 0 || m_DestroyedWindows.contains(sample.Window)) {
      continue;
    }
    PlayerWindow& target = m_Windows[sample.Window];
    const bool hasArea = sample.Width > 0 && sample.Height > 0;
    if (hasArea || target.NativeWindow == 0) {
      target.HasGeometry = hasArea;
      ApplyGeometry(target, sample.X, sample.Y, (std::max)(sample.Width, 0),
                    (std::max)(sample.Height, 0));
    }
    ApplyVisibility(target, sample.Visible && hasArea);
  }
}

void HdcVirtualizationService::ApplyGeometry(
    PlayerWindow& window, int x, int y, int clientWidth, int clientHeight) {
  if (x != window.X || y != window.Y) {
    window.X = x;
    window.Y = y;
    if (window.NativeWindow != 0 && !PlayerConfig().forceWindowPos.enabled) {
      windowing::WindowManager::Get().MoveWindow(windowing::WindowProtocol::Win,
                                                 window.NativeDisplay, window.NativeWindow, x, y);
    }
  }
  if (clientWidth != window.ClientWidth || clientHeight != window.ClientHeight) {
    Resize(window, clientWidth, clientHeight);
  }
}

void HdcVirtualizationService::ApplyVisibility(PlayerWindow& window, bool shown) {
  if (shown == window.Shown) {
    return;
  }
  window.Shown = shown;
  if (window.NativeWindow != 0 && !PlayerConfig().showWindowsWA) {
    windowing::WindowManager::Get().SetWindowVisibility(
        windowing::WindowProtocol::Win, window.NativeDisplay, window.NativeWindow, shown);
  }
}

void HdcVirtualizationService::SetPendingPixelFormat(uint32_t threadId,
                                                     std::vector<int32_t> attributes) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_PendingPixelFormats[threadId] = std::move(attributes);
}

std::vector<int32_t> HdcVirtualizationService::TakePendingPixelFormat(uint32_t threadId) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_PendingPixelFormats.find(threadId);
  if (it == m_PendingPixelFormats.end()) {
    return {};
  }
  std::vector<int32_t> attributes = std::move(it->second);
  m_PendingPixelFormats.erase(it);
  return attributes;
}

// Zero-valued attributes are left out: wglChoosePixelFormatARB matches booleans exactly, so
// requiring e.g. WGL_BIND_TO_TEXTURE_RGB_ARB to be FALSE would reject formats that merely support
// more. WGL_DOUBLE_BUFFER_ARB, WGL_STEREO_ARB and WGL_SAMPLE_BUFFERS_ARB are kept either way: the
// first two decide the buffers of the default framebuffer, the last whether the format is
// multisampled. An undefined swap method is left out too, as it is matched exactly and promises
// nothing about the back buffer.
int HdcVirtualizationService::ChooseCapturedPixelFormat(HDC realDc,
                                                        const std::vector<int32_t>& captured) {
  std::vector<int> attributes;
  for (size_t i = 0; i + 1 < captured.size(); i += 2) {
    if (captured[i] == WGL_SWAP_METHOD_ARB && captured[i + 1] == WGL_SWAP_UNDEFINED_ARB) {
      continue;
    }
    if (captured[i + 1] != 0 || captured[i] == WGL_DOUBLE_BUFFER_ARB ||
        captured[i] == WGL_STEREO_ARB || captured[i] == WGL_SAMPLE_BUFFERS_ARB) {
      attributes.push_back(captured[i]);
      attributes.push_back(captured[i + 1]);
    }
  }
  if (attributes.empty()) {
    return 0;
  }
  attributes.push_back(0);
  int format = 0;
  UINT count = 0;
  auto choose = player::g_DispatchTable.wglChoosePixelFormatARB.load(std::memory_order_acquire);
  if (choose(realDc, attributes.data(), nullptr, 1, &format, &count) && count > 0) {
    return format;
  }
  return 0;
}

bool HdcVirtualizationService::EnsurePixelFormatSet(HDC recordedValue,
                                                    const PIXELFORMATDESCRIPTOR* requestedPfd,
                                                    const std::vector<int32_t>& attributes) {
  if (PbufferService::Get().FindDc(recordedValue)) {
    return true; // A pbuffer's pixel format is chosen when it is created.
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  PlayerWindow& window = WindowFor(recordedValue);
  if (window.PixelFormatSet) {
    return true;
  }
  // PFD_* flag values and PFD_TYPE_RGBA are from <wingdi.h>, hand-inlined for the same reason the
  // rest of this file avoids <windows.h>.
  constexpr DWORD kPfdDoubleBuffer = 0x00000001;
  constexpr DWORD kPfdDrawToWindow = 0x00000004;
  constexpr DWORD kPfdSupportOpenGL = 0x00000020;
  constexpr BYTE kPfdTypeRgba = 0;
  constexpr BYTE kPfdTypeColorIndex = 1;

  PIXELFORMATDESCRIPTOR pfd{};
  if (requestedPfd != nullptr) {
    pfd = *requestedPfd;
  } else {
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = kPfdDrawToWindow | kPfdSupportOpenGL | kPfdDoubleBuffer;
    pfd.iPixelType = kPfdTypeRgba;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
  }
  // The captured descriptor keeps its buffering semantics (single- or double-buffered, as the
  // driver resolved them at capture); only drawing to a window with OpenGL is required here, since
  // a recorded format may have been set on a bitmap or similar.
  pfd.dwFlags |= kPfdDrawToWindow | kPfdSupportOpenGL;
  if (pfd.iPixelType != kPfdTypeRgba && pfd.iPixelType != kPfdTypeColorIndex) {
    pfd.iPixelType = kPfdTypeRgba;
  }
#ifdef _WIN32
  int format = ChooseCapturedPixelFormat(window.RealHdc, attributes);
  if (format == 0) {
    format = ChoosePixelFormat(window.RealHdc, &pfd);
  }
  if (format != 0 && SetPixelFormat(window.RealHdc, format, &pfd)) {
    window.PixelFormatSet = true;
  }
  return GetPixelFormat(window.RealHdc) != 0;
#else
  return false;
#endif
}

} // namespace gits::opengl
