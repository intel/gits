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
#include "glEnumsAuto.h"
#include "glTypes.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace gits::opengl2 {

// The active uniform array a glGetUniformLocation name belongs to: its declared length and the
// queried location's index in it. -1/-1 if no active uniform matches.
struct UniformArrayElement {
  GLint ArraySize = -1;
  GLint ArrayIndex = -1;
};

// Port of the legacy OpenGL backend's GetUniformArraySizeAndOffset
// (OpenGL/common/openglTools.cpp). Finds the active uniform whose base name (`name` with a single
// trailing "[N]" chopped off; a ".field" suffix stops the chop, so struct members do not match an
// outer array) matches, and returns that uniform's size and `location`'s offset from its element
// 0. Shared by the recorder (capture driver) and the player (replay driver), which pass their own
// real entry points.
template <typename GetProgramivFn, typename GetActiveUniformFn, typename GetUniformLocationFn>
UniformArrayElement FindUniformArrayElement(GetProgramivFn getProgramiv,
                                            GetActiveUniformFn getActiveUniform,
                                            GetUniformLocationFn getUniformLocation,
                                            GLuint program,
                                            const GLchar* name,
                                            GLint location) {
  UniformArrayElement element;
  if (name == nullptr || !getProgramiv || !getActiveUniform || !getUniformLocation) {
    return element;
  }
  // In-place: replaces the last '[' with a NUL unless a '.' follows it.
  auto chopName = [](std::vector<char>& buf) {
    for (size_t i = buf.size(); i > 0; --i) {
      if (buf[i - 1] == '.') {
        break;
      }
      if (buf[i - 1] != '[') {
        continue;
      }
      buf[i - 1] = 0;
      break;
    }
  };

  std::vector<char> queriedName(name, name + std::strlen(name) + 1);
  chopName(queriedName);

  GLint uniformCount = 0;
  GLint nameMaxLen = 0;
  getProgramiv(program, GL_ACTIVE_UNIFORMS, &uniformCount);
  getProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &nameMaxLen);
  if (nameMaxLen <= 0) {
    return element;
  }
  std::vector<char> candidateName(static_cast<size_t>(nameMaxLen) + 1);
  for (GLint i = 0; i < uniformCount; ++i) {
    GLsizei nameLen = 0;
    GLint size = 0;
    GLenum type = 0;
    getActiveUniform(program, static_cast<GLuint>(i), nameMaxLen, &nameLen, &size, &type,
                     candidateName.data());
    std::vector<char> chopped(candidateName.begin(), candidateName.begin() + nameLen + 1);
    chopName(chopped);
    if (std::strcmp(chopped.data(), queriedName.data()) == 0) {
      element.ArrayIndex = location - getUniformLocation(program, chopped.data());
      element.ArraySize = size;
      return element;
    }
  }
  return element;
}

// Capture-time array facts for a glGetUniformLocation that returned an element of a uniform
// array (ArraySize > 1). Recorded right after that command. Applications commonly query one
// element by name and compute the others as location + i; the player maps those computed
// locations for the index range both drivers keep active, which only the capture-time size can
// bound: a range from the replay size alone could claim a captured location that belonged to a
// different uniform at capture time.
class UniformArrayMetaCommand : public Command {
public:
  UniformArrayMetaCommand(uint32_t threadId)
      : Command(CommandId::ID_META_UNIFORM_ARRAY, threadId) {}
  UniformArrayMetaCommand() : Command(CommandId::ID_META_UNIFORM_ARRAY) {}

public:
  // The program's object-name key, as recorded for the glGetUniformLocation.
  Argument<GITSKey> m_Program{};
  // The name queried, null-terminated.
  ArrayArgument<GLchar> m_Name{};
  // The location glGetUniformLocation returned at capture time.
  Argument<GLint> m_Location{};
  Argument<GLint> m_ArraySize{};
  Argument<GLint> m_ArrayIndex{};
};

inline uint64_t GetSize(const UniformArrayMetaCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_Program) +
         GetSize(command.m_Name) + GetSize(command.m_Location) + GetSize(command.m_ArraySize) +
         GetSize(command.m_ArrayIndex);
}

inline void Encode(const UniformArrayMetaCommand& command, char* dest) {
  uint64_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_Program);
  Encode(dest, offset, command.m_Name);
  Encode(dest, offset, command.m_Location);
  Encode(dest, offset, command.m_ArraySize);
  Encode(dest, offset, command.m_ArrayIndex);
}

inline void Decode(const char* src, UniformArrayMetaCommand& command) {
  uint64_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_Program);
  Decode(src, offset, command.m_Name);
  Decode(src, offset, command.m_Location);
  Decode(src, offset, command.m_ArraySize);
  Decode(src, offset, command.m_ArrayIndex);
}

class UniformArrayMetaSerializer : public stream::CommandSerializer {
public:
  explicit UniformArrayMetaSerializer(const UniformArrayMetaCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[static_cast<size_t>(m_DataSize)]);
    Encode(command, m_Data.get());
  }

  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_UNIFORM_ARRAY);
  }
};

} // namespace gits::opengl2
