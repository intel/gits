// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "discardedOutputElements.h"

#include "glEnumsAuto.h"
#include "openglDriversAuto.h"

namespace gits::opengl2 {

namespace {

// A list entry is one value (a format, index or name) or a small tuple: GL_DOWNSAMPLE_SCALES_IMG
// reports pairs, GL_SUPPORTED_MULTISAMPLE_MODES_AMD triples. Sizing every entry as four values
// covers both without per-extension layouts.
constexpr size_t MAX_VALUES_PER_LIST_ENTRY = 4;

size_t ListElements(GLint length) {
  return length > 0 ? static_cast<size_t>(length) * MAX_VALUES_PER_LIST_ENTRY : 0;
}

GLenum StateListLengthPname(GLenum pname) {
  switch (pname) {
  case GL_COMPRESSED_TEXTURE_FORMATS:
    return GL_NUM_COMPRESSED_TEXTURE_FORMATS;
  case GL_PROGRAM_BINARY_FORMATS:
    return GL_NUM_PROGRAM_BINARY_FORMATS;
  case GL_SHADER_BINARY_FORMATS:
    return GL_NUM_SHADER_BINARY_FORMATS;
  case GL_DOWNSAMPLE_SCALES_IMG:
    return GL_NUM_DOWNSAMPLE_SCALES_IMG;
  case GL_SUPPORTED_MULTISAMPLE_MODES_AMD:
    return GL_NUM_SUPPORTED_MULTISAMPLE_MODES_AMD;
  default:
    return 0;
  }
}

} // namespace

size_t StateListElements(GLenum pname) {
  const GLenum lengthPname = StateListLengthPname(pname);
  auto realGetIntegerv = player::g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  if (lengthPname == 0 || !realGetIntegerv) {
    return 0;
  }
  GLint length = 0;
  realGetIntegerv(lengthPname, &length);
  return ListElements(length);
}

size_t TexBumpListElements(GLenum pname) {
  auto realGetTexBumpParameteriv =
      player::g_DispatchTable.glGetTexBumpParameterivATI.load(std::memory_order_acquire);
  if (pname != GL_BUMP_TEX_UNITS_ATI || !realGetTexBumpParameteriv) {
    return 0;
  }
  GLint length = 0;
  realGetTexBumpParameteriv(GL_BUMP_NUM_TEX_UNITS_ATI, &length);
  return ListElements(length);
}

size_t AtomicCounterBufferListElements(GLuint program, GLuint bufferIndex, GLenum pname) {
  auto realGetActiveAtomicCounterBufferiv =
      player::g_DispatchTable.glGetActiveAtomicCounterBufferiv.load(std::memory_order_acquire);
  if (pname != GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTER_INDICES ||
      !realGetActiveAtomicCounterBufferiv) {
    return 0;
  }
  GLint length = 0;
  realGetActiveAtomicCounterBufferiv(program, bufferIndex,
                                     GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTERS, &length);
  return ListElements(length);
}

size_t SubroutineUniformListElements(GLuint program,
                                     GLenum shadertype,
                                     GLuint index,
                                     GLenum pname) {
  auto realGetActiveSubroutineUniformiv =
      player::g_DispatchTable.glGetActiveSubroutineUniformiv.load(std::memory_order_acquire);
  if (pname != GL_COMPATIBLE_SUBROUTINES || !realGetActiveSubroutineUniformiv) {
    return 0;
  }
  GLint length = 0;
  realGetActiveSubroutineUniformiv(program, shadertype, index, GL_NUM_COMPATIBLE_SUBROUTINES,
                                   &length);
  return ListElements(length);
}

} // namespace gits::opengl2
