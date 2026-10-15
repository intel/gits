// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Screenshots of the default framebuffer, shared by the recorder and the player, which call the
// driver through dispatch tables of their own.
#include "glEnumsAuto.h"
#include "glTypes.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iterator>
#include <string_view>
#include <vector>

namespace gits::opengl2 {

namespace screenshot_detail {

struct PackParameter {
  GLenum Name;
  GLint Default;
  int Major;
  int Minor;
};

inline constexpr PackParameter PACK_PARAMETERS[] = {
    {GL_PACK_SWAP_BYTES, 0, 1, 0},   {GL_PACK_LSB_FIRST, 0, 1, 0},   {GL_PACK_ROW_LENGTH, 0, 1, 0},
    {GL_PACK_SKIP_ROWS, 0, 1, 0},    {GL_PACK_SKIP_PIXELS, 0, 1, 0}, {GL_PACK_ALIGNMENT, 4, 1, 0},
    {GL_PACK_IMAGE_HEIGHT, 0, 1, 2}, {GL_PACK_SKIP_IMAGES, 0, 1, 2},
};

template <typename FunctionT>
FunctionT Real(const std::atomic<FunctionT>& function) {
  return function.load(std::memory_order_acquire);
}

} // namespace screenshot_detail

// Reads the color buffer a swap presents from the current context's default framebuffer, and
// leaves the state it changes as it was. Only state the version or its extensions have is
// queried, so no GL error is raised.
template <typename DispatchTableT>
std::vector<uint8_t> ReadDefaultFramebuffer(DispatchTableT& table, GLsizei width, GLsizei height) {
  using screenshot_detail::PACK_PARAMETERS;
  using screenshot_detail::Real;
  const auto getInteger = [&](GLenum pname) {
    GLint value = 0;
    Real(table.glGetIntegerv)(pname, &value);
    return value;
  };
  int major = 0;
  int minor = 0;
  if (const auto* version = reinterpret_cast<const char*>(Real(table.glGetString)(GL_VERSION))) {
    std::sscanf(version, "%d.%d", &major, &minor);
  }
  const auto atLeast = [&](int requiredMajor, int requiredMinor) {
    return major > requiredMajor || (major == requiredMajor && minor >= requiredMinor);
  };

  // Before 3.0, framebuffer objects come from GL_ARB_framebuffer_object or
  // GL_EXT_framebuffer_object, whose one binding is for drawing and reading unless
  // GL_EXT_framebuffer_blit separates them. The EXT enums have the core values.
  const char* extensions =
      atLeast(3, 0) ? nullptr
                    : reinterpret_cast<const char*>(Real(table.glGetString)(GL_EXTENSIONS));
  const auto hasExtension = [&](std::string_view name) {
    for (std::string_view rest = extensions != nullptr ? extensions : ""; !rest.empty();) {
      const size_t end = rest.find(' ');
      if (rest.substr(0, end) == name) {
        return true;
      }
      rest = end == std::string_view::npos ? std::string_view{} : rest.substr(end + 1);
    }
    return false;
  };
  const bool coreFramebuffers = atLeast(3, 0) || hasExtension("GL_ARB_framebuffer_object");
  const bool extFramebuffers = !coreFramebuffers && hasExtension("GL_EXT_framebuffer_object");
  const bool separateRead = coreFramebuffers || hasExtension("GL_EXT_framebuffer_blit");
  const auto bindFramebuffer = [&](GLenum target, GLuint framebuffer) {
    if (coreFramebuffers) {
      Real(table.glBindFramebuffer)(target, framebuffer);
    } else {
      Real(table.glBindFramebufferEXT)(target, framebuffer);
    }
  };
  // The draw binding is reset too: with a framebuffer object bound for drawing, reading the
  // window can return its previous frame.
  const bool framebuffers = coreFramebuffers || extFramebuffers;
  const GLint drawFramebuffer = framebuffers ? getInteger(GL_DRAW_FRAMEBUFFER_BINDING) : 0;
  const GLint readFramebuffer =
      framebuffers && separateRead ? getInteger(GL_READ_FRAMEBUFFER_BINDING) : drawFramebuffer;
  const bool framebufferBound = drawFramebuffer != 0 || readFramebuffer != 0;
  if (framebufferBound) {
    bindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  // Before 2.1, pixel pack buffers come from GL_ARB_pixel_buffer_object or
  // GL_EXT_pixel_buffer_object, with the core enums, bound through GL_ARB_vertex_buffer_object
  // before 1.5.
  const bool packBuffers = atLeast(2, 1) || hasExtension("GL_ARB_pixel_buffer_object") ||
                           hasExtension("GL_EXT_pixel_buffer_object");
  const auto bindPackBuffer = [&](GLuint buffer) {
    if (atLeast(1, 5)) {
      Real(table.glBindBuffer)(GL_PIXEL_PACK_BUFFER, buffer);
    } else {
      Real(table.glBindBufferARB)(GL_PIXEL_PACK_BUFFER, buffer);
    }
  };
  const GLint packBuffer = packBuffers ? getInteger(GL_PIXEL_PACK_BUFFER_BINDING) : 0;
  if (packBuffer != 0) {
    bindPackBuffer(0);
  }
  GLint packValues[std::size(PACK_PARAMETERS)]{};
  for (size_t i = 0; i < std::size(PACK_PARAMETERS); ++i) {
    const auto& parameter = PACK_PARAMETERS[i];
    packValues[i] = parameter.Default;
    if (atLeast(parameter.Major, parameter.Minor)) {
      packValues[i] = getInteger(parameter.Name);
      if (packValues[i] != parameter.Default) {
        Real(table.glPixelStorei)(parameter.Name, parameter.Default);
      }
    }
  }
  const GLint readBuffer = getInteger(GL_READ_BUFFER);
  GLboolean doubleBuffered = GL_FALSE;
  Real(table.glGetBooleanv)(GL_DOUBLEBUFFER, &doubleBuffered);
  const GLenum presented = doubleBuffered != GL_FALSE ? GL_BACK : GL_FRONT;
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
    bindPackBuffer(static_cast<GLuint>(packBuffer));
  }
  if (framebufferBound && separateRead) {
    bindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(readFramebuffer));
    bindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(drawFramebuffer));
  } else if (framebufferBound) {
    bindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(drawFramebuffer));
  }
  return pixels;
}

// Writes `pixels` read by ReadDefaultFramebuffer as frame `frame` in `directory`, in the
// configured image format.
void WriteScreenshot(const std::filesystem::path& directory,
                     unsigned frame,
                     int width,
                     int height,
                     std::vector<uint8_t>& pixels);

} // namespace gits::opengl2
