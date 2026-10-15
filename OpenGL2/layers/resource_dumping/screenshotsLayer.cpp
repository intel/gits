// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "screenshotsLayer.h"

#include "configurator.h"
#include "glEnumsAuto.h"
#include "hdcVirtualizationService.h"
#include "log.h"
#include "openglDriversAuto.h"
#include "threadContextService.h"

#include <stb_image_write.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

// Hand-declared instead of including <windows.h>, which conflicts with glTypes.h.
typedef struct HWND__* HWND;
extern "C" __declspec(dllimport) HWND __stdcall WindowFromDC(HDC hDC);
extern "C" __declspec(dllimport) BOOL __stdcall GetClientRect(HWND hWnd, RECT* lpRect);

namespace gits::opengl2 {

namespace {

template <typename FunctionT>
FunctionT Real(const std::atomic<FunctionT>& function) {
  return function.load(std::memory_order_acquire);
}

GLint GetInteger(GLenum pname) {
  GLint value = 0;
  Real(player::g_DispatchTable.glGetIntegerv)(pname, &value);
  return value;
}

struct PackParameter {
  GLenum Name;
  GLint Default;
  int Major;
  int Minor;
};

constexpr PackParameter PACK_PARAMETERS[] = {
    {GL_PACK_SWAP_BYTES, 0, 1, 0},   {GL_PACK_LSB_FIRST, 0, 1, 0},   {GL_PACK_ROW_LENGTH, 0, 1, 0},
    {GL_PACK_SKIP_ROWS, 0, 1, 0},    {GL_PACK_SKIP_PIXELS, 0, 1, 0}, {GL_PACK_ALIGNMENT, 4, 1, 0},
    {GL_PACK_IMAGE_HEIGHT, 0, 1, 2}, {GL_PACK_SKIP_IMAGES, 0, 1, 2},
};

// Reads the color buffer a swap presents from the current context's default framebuffer, and
// leaves the state it changes as it was. Only state the version has is queried, so no GL error is
// raised.
std::vector<uint8_t> ReadDefaultFramebuffer(GLsizei width, GLsizei height) {
  auto& table = player::g_DispatchTable;
  int major = 0;
  int minor = 0;
  if (const auto* version = reinterpret_cast<const char*>(Real(table.glGetString)(GL_VERSION))) {
    std::sscanf(version, "%d.%d", &major, &minor);
  }
  const auto atLeast = [&](int requiredMajor, int requiredMinor) {
    return major > requiredMajor || (major == requiredMajor && minor >= requiredMinor);
  };

  const GLint readFramebuffer = atLeast(3, 0) ? GetInteger(GL_READ_FRAMEBUFFER_BINDING) : 0;
  if (readFramebuffer != 0) {
    Real(table.glBindFramebuffer)(GL_READ_FRAMEBUFFER, 0);
  }
  const GLint packBuffer = atLeast(2, 1) ? GetInteger(GL_PIXEL_PACK_BUFFER_BINDING) : 0;
  if (packBuffer != 0) {
    Real(table.glBindBuffer)(GL_PIXEL_PACK_BUFFER, 0);
  }
  GLint packValues[std::size(PACK_PARAMETERS)]{};
  for (size_t i = 0; i < std::size(PACK_PARAMETERS); ++i) {
    const PackParameter& parameter = PACK_PARAMETERS[i];
    packValues[i] = parameter.Default;
    if (atLeast(parameter.Major, parameter.Minor)) {
      packValues[i] = GetInteger(parameter.Name);
      if (packValues[i] != parameter.Default) {
        Real(table.glPixelStorei)(parameter.Name, parameter.Default);
      }
    }
  }
  const GLint readBuffer = GetInteger(GL_READ_BUFFER);
  GLboolean doubleBuffered = GL_FALSE;
  Real(table.glGetBooleanv)(GL_DOUBLEBUFFER, &doubleBuffered);
  const GLenum presented = doubleBuffered ? GL_BACK : GL_FRONT;
  if (static_cast<GLenum>(readBuffer) != presented) {
    Real(table.glReadBuffer)(presented);
  }

  std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
  Real(table.glReadPixels)(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  if (static_cast<GLenum>(readBuffer) != presented) {
    Real(table.glReadBuffer)(static_cast<GLenum>(readBuffer));
  }
  for (size_t i = 0; i < std::size(PACK_PARAMETERS); ++i) {
    if (packValues[i] != PACK_PARAMETERS[i].Default) {
      Real(table.glPixelStorei)(PACK_PARAMETERS[i].Name, packValues[i]);
    }
  }
  if (packBuffer != 0) {
    Real(table.glBindBuffer)(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(packBuffer));
  }
  if (readFramebuffer != 0) {
    Real(table.glBindFramebuffer)(GL_READ_FRAMEBUFFER, static_cast<GLuint>(readFramebuffer));
  }
  return pixels;
}

// GL rows start at the bottom; image rows at the top.
void WriteImage(const std::string& path, int width, int height, std::vector<uint8_t>& pixels) {
  const size_t stride = static_cast<size_t>(width) * 4;
  std::vector<uint8_t> row(stride);
  for (int y = 0; y < height / 2; ++y) {
    uint8_t* top = pixels.data() + y * stride;
    uint8_t* bottom = pixels.data() + (height - 1 - y) * stride;
    std::memcpy(row.data(), top, stride);
    std::memcpy(top, bottom, stride);
    std::memcpy(bottom, row.data(), stride);
  }
  for (size_t i = 3; i < pixels.size(); i += 4) {
    pixels[i] = 0xFF;
  }

  int saved = 0;
  std::string fileName = path;
  if (Configurator::Get().common.shared.screenshots.format == ImageFormat::JPEG) {
    fileName += ".jpg";
    constexpr int jpegQuality = 95;
    saved = stbi_write_jpg(fileName.c_str(), width, height, 4, pixels.data(), jpegQuality);
  } else {
    fileName += ".png";
    saved =
        stbi_write_png(fileName.c_str(), width, height, 4, pixels.data(), static_cast<int>(stride));
  }
  if (!saved) {
    LOG_ERROR << "ScreenshotsLayer: failed to write " << fileName;
  }
}

} // namespace

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
    std::vector<uint8_t> pixels = ReadDefaultFramebuffer(rect.right, rect.bottom);
    std::ostringstream name;
    name << "frame" << std::setw(8) << std::setfill('0') << m_Frame;
    WriteImage((m_DumpPath / name.str()).string(), rect.right, rect.bottom, pixels);
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
