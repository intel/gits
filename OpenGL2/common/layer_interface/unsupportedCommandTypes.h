// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"
#include "commandSerializer.h"

#include <cstdint>
#include <cstring>
#include <string>

namespace gits::opengl2 {

// Recorded in place of a disabled command's real call, carrying only its canonical
// name. See modern-opengl-backend-architecture.md, "Intercept every known runtime
// command, support commands incrementally": one occurrence is emitted per disabled call,
// independent of the recorder's separate once-per-name warning (see
// recorder/unsupportedCommand.h). The player makes no driver call for it and logs a
// degraded-fidelity warning only for the first occurrence of each name - see
// player/include/unsupportedCommandRunner.h, registered by OpenGL2CommandFactory for
// CommandId::ID_META_UNSUPPORTED_COMMAND.
class UnsupportedCommand : public Command {
public:
  UnsupportedCommand(uint32_t threadId, std::string name)
      : Command(CommandId::ID_META_UNSUPPORTED_COMMAND, threadId), m_Name(std::move(name)) {}

  const std::string& GetName() const {
    return m_Name;
  }

private:
  std::string m_Name;
};

// Flat [nameLength][name bytes] payload.
class UnsupportedCommandSerializer : public stream::CommandSerializer {
public:
  explicit UnsupportedCommandSerializer(const UnsupportedCommand& command) {
    const auto& name = command.GetName();
    const auto nameLength = static_cast<uint32_t>(name.size());
    m_DataSize = sizeof(nameLength) + nameLength;
    m_Data.reset(new char[static_cast<size_t>(m_DataSize)]);
    std::memcpy(m_Data.get(), &nameLength, sizeof(nameLength));
    std::memcpy(m_Data.get() + sizeof(nameLength), name.data(), nameLength);
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
