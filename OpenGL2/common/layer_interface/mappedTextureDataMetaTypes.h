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
#include "glTypes.h"

#include <cstdint>

namespace gits::opengl2 {

// The changed contents of glMapTexture2DINTEL mappings, captured by MappedTextureService
// (recorder/include/mappedTextureService.h): before glSyncTextureINTEL (every mapped level of the
// texture), before glUnmapTexture2DINTEL (that level), and before a swap (every mapping of the
// share group). Each of those records at most one command, under a key reserved before any
// mapping is read. The application's own map/sync/unmap calls are not recorded;
// MappedTextureDataMetaRunner maps each level itself on replay, copies the rows and unmaps it
// again. Entry i of m_Textures/m_Levels/m_Strides/m_Rows describes one level, whose m_Rows[i]
// rows of m_Strides[i] bytes follow those of entry i - 1 in m_Data, laid out as the capture
// driver's mapping was; the replay driver may report a different stride.
class MappedTextureDataMetaCommand : public Command {
public:
  MappedTextureDataMetaCommand(uint32_t threadId)
      : Command(CommandId::ID_META_MAPPED_TEXTURE_DATA, threadId) {}
  MappedTextureDataMetaCommand() : Command(CommandId::ID_META_MAPPED_TEXTURE_DATA) {}

public:
  HandleArrayArgument<GLuint> m_Textures{};
  ArrayArgument<GLint> m_Levels{};
  ArrayArgument<GLint> m_Strides{};
  ArrayArgument<GLint> m_Rows{};
  ArrayArgument<uint8_t> m_Data{};
};

inline uint64_t GetSize(const MappedTextureDataMetaCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_Textures) +
         GetSize(command.m_Levels) + GetSize(command.m_Strides) + GetSize(command.m_Rows) +
         GetSize(command.m_Data);
}

inline void Encode(const MappedTextureDataMetaCommand& command, char* dest) {
  uint64_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_Textures);
  Encode(dest, offset, command.m_Levels);
  Encode(dest, offset, command.m_Strides);
  Encode(dest, offset, command.m_Rows);
  Encode(dest, offset, command.m_Data);
}

inline void Decode(const char* src, MappedTextureDataMetaCommand& command) {
  uint64_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_Textures);
  Decode(src, offset, command.m_Levels);
  Decode(src, offset, command.m_Strides);
  Decode(src, offset, command.m_Rows);
  Decode(src, offset, command.m_Data);
}

class MappedTextureDataMetaSerializer : public stream::CommandSerializer {
public:
  explicit MappedTextureDataMetaSerializer(const MappedTextureDataMetaCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[static_cast<size_t>(m_DataSize)]);
    Encode(command, m_Data.get());
  }

  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_MAPPED_TEXTURE_DATA);
  }
};

} // namespace gits::opengl2
