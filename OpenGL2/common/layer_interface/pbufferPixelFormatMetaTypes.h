// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "argumentCoders.h"
#include "command.h"
#include "commandSerializer.h"

#include <cstdint>

namespace gits::opengl2 {

// The capture driver's attributes of a wglCreatePbufferARB/EXT's pixel format, recorded right
// before that command. Its iPixelFormat is an index into the capture driver's format table, so the
// player chooses a format with these attributes on the replay driver instead - the same
// re-negotiation the legacy backend's CwglCreatePbufferARB does (OpenGL/common/wglFunctions.cpp).
class PbufferPixelFormatMetaCommand : public Command {
public:
  PbufferPixelFormatMetaCommand(uint32_t threadId)
      : Command(CommandId::ID_META_PBUFFER_PIXEL_FORMAT, threadId) {}
  PbufferPixelFormatMetaCommand() : Command(CommandId::ID_META_PBUFFER_PIXEL_FORMAT) {}

public:
  // {WGL_*_ARB attribute, value} pairs, without the terminating 0. Empty when the attributes
  // could not be queried.
  ArrayArgument<int32_t> m_Attributes{};
};

inline uint64_t GetSize(const PbufferPixelFormatMetaCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_Attributes);
}

inline void Encode(const PbufferPixelFormatMetaCommand& command, char* dest) {
  uint64_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_Attributes);
}

inline void Decode(const char* src, PbufferPixelFormatMetaCommand& command) {
  uint64_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_Attributes);
}

class PbufferPixelFormatMetaSerializer : public stream::CommandSerializer {
public:
  explicit PbufferPixelFormatMetaSerializer(const PbufferPixelFormatMetaCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[static_cast<size_t>(m_DataSize)]);
    Encode(command, m_Data.get());
  }

  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_PBUFFER_PIXEL_FORMAT);
  }
};

} // namespace gits::opengl2
