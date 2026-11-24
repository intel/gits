// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "glBufferQueries.h"

#include "glContextCapabilities.h"
#include "glEnumsAuto.h"
#include "glVersionString.h"
#include "glDispatchAuto.h"

#include <atomic>

namespace gits::opengl {

GLenum BufferBindingPname(GLenum target) {
  switch (target) {
  case GL_ARRAY_BUFFER:
    return GL_ARRAY_BUFFER_BINDING;
  case GL_ELEMENT_ARRAY_BUFFER:
    return GL_ELEMENT_ARRAY_BUFFER_BINDING;
  case GL_PIXEL_PACK_BUFFER:
    return GL_PIXEL_PACK_BUFFER_BINDING;
  case GL_PIXEL_UNPACK_BUFFER:
    return GL_PIXEL_UNPACK_BUFFER_BINDING;
  case GL_UNIFORM_BUFFER:
    return GL_UNIFORM_BUFFER_BINDING;
  case GL_TRANSFORM_FEEDBACK_BUFFER:
    return GL_TRANSFORM_FEEDBACK_BUFFER_BINDING;
  case GL_COPY_READ_BUFFER:
    return GL_COPY_READ_BUFFER_BINDING;
  case GL_COPY_WRITE_BUFFER:
    return GL_COPY_WRITE_BUFFER_BINDING;
  case GL_DRAW_INDIRECT_BUFFER:
    return GL_DRAW_INDIRECT_BUFFER_BINDING;
  case GL_ATOMIC_COUNTER_BUFFER:
    return GL_ATOMIC_COUNTER_BUFFER_BINDING;
  case GL_DISPATCH_INDIRECT_BUFFER:
    return GL_DISPATCH_INDIRECT_BUFFER_BINDING;
  case GL_SHADER_STORAGE_BUFFER:
    return GL_SHADER_STORAGE_BUFFER_BINDING;
  case GL_QUERY_BUFFER:
    return GL_QUERY_BUFFER_BINDING;
  case GL_TEXTURE_BUFFER:
    return GL_TEXTURE_BUFFER_BINDING;
  case GL_PARAMETER_BUFFER:
    return GL_PARAMETER_BUFFER_BINDING;
  default:
    return 0;
  }
}

bool GetBoundBufferParameter(GLenum target, GLenum pname, GLint& value) {
  if (!TryResolveRealFunction("glGetBufferParameteriv")) {
    return false;
  }
  auto get = g_DispatchTable.glGetBufferParameteriv.load(std::memory_order_acquire);
  get(target, pname, &value);
  return true;
}

bool GetBoundBufferParameter(GLenum target, GLenum pname, GLint64& value) {
  if (pname == GL_BUFFER_SIZE) {
    return GetBoundBufferSize(target, value);
  }
  GLint result = 0;
  if (!GetBoundBufferParameter(target, pname, result)) {
    return false;
  }
  value = result;
  return true;
}

bool GetBoundBufferSize(GLenum target, GLint64& size) {
  if (GlVersionAtLeast(CurrentContextGLVersion(), 3, 2)) {
    if (!TryResolveRealFunction("glGetBufferParameteri64v")) {
      return false;
    }
    auto get = g_DispatchTable.glGetBufferParameteri64v.load(std::memory_order_acquire);
    get(target, GL_BUFFER_SIZE, &size);
    return true;
  }
  GLint value = 0;
  if (!GetBoundBufferParameter(target, GL_BUFFER_SIZE, value)) {
    return false;
  }
  size = value;
  return true;
}

} // namespace gits::opengl
