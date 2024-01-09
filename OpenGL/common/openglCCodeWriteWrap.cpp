// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openglCCodeWriteWrap.cpp
*
* @brief Manual overrides of CArgument::Write(CCodeOStream &stream) methods. Applies for OpenGL API.
*
* These overrides are used when the regular ccodeWrap mechanism is not enough.
* For example, when the *_wrap function has different parameters from the API
* call it wraps. The manual overrides allow us to write anything to the CCode.
*/

#include "openglCCodeWriteWrap.h"

#include <cstddef>
#include <iostream>

namespace gits {
namespace OpenGL {

// Overrides
void CglGetProgramResourceiv_CCODEWRITEWRAP(CCodeOStream& stream,
                                            const CglGetProgramResourceiv& function) {
  StartScope(stream);

  RegisterAndDeclareIfNeeded(stream, function._program);
  RegisterAndDeclareIfNeeded(stream, function._programInterface);
  RegisterAndDeclareIfNeeded(stream, function._index);
  RegisterAndDeclareIfNeeded(stream, function._propCount);
  RegisterAndDeclareIfNeeded(stream, function._props);
  RegisterAndDeclareIfNeeded(stream, function._bufSize);
  RegisterAndDeclareIfNeeded(stream, function._length);
  RegisterAndDeclareIfNeeded(stream, function._params);

  stream.Indent() << "glGetProgramResourceiv_wrap(" << function._program << ", "
                  << function._programInterface << ", " << function._index << ", "
                  << function._propCount << ", " << function._props << ", " << function._bufSize
                  << ", " << function._length << ", " << function._params << ", "
                  << "resource_name, "
                  << "locations);" << std::endl;

  EndScope(stream);
}

} // namespace OpenGL
} // namespace gits
