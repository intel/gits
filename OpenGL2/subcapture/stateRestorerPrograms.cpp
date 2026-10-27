// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateRestorer.h"

#include "commandSerializersAuto.h"
#include "liveGl.h"
#include "uniformLocationMapService.h"

#include <string_view>

namespace gits::opengl2 {

namespace {

bool IsBuiltIn(std::string_view name) {
  return name.starts_with("gl_");
}

template <typename T, typename GetterT>
std::vector<T> ReadUniform(GetterT getter, GLuint program, GLint location, size_t components) {
  std::vector<T> values(16);
  getter(program, location, values.data());
  values.resize(components);
  return values;
}

} // namespace

void StateRestorer::RestoreShader(GITSKey key, GLuint name) {
  if (name == 0 || !GL_FN(glIsShader)(name)) {
    return;
  }
  GLint type = 0;
  GLint compiled = GL_FALSE;
  GL_FN(glGetShaderiv)(name, GL_SHADER_TYPE, &type);
  GL_FN(glGetShaderiv)(name, GL_COMPILE_STATUS, &compiled);
  EmitShader(key, static_cast<GLenum>(type), Required(key) ? ShaderSource(name) : std::string(),
             compiled != GL_FALSE);
}

void StateRestorer::EmitShader(GITSKey key, GLenum type, const std::string& source, bool compile) {
  glCreateShaderCommand create;
  create.m_type.Value = type;
  create.m_Return.Key = key;
  Emit<glCreateShaderCommandSerializer>(create);
  if (source.empty()) {
    return;
  }
  glShaderSourceCommand shaderSource;
  shaderSource.m_shader.Key = key;
  shaderSource.m_count.Value = 1;
  const GLchar* text = source.c_str();
  shaderSource.m_string = ArrayOfArrays<GLchar>(&text, 1, nullptr);
  Emit<glShaderSourceCommandSerializer>(shaderSource);
  if (compile) {
    glCompileShaderCommand compileShader;
    compileShader.m_shader.Key = key;
    Emit<glCompileShaderCommandSerializer>(compileShader);
  }
}

void StateRestorer::RestoreProgram(GITSKey key, GLuint name) {
  if (name == 0 || !GL_FN(glIsProgram)(name)) {
    return;
  }
  const ProgramState* tracked = m_State.FindProgram(key);
  GLint linked = GL_FALSE;
  GL_FN(glGetProgramiv)(name, GL_LINK_STATUS, &linked);
  const size_t setupCount = tracked != nullptr ? tracked->LinkSetup.size() : 0;
  const size_t linkedSetupCount = tracked != nullptr ? tracked->LinkedSetupCount : 0;
  const auto emitLinkSetup = [&](size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
      tracked->LinkSetup[i]->Record(m_Recorder, m_Recorder.CreateStateRestoreKey(), m_ThreadId);
    }
  };

  if (Required(key) && tracked != nullptr && tracked->Creation) {
    tracked->Creation->Record(m_Recorder, m_Recorder.CreateStateRestoreKey(), m_ThreadId);
    emitLinkSetup(0, setupCount);
    if (linked) {
      RestoreUniforms(key, name);
    }
    return;
  }

  glCreateProgramCommand create;
  create.m_Return.Key = key;
  Emit<glCreateProgramCommandSerializer>(create);
  if (!Required(key)) {
    return;
  }

  const auto attach = [this](GITSKey program, GITSKey shader) {
    glAttachShaderCommand command;
    command.m_program.Key = program;
    command.m_shader.Key = shader;
    Emit<glAttachShaderCommandSerializer>(command);
  };
  const auto attachCurrentShaders = [&] {
    for (GLuint shader : AttachedShaderNames(name)) {
      if (const GITSKey shaderKey = KeyOf(GLObjectKind::Shader, shader)) {
        attach(key, shaderKey);
      }
    }
  };

