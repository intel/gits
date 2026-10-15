// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Queries of the replay driver's state for subcapture state restore.

#include "glEnumsAuto.h"
#include "openglDriversAuto.h"

#include <atomic>
#include <string>
#include <vector>

namespace gits::opengl2 {

template <typename FunctionT>
FunctionT Gl(const std::atomic<FunctionT>& function) {
  return function.load(std::memory_order_acquire);
}

#define GL_FN(name) Gl(player::g_DispatchTable.name)

inline constexpr GLint MAX_RESTORED_VERTEX_ATTRIBUTES = 32;

inline GLint GetInteger(GLenum pname) {
  GLint value = 0;
  GL_FN(glGetIntegerv)(pname, &value);
  return value;
}

inline GLint GetIntegerIndexed(GLenum pname, GLuint index) {
  GLint value = 0;
  GL_FN(glGetIntegeri_v)(pname, index, &value);
  return value;
}

inline GLint64 GetInteger64Indexed(GLenum pname, GLuint index) {
  GLint64 value = 0;
  GL_FN(glGetInteger64i_v)(pname, index, &value);
  return value;
}

inline GLfloat GetFloat(GLenum pname) {
  GLfloat value = 0;
  GL_FN(glGetFloatv)(pname, &value);
  return value;
}

inline GLuint GetName(GLenum pname) {
  return static_cast<GLuint>(GetInteger(pname));
}

inline GLint GetVertexAttribute(GLuint index, GLenum pname) {
  GLint value = 0;
  GL_FN(glGetVertexAttribiv)(index, pname, &value);
  return value;
}

struct TextureTarget {
  GLenum Target;
  GLenum Binding;
  int Major;
  int Minor;
};

inline constexpr TextureTarget TEXTURE_TARGETS[] = {
    {GL_TEXTURE_1D, GL_TEXTURE_BINDING_1D, 1, 0},
    {GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D, 1, 0},
    {GL_TEXTURE_3D, GL_TEXTURE_BINDING_3D, 1, 2},
    {GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP, 1, 3},
    {GL_TEXTURE_1D_ARRAY, GL_TEXTURE_BINDING_1D_ARRAY, 3, 0},
    {GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BINDING_2D_ARRAY, 3, 0},
    {GL_TEXTURE_RECTANGLE, GL_TEXTURE_BINDING_RECTANGLE, 3, 1},
    {GL_TEXTURE_BUFFER, GL_TEXTURE_BINDING_BUFFER, 3, 1},
    {GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BINDING_2D_MULTISAMPLE, 3, 2},
    {GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY, 3, 2},
    {GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BINDING_CUBE_MAP_ARRAY, 4, 0},
};

inline const TextureTarget* FindTextureTarget(GLenum target) {
  for (const TextureTarget& entry : TEXTURE_TARGETS) {
    if (entry.Target == target) {
      return &entry;
    }
  }
  return nullptr;
}

inline std::vector<GLuint> AttachedShaderNames(GLuint program) {
  GLint count = 0;
  GL_FN(glGetProgramiv)(program, GL_ATTACHED_SHADERS, &count);
  std::vector<GLuint> shaders(static_cast<size_t>(count > 0 ? count : 0));
  GLsizei written = 0;
  if (!shaders.empty()) {
    GL_FN(glGetAttachedShaders)(program, count, &written, shaders.data());
  }
  shaders.resize(static_cast<size_t>(written));
  return shaders;
}

inline std::string ShaderSource(GLuint shader) {
  GLint length = 0;
  GL_FN(glGetShaderiv)(shader, GL_SHADER_SOURCE_LENGTH, &length);
  if (length <= 0) {
    return {};
  }
  std::vector<GLchar> source(static_cast<size_t>(length));
  GLsizei written = 0;
  GL_FN(glGetShaderSource)(shader, length, &written, source.data());
  return std::string(source.data(), static_cast<size_t>(written));
}

} // namespace gits::opengl2
