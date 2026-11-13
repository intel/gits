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

#include <algorithm>
#include <atomic>
#include <cstring>
#include <string>
#include <vector>

namespace gits::opengl {

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

// Whether the current context, of GL `major`.`minor`, keeps the features GL 3.0 deprecated. A
// forward-compatible context removes them, a GL 3.1 context keeps them only with
// GL_ARB_compatibility, and a later one only in a compatibility profile.
inline bool CompatibilityContext(int major, int minor) {
  if (major < 3) {
    return true;
  }
  if ((GetInteger(GL_CONTEXT_FLAGS) & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) != 0) {
    return false;
  }
  if (major == 3 && minor == 0) {
    return true;
  }
  if (major == 3 && minor == 1) {
    const GLint count = GetInteger(GL_NUM_EXTENSIONS);
    for (GLint i = 0; i < count; ++i) {
      const auto* name =
          reinterpret_cast<const char*>(GL_FN(glGetStringi)(GL_EXTENSIONS, static_cast<GLuint>(i)));
      if (name != nullptr && std::strcmp(name, "GL_ARB_compatibility") == 0) {
        return true;
      }
    }
    return false;
  }
  return (GetInteger(GL_CONTEXT_PROFILE_MASK) & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0;
}

inline GLint GetVertexAttribute(GLuint index, GLenum pname) {
  GLint value = 0;
  GL_FN(glGetVertexAttribiv)(index, pname, &value);
  return value;
}

// The buffer of a vertex buffer binding of the bound vertex array. Before GL 4.4 it can be queried
// only through an attribute reading the binding, so attribute 0 reads it meanwhile.
inline GLuint VertexBindingBuffer(GLuint binding, bool bindingBuffers) {
  if (bindingBuffers) {
    return static_cast<GLuint>(GetIntegerIndexed(GL_VERTEX_BINDING_BUFFER, binding));
  }
  const auto original = static_cast<GLuint>(GetVertexAttribute(0, GL_VERTEX_ATTRIB_BINDING));
  GL_FN(glVertexAttribBinding)(0, binding);
  const auto buffer =
      static_cast<GLuint>(GetVertexAttribute(0, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING));
  GL_FN(glVertexAttribBinding)(0, original);
  return buffer;
}

struct TextureTarget {
  GLenum Target;
  GLenum Binding;
  int Major;
  int Minor;
  const char* Extension{};
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
    {GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BINDING_CUBE_MAP_ARRAY, 4, 0,
     "GL_ARB_texture_cube_map_array"},
};

inline const TextureTarget* FindTextureTarget(GLenum target) {
  for (const TextureTarget& entry : TEXTURE_TARGETS) {
    if (entry.Target == target) {
      return &entry;
    }
  }
  return nullptr;
}

// The shaders attached to the program, ordered by `order`, the names in the order they were
// attached. glGetAttachedShaders returns them in an order of its own, while the resources a link
// assigns can depend on the order of the shaders.
inline std::vector<GLuint> AttachedShaderNames(GLuint program,
                                               const std::vector<GLuint>& order = {}) {
  GLint count = 0;
  GL_FN(glGetProgramiv)(program, GL_ATTACHED_SHADERS, &count);
  std::vector<GLuint> shaders(static_cast<size_t>(count > 0 ? count : 0));
  GLsizei written = 0;
  if (!shaders.empty()) {
    GL_FN(glGetAttachedShaders)(program, count, &written, shaders.data());
  }
  shaders.resize(static_cast<size_t>(written));
  const auto position = [&order](GLuint shader) {
    return std::find(order.begin(), order.end(), shader) - order.begin();
  };
  std::stable_sort(shaders.begin(), shaders.end(),
                   [&](GLuint a, GLuint b) { return position(a) < position(b); });
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

} // namespace gits::opengl