  if (tracked != nullptr && tracked->Binary) {
    attachCurrentShaders();
    emitLinkSetup(0, linkedSetupCount);
    tracked->Binary->Record(m_Recorder, m_Recorder.CreateStateRestoreKey(), m_ThreadId);
    emitLinkSetup(linkedSetupCount, setupCount);
    if (linked) {
      RestoreUniforms(key, name);
    }
    return;
  }

  // A program whose last link failed is linked from the snapshot of the last successful one, the
  // executable it keeps using while in use.
  const bool relinkFromSnapshot = tracked != nullptr && !tracked->LinkedShaders.empty();
  if (!relinkFromSnapshot) {
    attachCurrentShaders();
    if (!linked) {
      emitLinkSetup(0, setupCount);
      return;
    }
  }

  std::vector<GITSKey> temporaries;
  if (relinkFromSnapshot) {
    for (const auto& [type, source] : tracked->LinkedShaders) {
      const GITSKey shader = m_Recorder.CreateStateRestoreObjectKey();
      EmitShader(shader, type, source, true);
      attach(key, shader);
      temporaries.push_back(shader);
    }
  }
  emitLinkSetup(0, linkedSetupCount);
  GLint attributes = 0;
  GLint maxLength = 0;
  if (linked) {
    GL_FN(glGetProgramiv)(name, GL_ACTIVE_ATTRIBUTES, &attributes);
    GL_FN(glGetProgramiv)(name, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);
  }
  std::vector<GLchar> buffer(static_cast<size_t>(maxLength > 0 ? maxLength : 1));
  for (GLint i = 0; i < attributes; ++i) {
    GLsizei length = 0;
    GLint size = 0;
    GLenum type = 0;
    GL_FN(glGetActiveAttrib)
    (name, static_cast<GLuint>(i), static_cast<GLsizei>(buffer.size()), &length, &size, &type,
     buffer.data());
    const std::string attribute(buffer.data(), static_cast<size_t>(length));
    const GLint location = GL_FN(glGetAttribLocation)(name, attribute.c_str());
    if (IsBuiltIn(attribute) || location < 0) {
      continue;
    }
    glBindAttribLocationCommand command;
    command.m_program.Key = key;
    command.m_index.Value = static_cast<GLuint>(location);
    command.m_name = ArrayArgument<GLchar>(attribute.c_str(), NullTerminated);
    Emit<glBindAttribLocationCommandSerializer>(command);
  }

  glLinkProgramCommand link;
  link.m_program.Key = key;
  Emit<glLinkProgramCommandSerializer>(link);

  for (GITSKey shader : temporaries) {
    glDetachShaderCommand detach;
    detach.m_program.Key = key;
    detach.m_shader.Key = shader;
    Emit<glDetachShaderCommandSerializer>(detach);
    glDeleteShaderCommand remove;
    remove.m_shader.Key = shader;
    Emit<glDeleteShaderCommandSerializer>(remove);
  }
  emitLinkSetup(linkedSetupCount, setupCount);
  if (relinkFromSnapshot) {
    attachCurrentShaders();
  }
  if (linked) {
    RestoreUniforms(key, name);
  }
}

