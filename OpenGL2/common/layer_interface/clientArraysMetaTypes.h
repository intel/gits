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

// Which gl*Pointer call re-specifies an array on replay.
enum class ClientArrayKind : uint32_t {
  Vertex = 0,
  Normal = 1,
  Color = 2,
  SecondaryColor = 3,
  FogCoord = 4,
  Index = 5,
  EdgeFlag = 6,
  TexCoord = 7,     // Slot is the texture unit index (0 for GL_TEXTURE0).
  VertexAttrib = 8, // Slot is the generic attribute index.
};

// Bits of ClientArraysMetaCommand::m_Flags.
constexpr uint32_t CLIENT_ARRAY_FLAG_NORMALIZED = 1u << 0; // glVertexAttribPointer `normalized`
constexpr uint32_t CLIENT_ARRAY_FLAG_INTEGER = 1u << 1;    // glVertexAttribIPointer
constexpr uint32_t CLIENT_ARRAY_FLAG_LONG = 1u << 2;       // glVertexAttribLPointer

// Recorded immediately before a draw that reads vertex arrays from client memory (arrays enabled
// with no GL_ARRAY_BUFFER bound when their pointer was specified). The gl*Pointer commands
// themselves only carry the recording process's address; this carries the bytes the draw reads.
//
// The bytes travel in blocks, which the player keeps by id until released:
// - m_BlockIds/m_BlockSizes/m_Data: blocks sent by this command (new ids, or replacing the
//   content of an existing id), m_BlockSizes[i] bytes of m_Data each, in order.
// - m_ReleasedBlocks: ids the player may free. Never referenced by this or any later command.
// A block covers the union of overlapping array ranges, so interleaved arrays share one copy,
// and a block whose content has not changed since it was sent is only referenced again.
//
// Arrays are parallel lists. Array i reads block m_ArrayBlocks[i]; its pointer is that block's
// first byte plus m_ArrayOffsets[i] (negative when the draw's lowest vertex is not the first).
class ClientArraysMetaCommand : public Command {
public:
  ClientArraysMetaCommand(uint32_t threadId)
      : Command(CommandId::ID_META_CLIENT_ARRAYS, threadId) {}
  ClientArraysMetaCommand() : Command(CommandId::ID_META_CLIENT_ARRAYS) {}

public:
  ArrayArgument<uint64_t> m_ReleasedBlocks{};
  ArrayArgument<uint64_t> m_BlockIds{};
  ArrayArgument<uint64_t> m_BlockSizes{};
  ArrayArgument<uint8_t> m_Data{};
  ArrayArgument<uint32_t> m_Kinds{};
  ArrayArgument<uint32_t> m_Slots{};
  ArrayArgument<int32_t> m_Sizes{};
  ArrayArgument<uint32_t> m_Types{};
  ArrayArgument<int32_t> m_Strides{};
  ArrayArgument<uint32_t> m_Flags{};
  ArrayArgument<uint64_t> m_ArrayBlocks{};
  ArrayArgument<int64_t> m_ArrayOffsets{};
};

inline uint64_t GetSize(const ClientArraysMetaCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_ReleasedBlocks) +
         GetSize(command.m_BlockIds) + GetSize(command.m_BlockSizes) + GetSize(command.m_Data) +
         GetSize(command.m_Kinds) + GetSize(command.m_Slots) + GetSize(command.m_Sizes) +
         GetSize(command.m_Types) + GetSize(command.m_Strides) + GetSize(command.m_Flags) +
         GetSize(command.m_ArrayBlocks) + GetSize(command.m_ArrayOffsets);
}

inline void Encode(const ClientArraysMetaCommand& command, char* dest) {
  uint64_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_ReleasedBlocks);
  Encode(dest, offset, command.m_BlockIds);
  Encode(dest, offset, command.m_BlockSizes);
  Encode(dest, offset, command.m_Data);
  Encode(dest, offset, command.m_Kinds);
  Encode(dest, offset, command.m_Slots);
  Encode(dest, offset, command.m_Sizes);
  Encode(dest, offset, command.m_Types);
  Encode(dest, offset, command.m_Strides);
  Encode(dest, offset, command.m_Flags);
  Encode(dest, offset, command.m_ArrayBlocks);
  Encode(dest, offset, command.m_ArrayOffsets);
}

inline void Decode(const char* src, ClientArraysMetaCommand& command) {
  uint64_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_ReleasedBlocks);
  Decode(src, offset, command.m_BlockIds);
  Decode(src, offset, command.m_BlockSizes);
  Decode(src, offset, command.m_Data);
  Decode(src, offset, command.m_Kinds);
  Decode(src, offset, command.m_Slots);
  Decode(src, offset, command.m_Sizes);
  Decode(src, offset, command.m_Types);
  Decode(src, offset, command.m_Strides);
  Decode(src, offset, command.m_Flags);
  Decode(src, offset, command.m_ArrayBlocks);
  Decode(src, offset, command.m_ArrayOffsets);
}

class ClientArraysMetaSerializer : public stream::CommandSerializer {
public:
  explicit ClientArraysMetaSerializer(const ClientArraysMetaCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[static_cast<size_t>(m_DataSize)]);
    Encode(command, m_Data.get());
  }

  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_CLIENT_ARRAYS);
  }
};

} // namespace gits::opengl2
