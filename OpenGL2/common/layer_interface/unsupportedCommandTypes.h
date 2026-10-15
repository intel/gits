// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "argumentCoders.h"
#include "command.h"
#include "commandSerializer.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

namespace gits::opengl2 {

// Recorded in place of a disabled command's real call. One occurrence is emitted per disabled
// call, independent of the recorder's separate once-per-name warning. The player makes no driver
// call for it and logs a degraded-fidelity warning only for the first occurrence of each name.
class UnsupportedCommand : public Command {
public:
  UnsupportedCommand(uint32_t threadId, std::string name)
      : Command(CommandId::ID_META_UNSUPPORTED_COMMAND, threadId), m_Name(std::move(name)) {}
  UnsupportedCommand() : Command(CommandId::ID_META_UNSUPPORTED_COMMAND) {}

  const std::string& GetName() const {
    return m_Name;
  }

private:
  friend void Decode(const char* src, UnsupportedCommand& command);

  std::string m_Name;
};

inline uint64_t GetSize(const UnsupportedCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + sizeof(uint64_t) +
         command.GetName().size();
}

inline void Encode(const UnsupportedCommand& command, char* dest) {
  uint64_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  const auto nameLength = static_cast<uint64_t>(command.GetName().size());
  Encode(dest, offset, nameLength);
  std::memcpy(dest + offset, command.GetName().data(), nameLength);
}

inline void Decode(const char* src, UnsupportedCommand& command) {
  uint64_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  uint64_t nameLength{};
  Decode(src, offset, nameLength);
  command.m_Name.assign(src + offset, nameLength);
}

class UnsupportedCommandSerializer : public stream::CommandSerializer {
public:
  explicit UnsupportedCommandSerializer(const UnsupportedCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[static_cast<size_t>(m_DataSize)]);
    Encode(command, m_Data.get());
  }

  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_UNSUPPORTED_COMMAND);
  }
};

// API-agnostic frame-end marker - see stream::CommonCommandId::ID_FRAME_END and
// player/streamPlayer.cpp's CommonCommandFactory, which already handles this id for every
// backend (frame counting/control, per-frame message polling via FrameEndCommandRunner). No
// payload of its own, mirroring DirectX's identically-shaped FrameEndCommand
// (DirectX/common/layer_interface/src/include/commandsCustom.h). Recorded once per successful
// wglSwapBuffers/wglSwapLayerBuffers call (see recorderWrappersAuto.cpp.mako's
// frame_end_commands-driven hook) - this backend previously recorded every command, including
// these two, as an ordinary API command with no frame-end marker at all, so the shared player
// infrastructure never ran FrameEndCommandRunner for an OpenGL2 stream.
class FrameEndCommand : public Command {
public:
  explicit FrameEndCommand(uint32_t threadId) : Command(CommandId::ID_FRAME_END, threadId) {}
};

class FrameEndSerializer : public stream::CommandSerializer {
public:
  explicit FrameEndSerializer(const FrameEndCommand& /*command*/) {}

  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_FRAME_END);
  }
};

} // namespace gits::opengl2