void StateRestorer::RestoreUniforms(GITSKey key, GLuint program) {
  glUseProgramCommand use;
  use.m_program.Key = key;
  Emit<glUseProgramCommandSerializer>(use);

  const std::unordered_map<GLint, std::vector<GLint>> captured =
      UniformLocationMapService::Get().CapturedLocations(key);
  const ProgramState* tracked = m_State.FindProgram(key);
  GLint uniforms = 0;
  GLint maxLength = 0;
  GL_FN(glGetProgramiv)(program, GL_ACTIVE_UNIFORMS, &uniforms);
  GL_FN(glGetProgramiv)(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);
  std::vector<GLchar> buffer(static_cast<size_t>(maxLength > 0 ? maxLength : 1));
  for (GLint i = 0; i < uniforms; ++i) {
    const auto index = static_cast<GLuint>(i);
    GLsizei length = 0;
    GLint size = 0;
    GLenum type = 0;
    GL_FN(glGetActiveUniform)
    (program, index, static_cast<GLsizei>(buffer.size()), &length, &size, &type, buffer.data());
    const std::string name(buffer.data(), static_cast<size_t>(length));
    if (IsBuiltIn(name) || type == GL_UNSIGNED_INT_ATOMIC_COUNTER) {
      continue;
    }
    if (m_Version.AtLeast(3, 1)) {
      GLint block = -1;
      GL_FN(glGetActiveUniformsiv)(program, 1, &index, GL_UNIFORM_BLOCK_INDEX, &block);
      if (block != -1) {
        continue;
      }
    }
    const bool array = name.ends_with("[0]");
    const std::string base = array ? name.substr(0, name.size() - 3) : name;
    for (GLint element = 0; element < size; ++element) {
      const std::string elementName = array ? base + "[" + std::to_string(element) + "]" : name;
      const GLint location = GL_FN(glGetUniformLocation)(program, elementName.c_str());
      if (location < 0) {
        continue;
      }
      // Every captured location the player resolves to this uniform gets a lookup, so commands
      // of the range resolve as they did.
      const auto it = captured.find(location);
      const std::vector<GLint> capturedLocations =
          it != captured.end() ? it->second : std::vector<GLint>{m_NextUnusedLocation++};
      const auto emitLookup = [&](GLint capturedLocation) {
        glGetUniformLocationCommand lookup;
        lookup.m_program.Key = key;
        lookup.m_name = ArrayArgument<GLchar>(elementName.c_str(), NullTerminated);
        lookup.m_Return.Value = capturedLocation;
        Emit<glGetUniformLocationCommandSerializer>(lookup);
      };
      const StoredCommand* handleSetter = nullptr;
      for (GLint capturedLocation : capturedLocations) {
        emitLookup(capturedLocation);
        if (tracked != nullptr) {
          if (const auto setter = tracked->HandleUniforms.find(capturedLocation);
              setter != tracked->HandleUniforms.end()) {
            handleSetter = setter->second.get();
          }
        }
      }
      // A handle set through a location the shader declares needs no lookup, so its setter is
      // found by the location the player resolved it to, and gets a lookup of its own.
      if (handleSetter == nullptr && tracked != nullptr) {
        for (const auto& [capturedLocation, setter] : tracked->HandleUniforms) {
          if (UniformLocationMapService::Get().ResolveForProgram(key, capturedLocation) ==
              location) {
            emitLookup(capturedLocation);
            handleSetter = setter.get();
            break;
          }
        }
      }
      if (handleSetter != nullptr) {
        handleSetter->Record(m_Recorder, m_Recorder.CreateStateRestoreKey(), m_ThreadId);
      } else {
        EmitUniform(program, location, capturedLocations.front(), type);
      }
    }
  }

  if (m_Version.AtLeast(3, 1)) {
    GLint blocks = 0;
    GL_FN(glGetProgramiv)(program, GL_ACTIVE_UNIFORM_BLOCKS, &blocks);
    for (GLint i = 0; i < blocks; ++i) {
      glUniformBlockBindingCommand command;
      command.m_program.Key = key;
      command.m_uniformBlockIndex.Value = static_cast<GLuint>(i);
      GLint binding = 0;
      GL_FN(glGetActiveUniformBlockiv)
      (program, static_cast<GLuint>(i), GL_UNIFORM_BLOCK_BINDING, &binding);
      command.m_uniformBlockBinding.Value = static_cast<GLuint>(binding);
      Emit<glUniformBlockBindingCommandSerializer>(command);
    }
  }
  if (m_Version.AtLeast(4, 3)) {
    GLint blocks = 0;
    GL_FN(glGetProgramInterfaceiv)(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &blocks);
    for (GLint i = 0; i < blocks; ++i) {
      const GLenum property = GL_BUFFER_BINDING;
      GLint binding = 0;
      GL_FN(glGetProgramResourceiv)
      (program, GL_SHADER_STORAGE_BLOCK, static_cast<GLuint>(i), 1, &property, 1, nullptr,
       &binding);
      glShaderStorageBlockBindingCommand command;
      command.m_program.Key = key;
      command.m_storageBlockIndex.Value = static_cast<GLuint>(i);
      command.m_storageBlockBinding.Value = static_cast<GLuint>(binding);
      Emit<glShaderStorageBlockBindingCommandSerializer>(command);
    }
  }

  // The context state is restored with no program in use, and RestoreBindings uses the recorded
  // one last.
  glUseProgramCommand unuse;
  Emit<glUseProgramCommandSerializer>(unuse);
}

