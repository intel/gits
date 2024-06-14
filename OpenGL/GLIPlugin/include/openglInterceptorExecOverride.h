// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openglInterceptorExecOverride.h
*
* @brief
*
*/

#pragma once

#include "gitsPluginPrePost.h"

namespace gits {
namespace OpenGL {

void execWrap_glBufferStorage(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) {
  GLbitfield flags_interceptor = flags;
  if (((flags & GL_MAP_PERSISTENT_BIT) &&
       CGitsPlugin::Configuration().opengl.recorder.coherentMapBehaviorWA) ||
      (flags & GL_MAP_COHERENT_BIT)) {
    flags_interceptor |= GL_MAP_READ_BIT;
  }
  CGitsPlugin::RecorderWrapper().Drivers().gl.glBufferStorage(
      target, size, data,
      flags_interceptor | CGitsPlugin::Configuration().opengl.recorder.bufferStorageFlagsMask);
}

GLenum execWrap_glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
  if (CGitsPlugin::Configuration().opengl.recorder.forceSyncFlushCommands) {
    flags |= GL_SYNC_FLUSH_COMMANDS_BIT;
  }

  return CGitsPlugin::RecorderWrapper().Drivers().gl.glClientWaitSync(sync, flags, timeout);
}

void execWrap_glGetIntegerv(GLenum pname, GLint* params) {
  CGitsPlugin::RecorderWrapper().Drivers().gl.glGetIntegerv(pname, params);

  // Replace OpenGL version.
  const auto& shared = CGitsPlugin::Configuration().opengl.shared;
  if (!shared.forceGLVersion.empty()) {
    switch (pname) {
    case GL_MAJOR_VERSION:
      *params = shared.forceGLVersionMajor;
      break;
    case GL_MINOR_VERSION:
      *params = shared.forceGLVersionMinor;
      break;
    }
  }
}

const GLubyte* execWrap_glGetString(GLenum name) {
  auto return_value = CGitsPlugin::RecorderWrapper().Drivers().gl.glGetString(name);

  // Replace OpenGL version.
  const std::string& forceGLVersion = CGitsPlugin::Configuration().opengl.shared.forceGLVersion;
  if (!forceGLVersion.empty() && name == GL_VERSION && return_value) {
    return_value = (const GLubyte*)forceGLVersion.c_str();
  }

  // Remove certain OpenGL extensions.
  const auto& suprExts = CGitsPlugin::Configuration().opengl.recorder.suppressExtensions;
  if (!suprExts.empty() && name == GL_EXTENSIONS && return_value) {
    static std::map<const GLubyte*, std::string> resultStrings;
    if (resultStrings.find(return_value) == resultStrings.end()) {
      std::string str((const char*)return_value);
      for (const auto& extStr : suprExts) {
        size_t pos = str.find(extStr);
        if (pos != std::string::npos) {
          Log(WARN) << "Suppressing extension " << extStr
                    << " by removing it from the available extensions string";
          if (pos != 0) {
            pos--; //also remove leading space
          }
          str.replace(pos, extStr.length() + 1, "");
        }
      }
      resultStrings[return_value] = std::move(str);
    }
    return_value = (const GLubyte*)resultStrings[return_value].c_str();
  }

  return return_value;
}

const GLubyte* execWrap_glGetStringi(GLenum name, GLuint index) {
  auto return_value = CGitsPlugin::RecorderWrapper().Drivers().gl.glGetStringi(name, index);

  const auto& suprExts = CGitsPlugin::Configuration().opengl.recorder.suppressExtensions;
  if (!suprExts.empty() && name == GL_EXTENSIONS && return_value) {
    if (std::find(suprExts.begin(), suprExts.end(), (const char*)return_value) != suprExts.end()) {
      Log(WARN) << "Suppressing extension " << return_value
                << " by returning a string GL_GITS_removed_extension";
      return_value = (const GLubyte*)"GL_GITS_removed_extension";
    }
  }

  return return_value;
}

void execWrap_glInsertEventMarkerEXT(GLsizei length, const GLchar* marker) {
  //CGitsPlugin::RecorderWrapper().Drivers().gl.glInsertEventMarkerEXT(length, marker);
}

void execWrap_glLabelObjectEXT(GLenum type, GLuint object, GLsizei length, const GLchar* label) {
  CGitsPlugin::RecorderWrapper().Drivers().gl.glLabelObjectEXT(type, object, length, label);
}

void* execWrap_glMapBufferRange(GLenum target,
                                GLintptr offset,
                                GLsizeiptr length,
                                GLbitfield access) {
  GLbitfield access_interceptor = access;
  if (((access & GL_MAP_PERSISTENT_BIT) &&
       CGitsPlugin::Configuration().opengl.recorder.coherentMapBehaviorWA) ||
      (access & GL_MAP_COHERENT_BIT)) {
    access_interceptor |= GL_MAP_READ_BIT;
    access_interceptor &= ~GL_MAP_UNSYNCHRONIZED_BIT;
  }
  return CGitsPlugin::RecorderWrapper().Drivers().gl.glMapBufferRange(
      target, offset, length,
      access_interceptor & CGitsPlugin::Configuration().opengl.recorder.bufferMapAccessMask);
}

void* execWrap_glMapBufferRangeEXT(GLenum target,
                                   GLintptr offset,
                                   GLsizeiptr length,
                                   GLbitfield access) {
  GLbitfield access_interceptor = access;
  if (((access & GL_MAP_PERSISTENT_BIT) &&
       CGitsPlugin::Configuration().opengl.recorder.coherentMapBehaviorWA) ||
      (access & GL_MAP_COHERENT_BIT)) {
    access_interceptor |= GL_MAP_READ_BIT;
    access_interceptor &= ~GL_MAP_UNSYNCHRONIZED_BIT;
  }
  return CGitsPlugin::RecorderWrapper().Drivers().gl.glMapBufferRangeEXT(
      target, offset, length,
      access_interceptor & CGitsPlugin::Configuration().opengl.recorder.bufferMapAccessMask);
}

void* execWrap_glMapNamedBufferRange(GLuint buffer,
                                     GLintptr offset,
                                     GLsizeiptr length,
                                     GLbitfield access) {
  GLbitfield access_interceptor = access;
  if (((access & GL_MAP_PERSISTENT_BIT) &&
       CGitsPlugin::Configuration().opengl.recorder.coherentMapBehaviorWA) ||
      (access & GL_MAP_COHERENT_BIT)) {
    access_interceptor |= GL_MAP_READ_BIT;
    access_interceptor &= ~GL_MAP_UNSYNCHRONIZED_BIT;
  }
  return CGitsPlugin::RecorderWrapper().Drivers().gl.glMapNamedBufferRange(
      buffer, offset, length,
      access_interceptor & CGitsPlugin::Configuration().opengl.recorder.bufferMapAccessMask);
}

void* execWrap_glMapNamedBufferRangeEXT(GLuint buffer,
                                        GLintptr offset,
                                        GLsizeiptr length,
                                        GLbitfield access) {
  GLbitfield access_interceptor = access;
  if (((access & GL_MAP_PERSISTENT_BIT) &&
       CGitsPlugin::Configuration().opengl.recorder.coherentMapBehaviorWA) ||
      (access & GL_MAP_COHERENT_BIT)) {
    access_interceptor |= GL_MAP_READ_BIT;
    access_interceptor &= ~GL_MAP_UNSYNCHRONIZED_BIT;
  }
  return CGitsPlugin::RecorderWrapper().Drivers().gl.glMapNamedBufferRangeEXT(
      buffer, offset, length,
      access_interceptor & CGitsPlugin::Configuration().opengl.recorder.bufferMapAccessMask);
}

void* execWrap_glMapTexture2DINTEL(
    GLuint texture, GLint level, GLbitfield access, GLint* stride, GLenum* layout) {
  // We override access to +read so that we can inspect the changes done to the mapping
  return CGitsPlugin::RecorderWrapper().Drivers().gl.glMapTexture2DINTEL(
      texture, level, access | GL_MAP_READ_BIT, stride, layout);
}

void execWrap_glNamedBufferStorage(GLuint buffer,
                                   GLsizeiptr size,
                                   const void* data,
                                   GLbitfield flags) {
  GLbitfield flags_interceptor = flags;
  if (((flags & GL_MAP_PERSISTENT_BIT) &&
       CGitsPlugin::Configuration().opengl.recorder.coherentMapBehaviorWA) ||
      (flags & GL_MAP_COHERENT_BIT)) {
    flags_interceptor |= GL_MAP_READ_BIT;
  }

  CGitsPlugin::RecorderWrapper().Drivers().gl.glNamedBufferStorage(
      buffer, size, data,
      flags_interceptor | CGitsPlugin::Configuration().opengl.recorder.bufferStorageFlagsMask);
}

void execWrap_glNamedBufferStorageEXT(GLuint buffer,
                                      GLsizeiptr size,
                                      const void* data,
                                      GLbitfield flags) {
  GLbitfield flags_interceptor = flags;
  if (((flags & GL_MAP_PERSISTENT_BIT) &&
       CGitsPlugin::Configuration().opengl.recorder.coherentMapBehaviorWA) ||
      (flags & GL_MAP_COHERENT_BIT)) {
    flags_interceptor |= GL_MAP_READ_BIT;
  }

  CGitsPlugin::RecorderWrapper().Drivers().gl.glNamedBufferStorageEXT(
      buffer, size, data,
      flags_interceptor | CGitsPlugin::Configuration().opengl.recorder.bufferStorageFlagsMask);
}

void execWrap_glPopGroupMarkerEXT() {
  //CGitsPlugin::RecorderWrapper().Drivers().gl.glPopGroupMarkerEXT();
}

void execWrap_glProgramBinary(GLuint program,
                              GLenum binaryFormat,
                              const void* binary,
                              GLsizei length) {
  if (CGitsPlugin::Configuration().opengl.recorder.suppressProgramBinary) {
    length = 1;
  }
  CGitsPlugin::RecorderWrapper().Drivers().gl.glProgramBinary(program, binaryFormat, binary,
                                                              length);
}

void execWrap_glProgramBinaryOES(GLuint program,
                                 GLenum binaryFormat,
                                 const void* binary,
                                 GLsizei length) {
  if (CGitsPlugin::Configuration().opengl.recorder.suppressProgramBinary) {
    length = 1;
  }
  CGitsPlugin::RecorderWrapper().Drivers().gl.glProgramBinaryOES(program, binaryFormat, binary,
                                                                 length);
}

void execWrap_glPushGroupMarkerEXT(GLsizei length, const GLchar* marker) {
  //CGitsPlugin::RecorderWrapper().Drivers().gl.glPushGroupMarkerEXT(length, marker);
}

void execWrap_glSamplePass(GLenum mode) {
  //CGitsPlugin::RecorderWrapper().Drivers().gl.glSamplePass(mode);
}

void execWrap_glShaderBinary(
    GLsizei count, const GLuint* shaders, GLenum binaryformat, const void* binary, GLsizei length) {
  if (CGitsPlugin::Configuration().opengl.recorder.suppressProgramBinary) {
    length = 1;
  }
  CGitsPlugin::RecorderWrapper().Drivers().gl.glShaderBinary(count, shaders, binaryformat, binary,
                                                             length);
}

} // namespace OpenGL
} // namespace gits
