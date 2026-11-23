// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"
#include "fastOStream.h"
#include "printArguments.h"

#include <mutex>

namespace gits::opengl {

class CommandPrinter {
public:
  CommandPrinter(FastOStream& stream, std::mutex& mutex, const Command& command, const char* name);

  template <typename T>
  void Add(const T& value) {
    if (m_HasArgument) {
      m_Stream << ", ";
    }
    PrintArgument(m_Stream, value);
    m_HasArgument = true;
  }

  void AddBoolean(GLboolean value) {
    if (m_HasArgument) {
      m_Stream << ", ";
    }
    PrintGLBoolean(m_Stream, value);
    m_HasArgument = true;
  }

  void AnnotateUnknownSize() {
    m_Stream << " [size=unknown]";
  }

  void AddEnum(GLenum value, GLenumApi api, GLenumGroup group) {
    if (m_HasArgument) {
      m_Stream << ", ";
    }
    PrintEnum(m_Stream, value, api, group);
    m_HasArgument = true;
  }

  template <typename T>
  void AddEnumArray(const ArrayArgument<T>& value, GLenumApi api, GLenumGroup group) {
    if (m_HasArgument) {
      m_Stream << ", ";
    }
    PrintEnumArray(m_Stream, value, api, group);
    m_HasArgument = true;
  }

  void AddOpaque(uintptr_t value) {
    if (m_HasArgument) {
      m_Stream << ", ";
    }
    PrintOpaque(m_Stream, value);
    m_HasArgument = true;
  }

  void AddText(std::string_view text) {
    if (m_HasArgument) {
      m_Stream << ", ";
    }
    PrintText(m_Stream, text, std::min(text.size(), kMaxPrintedTextBytes));
    m_HasArgument = true;
  }

  template <typename T>
  void Result(const T& value) {
    m_Stream << ") = ";
    PrintArgument(m_Stream, value);
    m_HasResult = true;
  }

  void ResultBoolean(GLboolean value) {
    m_Stream << ") = ";
    PrintGLBoolean(m_Stream, value);
    m_HasResult = true;
  }

  void ResultEnum(GLenum value, GLenumApi api, GLenumGroup group) {
    m_Stream << ") = ";
    PrintEnum(m_Stream, value, api, group);
    m_HasResult = true;
  }

  void Finish(bool flush, const char* suffix = "");

private:
  FastOStream& m_Stream;
  std::unique_lock<std::mutex> m_Lock;
  bool m_HasArgument{};
  bool m_HasResult{};
};

} // namespace gits::opengl