// Subroutine selections belong to the context and glUseProgram resets them, so they are restored
// for the current program only.
void StateRestorer::RestoreSubroutines(GLuint program) {
  std::vector<GLenum> stages{GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER,
                             GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};
  if (m_Version.AtLeast(4, 3)) {
    stages.push_back(GL_COMPUTE_SHADER);
  }
  for (GLenum stage : stages) {
    // The locations query can fail for a stage the program has no shader for, which has no
    // subroutine uniforms.
    GLint uniforms = 0;
    GL_FN(glGetProgramStageiv)(program, stage, GL_ACTIVE_SUBROUTINE_UNIFORMS, &uniforms);
    if (uniforms <= 0) {
      continue;
    }
    GLint locations = 0;
    GL_FN(glGetProgramStageiv)(program, stage, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &locations);
    if (locations <= 0) {
      continue;
    }
    glUniformSubroutinesuivCommand command;
    command.m_shadertype.Value = stage;
    command.m_count.Value = locations;
    command.m_indices.HasValue = true;
    for (GLint location = 0; location < locations; ++location) {
      GLuint index = 0;
      GL_FN(glGetUniformSubroutineuiv)(stage, location, &index);
      command.m_indices.Data.push_back(index);
    }
    Emit<glUniformSubroutinesuivCommandSerializer>(command);
  }
}

