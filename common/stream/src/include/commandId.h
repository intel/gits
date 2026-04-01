// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits {
namespace stream {

enum class ApiId {
  ID_COMMON = 0,
  ID_OPENGL = 1,
  ID_GL_HELPER_TOKENS = 2,
  ID_WGL = 3,
  ID_GLX = 5,
  ID_EGL = 6,
  ID_OPENCL = 7,
  ID_VULKAN = 10,
  ID_LEVELZERO = 11,
  ID_OCLOC = 12,
  ID_DIRECTX = 13,
};

inline ApiId ExtractApiIdentifier(unsigned commandId) {
  return static_cast<stream::ApiId>(commandId / 0x10000);
}

enum class CommonCommandId {
  ID_INIT_START = 1,
  ID_INIT_END = 2,
  ID_FRAME_END = 4,
  ID_MARKER_UINT64 = 12,
};

} // namespace stream
} // namespace gits
