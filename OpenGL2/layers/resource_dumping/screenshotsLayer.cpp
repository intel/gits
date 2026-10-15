// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "screenshotsLayer.h"

#include "configurator.h"
#include "defaultFramebufferImage.h"
#include "hdcVirtualizationService.h"
#include "log.h"
#include "openglDriversAuto.h"
#include "threadContextService.h"

#include <vector>

// Hand-declared instead of including <windows.h>, which conflicts with glTypes.h.
typedef struct HWND__* HWND;
extern "C" __declspec(dllimport) HWND __stdcall WindowFromDC(HDC hDC);
extern "C" __declspec(dllimport) BOOL __stdcall GetClientRect(HWND hWnd, RECT* lpRect);

namespace gits::opengl2 {

using screenshot_detail::Real;

ScreenshotsLayer::ScreenshotsLayer()
    : Layer("Screenshots"), m_Range(Configurator::Get().common.shared.screenshots.frames) {
  const auto& config = Configurator::Get().common;
  if (!config.player.captureFrames.empty()) {
    m_Range = config.player.captureFrames;
  }
  m_DumpPath = config.player.outputDir;
  if (m_DumpPath.empty()) {
    m_DumpPath = config.player.streamDir / "gitsScreenshots/gitsPlayer";
  }
  std::filesystem::create_directories(m_DumpPath);
  if (config.shared.screenshots.format == ImageFormat::DDS) {
    LOG_WARNING << "ScreenshotsLayer: DDS is not supported, writing PNG";
  }
}

// The runner overwrites m_Return with the replay result, so the captured one is read in Pre.
void ScreenshotsLayer::Pre(wglSwapBuffersCommand& command) {
  Swap(command.m_hdc.Value, command.m_Return.Value != 0);
}

void ScreenshotsLayer::Pre(wglSwapLayerBuffersCommand& command) {
  Swap(command.m_hdc.Value, command.m_Return.Value != 0);
}

void ScreenshotsLayer::Post(wglMakeCurrentCommand& /*command*/) {
  NoteCurrent();
}

void ScreenshotsLayer::Post(wglMakeContextCurrentARBCommand& /*command*/) {
  NoteCurrent();
}

void ScreenshotsLayer::Post(wglMakeContextCurrentEXTCommand& /*command*/) {
  NoteCurrent();
}

void ScreenshotsLayer::Post(wglDeleteContextCommand& command) {
  if (!command.m_Return.Value) {
    return;
  }
  std::erase_if(m_DcContexts,
                [&](const auto& entry) { return entry.second == command.m_oldContext.Value; });
}

void ScreenshotsLayer::NoteCurrent() {
  auto& table = player::g_DispatchTable;
  if (const HGLRC context = Real(table.wglGetCurrentContext)()) {
    m_DcContexts[Real(table.wglGetCurrentDC)()] = context;
  }
}

void ScreenshotsLayer::Swap(HDC recordedDc, bool endsFrame) {
  if (!endsFrame) {
    return;
  }
  ++m_Frame;
  if (m_Range[m_Frame]) {
    Dump(HdcVirtualizationService::Get().Resolve(recordedDc));
  }
}

void ScreenshotsLayer::Dump(HDC dc) {
  RECT rect{};
  const HWND window = WindowFromDC(dc);
  if (window == nullptr || !GetClientRect(window, &rect) || rect.right <= 0 || rect.bottom <= 0) {
    LOG_WARNING << "ScreenshotsLayer: frame " << m_Frame << " swaps no visible window, skipped";
    return;
  }

  // The swapped DC is read with the context last made current on it, since the current one may
  // be made current on another DC, or read another DC.
  auto& table = player::g_DispatchTable;
  const auto entryPoint = ThreadContextService::Get().CurrentEntryPoint();
  const HGLRC context = Real(table.wglGetCurrentContext)();
  const HDC drawDc = context != nullptr ? Real(table.wglGetCurrentDC)() : nullptr;
  const HDC readDc =
      context != nullptr ? ThreadContextService::CurrentReadDc(entryPoint, drawDc) : nullptr;
  HGLRC dumpContext = context;
  if (drawDc != dc) {
    const auto it = m_DcContexts.find(dc);
    if (it != m_DcContexts.end()) {
      dumpContext = it->second;
    }
  }
  if (dumpContext == nullptr) {
    LOG_WARNING << "ScreenshotsLayer: frame " << m_Frame << " has no context, skipped";
    return;
  }
  const bool switchContext = drawDc != dc || readDc != dc;
  if (switchContext && !Real(table.wglMakeCurrent)(dc, dumpContext)) {
    LOG_WARNING << "ScreenshotsLayer: making a context current for frame " << m_Frame
                << " failed, skipped";
  } else {
    std::vector<uint8_t> pixels = ReadDefaultFramebuffer(table, rect.right, rect.bottom);
    WriteScreenshot(m_DumpPath, m_Frame, rect.right, rect.bottom, pixels);
  }
  if (!switchContext) {
    return;
  }
  if (context == nullptr) {
    Real(table.wglMakeCurrent)(nullptr, nullptr);
  } else {
    ThreadContextService::MakeCurrent(entryPoint, drawDc, readDc, context);
  }
}

} // namespace gits::opengl2