void StateRestorer::EmitUniform(GLuint program,
                                GLint location,
                                GLint capturedLocation,
                                GLenum type) {
#define UNIFORM_CASE(glType, name, T, getter, components)                                          \
  case glType:                                                                                     \
    EmitUniformValues<name##CommandSerializer, name##Command>(                                     \
        capturedLocation, ReadUniform<T>(GL_FN(getter), program, location, components));           \
    break;

  switch (type) {
    UNIFORM_CASE(GL_FLOAT, glUniform1fv, GLfloat, glGetUniformfv, 1)
    UNIFORM_CASE(GL_FLOAT_VEC2, glUniform2fv, GLfloat, glGetUniformfv, 2)
    UNIFORM_CASE(GL_FLOAT_VEC3, glUniform3fv, GLfloat, glGetUniformfv, 3)
    UNIFORM_CASE(GL_FLOAT_VEC4, glUniform4fv, GLfloat, glGetUniformfv, 4)
    UNIFORM_CASE(GL_DOUBLE, glUniform1dv, GLdouble, glGetUniformdv, 1)
    UNIFORM_CASE(GL_DOUBLE_VEC2, glUniform2dv, GLdouble, glGetUniformdv, 2)
    UNIFORM_CASE(GL_DOUBLE_VEC3, glUniform3dv, GLdouble, glGetUniformdv, 3)
    UNIFORM_CASE(GL_DOUBLE_VEC4, glUniform4dv, GLdouble, glGetUniformdv, 4)
    UNIFORM_CASE(GL_INT_VEC2, glUniform2iv, GLint, glGetUniformiv, 2)
    UNIFORM_CASE(GL_INT_VEC3, glUniform3iv, GLint, glGetUniformiv, 3)
    UNIFORM_CASE(GL_INT_VEC4, glUniform4iv, GLint, glGetUniformiv, 4)
    UNIFORM_CASE(GL_BOOL_VEC2, glUniform2iv, GLint, glGetUniformiv, 2)
    UNIFORM_CASE(GL_BOOL_VEC3, glUniform3iv, GLint, glGetUniformiv, 3)
    UNIFORM_CASE(GL_BOOL_VEC4, glUniform4iv, GLint, glGetUniformiv, 4)
    UNIFORM_CASE(GL_UNSIGNED_INT, glUniform1uiv, GLuint, glGetUniformuiv, 1)
    UNIFORM_CASE(GL_UNSIGNED_INT_VEC2, glUniform2uiv, GLuint, glGetUniformuiv, 2)
    UNIFORM_CASE(GL_UNSIGNED_INT_VEC3, glUniform3uiv, GLuint, glGetUniformuiv, 3)
    UNIFORM_CASE(GL_UNSIGNED_INT_VEC4, glUniform4uiv, GLuint, glGetUniformuiv, 4)
    UNIFORM_CASE(GL_FLOAT_MAT2, glUniformMatrix2fv, GLfloat, glGetUniformfv, 4)
    UNIFORM_CASE(GL_FLOAT_MAT3, glUniformMatrix3fv, GLfloat, glGetUniformfv, 9)
    UNIFORM_CASE(GL_FLOAT_MAT4, glUniformMatrix4fv, GLfloat, glGetUniformfv, 16)
    UNIFORM_CASE(GL_FLOAT_MAT2x3, glUniformMatrix2x3fv, GLfloat, glGetUniformfv, 6)
    UNIFORM_CASE(GL_FLOAT_MAT2x4, glUniformMatrix2x4fv, GLfloat, glGetUniformfv, 8)
    UNIFORM_CASE(GL_FLOAT_MAT3x2, glUniformMatrix3x2fv, GLfloat, glGetUniformfv, 6)
    UNIFORM_CASE(GL_FLOAT_MAT3x4, glUniformMatrix3x4fv, GLfloat, glGetUniformfv, 12)
    UNIFORM_CASE(GL_FLOAT_MAT4x2, glUniformMatrix4x2fv, GLfloat, glGetUniformfv, 8)
    UNIFORM_CASE(GL_FLOAT_MAT4x3, glUniformMatrix4x3fv, GLfloat, glGetUniformfv, 12)
    UNIFORM_CASE(GL_DOUBLE_MAT2, glUniformMatrix2dv, GLdouble, glGetUniformdv, 4)
    UNIFORM_CASE(GL_DOUBLE_MAT3, glUniformMatrix3dv, GLdouble, glGetUniformdv, 9)
    UNIFORM_CASE(GL_DOUBLE_MAT4, glUniformMatrix4dv, GLdouble, glGetUniformdv, 16)
    UNIFORM_CASE(GL_DOUBLE_MAT2x3, glUniformMatrix2x3dv, GLdouble, glGetUniformdv, 6)
    UNIFORM_CASE(GL_DOUBLE_MAT2x4, glUniformMatrix2x4dv, GLdouble, glGetUniformdv, 8)
    UNIFORM_CASE(GL_DOUBLE_MAT3x2, glUniformMatrix3x2dv, GLdouble, glGetUniformdv, 6)
    UNIFORM_CASE(GL_DOUBLE_MAT3x4, glUniformMatrix3x4dv, GLdouble, glGetUniformdv, 12)
    UNIFORM_CASE(GL_DOUBLE_MAT4x2, glUniformMatrix4x2dv, GLdouble, glGetUniformdv, 8)
    UNIFORM_CASE(GL_DOUBLE_MAT4x3, glUniformMatrix4x3dv, GLdouble, glGetUniformdv, 12)
  default: // Scalar integers, booleans, samplers and images.
    EmitUniformValues<glUniform1ivCommandSerializer, glUniform1ivCommand>(
        capturedLocation, ReadUniform<GLint>(GL_FN(glGetUniformiv), program, location, 1));
    break;
  }
#undef UNIFORM_CASE
}

} // namespace gits::opengl2
