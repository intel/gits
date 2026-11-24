// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"

namespace gits::opengl {

// Returns the binding pname for `target`, or 0 if unrecognized. Does not check context support.
GLenum BufferBindingPname(GLenum target);

// glGetBufferParameteriv of the buffer bound to `target`, through the core entry point.
// False (and `value` untouched) if the getter could not be resolved.
bool GetBoundBufferParameter(GLenum target, GLenum pname, GLint& value);

// Uses GetBoundBufferSize for GL_BUFFER_SIZE and widens other integer parameters to GLint64.
bool GetBoundBufferParameter(GLenum target, GLenum pname, GLint64& value);

// GL_BUFFER_SIZE of the buffer bound to `target`. GL_BUFFER_SIZE is a GLsizeiptr, so it is
// queried through glGetBufferParameteri64v on a GL 3.2+ context; older contexts only have the
// 32-bit getter of GetBoundBufferParameter. False (and `size` untouched) if no getter resolved.
bool GetBoundBufferSize(GLenum target, GLint64& size);

} // namespace gits::opengl
