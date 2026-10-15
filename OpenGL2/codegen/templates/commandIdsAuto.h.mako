// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// One enum section per API registry (GL/WGL/EGL/GLX), each with its own contiguous,
// append-only ID range - see codegen/command_ids.py's _API_RANGE_BEGIN for why these
// ranges are sized individually instead of using one uniform gap per API (GL's registry is
// over 20x larger than WGL/EGL/GLX's).

#pragma once

#include "commandId.h"

namespace gits::opengl2 {

enum class CommandId {
  ID_INIT_START = static_cast<int>(stream::CommonCommandId::ID_INIT_START),
  ID_INIT_END = static_cast<int>(stream::CommonCommandId::ID_INIT_END),
  ID_FRAME_END = static_cast<int>(stream::CommonCommandId::ID_FRAME_END),
  ID_MARKER_UINT64 = static_cast<int>(stream::CommonCommandId::ID_MARKER_UINT64),

  // gits::stream::ApiId::ID_OPENGL2 * 0x10000 = 0xF0000
  ID_META_BEGIN = 0xF0000,
  ID_META_UNSUPPORTED_COMMAND = 0xF0001,
  // See docs/development/opengl2-buffer-map-tracking-plan.md's Decision 6.
  ID_META_MAPPED_DATA = 0xF0002,
  // Real Win32 window geometry captured at record time - see WindowGeometryMetaCommand's own
  // header comment (common/layer_interface/windowGeometryMetaTypes.h) for the full rationale and
  // the DX12/Vulkan precedent this mirrors.
  ID_META_WINDOW_GEOMETRY = 0xF0003,
  // Client-memory vertex arrays captured at a draw - see ClientArraysMetaCommand
  // (common/layer_interface/clientArraysMetaTypes.h).
  ID_META_CLIENT_ARRAYS = 0xF0004,
  // Capture-time uniform array facts for a glGetUniformLocation - see UniformArrayMetaCommand
  // (common/layer_interface/uniformArrayMetaTypes.h).
  ID_META_UNIFORM_ARRAY = 0xF0005,
  // Contents of a glMapTexture2DINTEL mapping - see MappedTextureDataMetaCommand
  // (common/layer_interface/mappedTextureDataMetaTypes.h).
  ID_META_MAPPED_TEXTURE_DATA = 0xF0006,
  // Capture-time pixel format attributes for a wglCreatePbufferARB - see
  // PixelFormatMetaCommand (common/layer_interface/pixelFormatMetaTypes.h).
  ID_META_PIXEL_FORMAT = 0xF0007,

  // ID_META_BEGIN + 0x500
  ID_GL_BEGIN = 0xF0500,
% for cmd_name, cmd_id in command_ids['GL'].items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
% endfor

  // ID_GL_BEGIN + 0x2000
  ID_WGL_BEGIN = 0xF2500,
% for cmd_name, cmd_id in command_ids['WGL'].items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
% endfor

  // ID_WGL_BEGIN + 0x400
  ID_EGL_BEGIN = 0xF2900,
% for cmd_name, cmd_id in command_ids['EGL'].items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
% endfor

  // ID_EGL_BEGIN + 0x400
  ID_GLX_BEGIN = 0xF2D00,
% for cmd_name, cmd_id in command_ids['GLX'].items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
% endfor

  // Reserved up to ID_GLX_BEGIN + 0x400 (0xF3100); ID_END itself is just one past whatever
  // was last actually emitted above, not a hardcoded boundary (matches DirectX's own
  // commandIdsAuto.h.mako convention).
  ID_END
};

} // namespace gits::opengl2
