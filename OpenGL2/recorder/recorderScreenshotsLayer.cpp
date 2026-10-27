// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "recorderScreenshotsLayer.h"

#include "captureManager.h"
#include "configurator.h"
#include "defaultFramebufferImage.h"
#include "log.h"
#include "openglDriversAuto.h"
#include "systemApi.h"

#include <optional>
#include <utility>
#include <vector>

namespace gits::opengl2 {

namespace {

using screenshot_detail::Real;

enum class MakeCurrentEntryPoint {
  Core,
  ARB,
  EXT
};

// How the thread made its context current, which tells how to query and restore its read DC.
thread_local MakeCurrentEntryPoint t_EntryPoint = MakeCurrentEntryPoint::Core;

HDC CurrentReadDc(HDC drawDc) {
  switch (t_EntryPoint) {
  case MakeCurrentEntryPoint::ARB:
    return Real(g_DispatchTable.wglGetCurrentReadDCARB)();
  case MakeCurrentEntryPoint::EXT:
    return Real(g_DispatchTable.wglGetCurrentReadDCEXT)();
  default:
    return drawDc;
  }
}

bool MakeCurrent(HDC drawDc, HDC readDc, HGLRC context) {
  switch (t_EntryPoint) {
  case MakeCurrentEntryPoint::ARB:
    return Real(g_DispatchTable.wglMakeContextCurrentARB)(drawDc, readDc, context) != 0;
  case MakeCurrentEntryPoint::EXT:
    return Real(g_DispatchTable.wglMakeContextCurrentEXT)(drawDc, readDc, context) != 0;
  default:
    return Real(g_DispatchTable.wglMakeCurrent)(drawDc, context) != 0;
  }
}

} // namespace

RecorderScreenshotsLayer::RecorderScreenshotsLayer()
    : Layer("Screenshots"), m_Range(Configurator::Get().common.shared.screenshots.frames) {
  const auto& config = Configurator::Get().common;
  m_DumpPath = config.recorder.dumpPath / "gitsScreenshots/gitsRecorder";
  std::filesystem::create_directories(m_DumpPath);
  if (config.shared.screenshots.format == ImageFormat::DDS) {
    LOG_WARNING << "Screenshots: DDS is not supported, writing PNG";
  }
}

void RecorderScreenshotsLayer::Pre(wglSwapBuffersCommand& command) {
  Read(command.m_Key, command.m_hdc.Value);
}

void RecorderScreenshotsLayer::Post(wglSwapBuffersCommand& command) {
  EndFrame(command.m_Key, command.m_Return.Value != 0);
}

void RecorderScreenshotsLayer::Pre(wglSwapLayerBuffersCommand& command) {
  Read(command.m_Key, command.m_hdc.Value);
}

void RecorderScreenshotsLayer::Post(wglSwapLayerBuffersCommand& command) {
  EndFrame(command.m_Key, command.m_Return.Value != 0);
}

void RecorderScreenshotsLayer::Post(wglMakeCurrentCommand& command) {
  if (command.m_Return.Value != 0) {
    t_EntryPoint = MakeCurrentEntryPoint::Core;
    NoteCurrent();
  }
}

void RecorderScreenshotsLayer::Post(wglMakeContextCurrentARBCommand& command) {
  if (command.m_Return.Value != 0) {
    t_EntryPoint = MakeCurrentEntryPoint::ARB;
    NoteCurrent();
  }
}

void RecorderScreenshotsLayer::Post(wglMakeContextCurrentEXTCommand& command) {
  if (command.m_Return.Value != 0) {
    t_EntryPoint = MakeCurrentEntryPoint::EXT;
    NoteCurrent();
  }
}

void RecorderScreenshotsLayer::Post(wglDeleteContextCommand& command) {
  if (command.m_Return.Value == 0) {
    return;
  }
  const std::lock_guard<std::mutex> lock(m_Mutex);
  std::erase_if(m_DcContexts,
                [&](const auto& entry) { return entry.second == command.m_oldContext.Value; });
}

void RecorderScreenshotsLayer::NoteCurrent() {
  const LastSystemErrorGuard lastErrorGuard;
  if (const HGLRC context = Real(g_DispatchTable.wglGetCurrentContext)()) {
    const HDC dc = Real(g_DispatchTable.wglGetCurrentDC)();
    const std::lock_guard<std::mutex> lock(m_Mutex);
    m_DcContexts[dc] = context;
  }
}

void RecorderScreenshotsLayer::Pre(glXSwapBuffersCommand& command) {
  ReadGlx(command.m_Key, command.m_dpy.Value, command.m_drawable.Value);
}

void RecorderScreenshotsLayer::Post(glXSwapBuffersCommand& command) {
  EndFrame(command.m_Key, true);
}

std::optional<unsigned> RecorderScreenshotsLayer::BeginSwap(uint64_t key) {
  const std::lock_guard<std::mutex> lock(m_Mutex);
  // Swaps before this one still in flight on other threads may fail, so the swap is read if
  // any frame it can become is selected.
  unsigned frame = m_Frame + 1;
  auto inFlight = static_cast<unsigned>(CaptureManager::Get().FrameEndsPendingBefore(key));
  for (auto it = m_Swaps.begin(); it != m_Swaps.end() && it->first < key; ++it) {
    if (!it->second.Ended) {
      ++inFlight;
    } else if (it->second.Succeeded) {
      ++frame;
    }
  }
  m_Swaps.emplace(key, Swap{});
  for (unsigned candidate = frame; candidate <= frame + inFlight; ++candidate) {
    if (m_Range[candidate]) {
      return frame;
    }
  }
  return std::nullopt;
}

void RecorderScreenshotsLayer::Read(uint64_t key, HDC dc) {
  const std::optional<unsigned> selectedFrame = BeginSwap(key);
  if (!selectedFrame) {
    return;
  }
  const unsigned frame = *selectedFrame;
  HGLRC dcContext = nullptr;
  {
    const std::lock_guard<std::mutex> lock(m_Mutex);
    if (const auto it = m_DcContexts.find(dc); it != m_DcContexts.end()) {
      dcContext = it->second;
    }
  }
  const LastSystemErrorGuard lastErrorGuard;
  int32_t width = 0;
  int32_t height = 0;
  if (!GetWindowClientSize(dc, width, height) || width <= 0 || height <= 0) {
    LOG_WARNING << "Screenshots: frame " << frame << " swaps no visible window, skipped";
    return;
  }

  // The swapped DC is read with the context last made current on it, since the current one may
  // be made current on another DC, or read another DC.
  const HGLRC context = Real(g_DispatchTable.wglGetCurrentContext)();
  const HDC drawDc = context != nullptr ? Real(g_DispatchTable.wglGetCurrentDC)() : nullptr;
  const HDC readDc = context != nullptr ? CurrentReadDc(drawDc) : nullptr;
  const bool switchContext = drawDc != dc || readDc != dc;
  const HGLRC readContext = switchContext ? dcContext : context;
  if (readContext == nullptr) {
    LOG_WARNING << "Screenshots: frame " << frame << " has no context, skipped";
    return;
  }
  if (switchContext && Real(g_DispatchTable.wglMakeCurrent)(dc, readContext) == 0) {
    LOG_WARNING << "Screenshots: making a context current for frame " << frame
                << " failed, skipped";
  } else {
    Image image{width, height, ReadDefaultFramebuffer(g_DispatchTable, width, height)};
    const std::lock_guard<std::mutex> lock(m_Mutex);
    m_Swaps[key].Screenshot = std::move(image);
  }
  if (!switchContext) {
    return;
  }
  if (context == nullptr) {
    Real(g_DispatchTable.wglMakeCurrent)(nullptr, nullptr);
  } else {
    MakeCurrent(drawDc, readDc, context);
  }
}

void RecorderScreenshotsLayer::ReadGlx(uint64_t key, Display* display, GLXDrawable drawable) {
  const std::optional<unsigned> frame = BeginSwap(key);
  if (!frame) {
    return;
  }
  const LastSystemErrorGuard lastErrorGuard;
  // TODO: Read a drawable that is not current, as Read() does for a DC, once an application
  // swaps one.
  if (Real(g_DispatchTable.glXGetCurrentDrawable)() != drawable) {
    LOG_WARNING << "Screenshots: frame " << *frame << " swaps a drawable that is not current, "
                << "skipped";
    return;
  }
  unsigned width = 0;
  unsigned height = 0;
  Real(g_DispatchTable.glXQueryDrawable)(display, drawable, GLX_WIDTH, &width);
  Real(g_DispatchTable.glXQueryDrawable)(display, drawable, GLX_HEIGHT, &height);
  if (width == 0 || height == 0) {
    LOG_WARNING << "Screenshots: frame " << *frame << " swaps an empty drawable, skipped";
    return;
  }
  // The swapped drawable is read, not the read drawable of the context.
  const GLXDrawable readDrawable = Real(g_DispatchTable.glXGetCurrentReadDrawable)();
  const GLXContext context = Real(g_DispatchTable.glXGetCurrentContext)();
  const bool switchRead = readDrawable != drawable;
  if (switchRead &&
      Real(g_DispatchTable.glXMakeContextCurrent)(display, drawable, drawable, context) == 0) {
    LOG_WARNING << "Screenshots: reading the drawable of frame " << *frame << " failed, skipped";
    return;
  }
  Image image{static_cast<int>(width), static_cast<int>(height),
              ReadDefaultFramebuffer(g_DispatchTable, static_cast<GLsizei>(width),
                                     static_cast<GLsizei>(height))};
  if (switchRead) {
    Real(g_DispatchTable.glXMakeContextCurrent)(display, drawable, readDrawable, context);
  }
  const std::lock_guard<std::mutex> lock(m_Mutex);
  m_Swaps[key].Screenshot = std::move(image);
}

void RecorderScreenshotsLayer::EndFrame(uint64_t key, bool succeeded) {
  std::vector<std::pair<unsigned, Image>> screenshots;
  {
    const std::lock_guard<std::mutex> lock(m_Mutex);
    Swap& swap = m_Swaps[key];
    swap.Ended = true;
    swap.Succeeded = succeeded;
    // A swap with a lower key whose Pre layers have not run yet is not in m_Swaps, but is
    // numbered first.
    while (!m_Swaps.empty() && m_Swaps.begin()->second.Ended &&
           CaptureManager::Get().FrameEndsPendingBefore(m_Swaps.begin()->first) == 0) {
      auto node = m_Swaps.extract(m_Swaps.begin());
      Swap& ended = node.mapped();
      if (!ended.Succeeded) {
        continue;
      }
      ++m_Frame;
      if (ended.Screenshot && m_Range[m_Frame]) {
        screenshots.emplace_back(m_Frame, std::move(*ended.Screenshot));
      }
    }
  }
  if (screenshots.empty()) {
    return;
  }
  const LastSystemErrorGuard lastErrorGuard;
  for (auto& [frame, image] : screenshots) {
    WriteScreenshot(m_DumpPath, frame, image.Width, image.Height, image.Pixels);
  }
}

} // namespace gits::opengl2
