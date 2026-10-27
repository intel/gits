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

namespace gits::opengl {

// Recorded by MapTrackingService::UnmapBufferAndCapture (see recorder/mapTrackingService.cpp)
// instead of a generated command for glMapBufferRange/glUnmapBuffer themselves - see
// docs/development/opengl-buffer-map-tracking-plan.md's Decision 3 for why those two functions
// bypass the generated per-command path entirely, and Decision 6 for this command's own wire
// format. Carries the snapshot taken "update on unmap only" (Decision 2): the buffer's identity
// (by GITSKey - m_Buffer.Value is never serialized, exactly like every other HandleArgument<T> -
// so replay resolves to whichever buffer the player itself created, not the recorder's raw
// GLuint), the mapped range's offset/length within that buffer, and the captured bytes. Replayed
// by MappedDataMetaRunner (player/include/mappedDataMetaRunner.h), registered by
// OpenGLCommandFactory for CommandId::ID_META_MAPPED_DATA - mirrors UnsupportedCommand/
// UnsupportedCommandRunner's structural precedent (a hand-written meta-command with a
// hand-written factory registration, no generated counterpart) and Vulkan's own
// MappedDataMetaCommand naming (Vulkan/common/layer_interface/commandsCustom.h), simplified to
// the single-region shape "update on unmap only" ever produces (Vulkan's own type carries a
// multi-region MemoryRegions list, needed for its continuous dirty-tracking - not needed here).
class MappedDataMetaCommand : public Command {
public:
  MappedDataMetaCommand(uint32_t threadId) : Command(CommandId::ID_META_MAPPED_DATA, threadId) {}
  MappedDataMetaCommand() : Command(CommandId::ID_META_MAPPED_DATA) {}

public:
  HandleArgument<GLuint> m_Buffer{};
  Argument<uint64_t> m_Offset{};
  Argument<uint64_t> m_Length{};
  ArrayArgument<uint8_t> m_Data{};
};

// Hand-assembled from the same generic, per-field GetSize/Encode/Decode overloads
// (argumentCoders.h) every generated command's own coder composes - there is simply no
// per-command loop to add this command to, since it has no gl.xml-derived counterpart. Field
// order (m_Key, m_ThreadId, then declaration order) matches every generated command's own
// convention.
//
// m_Data previously needed its own bespoke, hand-rolled 64-bit coder here (GetMappedPayloadSize/
// EncodeMappedPayload/DecodeMappedPayload), because argumentCoders.h's own generic
// ArrayArgument<T> overloads were 32-bit at the time - this field's length originates from a
// 64-bit GLsizeiptr (see mapTrackingService.cpp) and can legitimately approach or exceed 4 GiB
// on this x64-only backend, which the then-32-bit generic coder could silently truncate/
// corrupt. Now that argumentCoders.h's generic overloads are 64-bit throughout (see that
// header's own top-of-file comment - the same change also closed this gap for every
// OTHER command's own array-typed fields, e.g. glBufferData/glBufferSubData's `data`), this
// field uses the same generic GetSize/Encode/Decode(const ArrayArgument<T>&) overload as
// everything else below, with no bespoke coder of its own and no wire-format change (the
// generic overload already used this exact 1-byte-flag + 8-byte-count + raw-bytes layout).
inline uint64_t GetSize(const MappedDataMetaCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_Buffer) +
         GetSize(command.m_Offset) + GetSize(command.m_Length) + GetSize(command.m_Data);
}

inline void Encode(const MappedDataMetaCommand& command, char* dest) {
  uint64_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_Buffer);
  Encode(dest, offset, command.m_Offset);
  Encode(dest, offset, command.m_Length);
  Encode(dest, offset, command.m_Data);
}

inline void Decode(const char* src, MappedDataMetaCommand& command) {
  uint64_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_Buffer);
  Decode(src, offset, command.m_Offset);
  Decode(src, offset, command.m_Length);
  Decode(src, offset, command.m_Data);
}

class MappedDataMetaSerializer : public stream::CommandSerializer {
public:
  explicit MappedDataMetaSerializer(const MappedDataMetaCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[static_cast<size_t>(m_DataSize)]);
    Encode(command, m_Data.get());
  }

  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_MAPPED_DATA);
  }
};

} // namespace gits::opengl
