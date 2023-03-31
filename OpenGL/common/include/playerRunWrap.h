// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   playerRunWrap.h
*
* @brief Manual overrides of OpenGL functions. Player will call these instead of calling the driver directly.
*
*/

#pragma once

#include "gits.h"
#include "openglFunction.h"
#include "stateTracking.h"
#include "ptblLibrary.h"
#include "playerRunWrapConditions.h"

namespace gits {
namespace OpenGL {

inline void glViewport_WRAPRUN(CGLint& x, CGLint& y, CGLsizei& width, CGLsizei& height) {
  // affectViewport option specifies the dimension of the window during
  // recording when used with forceWindowSize option. If specified we override
  // viewport dimension of only those calls which target the window as the
  // rendering surface. This is to avoid resizing rendering of other offscreen
  // objects (filtered based on viewport dimensions only). Note that we should in
  // practice resize this objects if we resize the main rendering surface but
  // since there is no easy way to do that we opt for this.
  bool overrideviewportsz = Config::Get().player.forceWindowSize
                                ? (Config::Get().player.affectViewport
                                       ? Config::Get().player.affectedViewport[0] == *width &&
                                             Config::Get().player.affectedViewport[1] == *height
                                       : true)
                                : false;
  if (!overrideviewportsz) {
    float scale = Config::Get().player.scaleFactor;
    drv.gl.glViewport(static_cast<GLint>(*x * scale), static_cast<GLint>(*y * scale),
                      static_cast<GLint>(*width * scale), static_cast<GLint>(*height * scale));
  } else {
    drv.gl.glViewport(0, 0, Config::Get().player.windowSize.first,
                      Config::Get().player.windowSize.second);
  }
}

inline void glProgramStringARB_WRAPRUN(CGLenum& target,
                                       CGLenum& format,
                                       CGLsizei& len,
                                       CShaderSource& string) {
  drv.gl.glProgramStringARB(*target, *format, (GLsizei)string.Text().size(), *string);
  if (ShouldLog(TRACE)) {
    std::string file_name = string.GetShaderFileName();
    Log(TRACE, NO_NEWLINE) << "code: " << file_name << "  ";
  }
}

inline void glScissor_WRAPRUN(CGLint& x, CGLint& y, CGLsizei& width, CGLsizei& height) {
  if (!Config::Get().player.forceWindowSize) {
    float scale = Config::Get().player.scaleFactor;
    drv.gl.glScissor(static_cast<GLint>(*x * scale), static_cast<GLint>(*y * scale),
                     static_cast<GLint>(*width * scale), static_cast<GLint>(*height * scale));
  } else {
    drv.gl.glScissor(0, 0, Config::Get().player.windowSize.first,
                     Config::Get().player.windowSize.second);
  }
}

inline void glGetSubroutineIndex_WRAPRUN(CGLuint& return_value,
                                         CGLProgram& program,
                                         CGLenum& shadertype,
                                         const CGLchar::CSArray& name) {
  GLuint actualSubIdx = drv.gl.glGetSubroutineIndex(*program, *shadertype, *name);
  if (actualSubIdx == GL_INVALID_INDEX) {
    Log(WARN) << "Invalid index found in glGetSubroutineIndex";
  } else {
    CGLSubroutineIndex::AddMapping(program.Original(), *shadertype, *return_value, actualSubIdx);
  }
}

inline void glGetUniformBlockIndex_WRAPRUN(CGLuint& return_value,
                                           CGLProgram& program,
                                           const CGLchar::CSArray& uniformBlockName) {
  GLuint currentIndex = drv.gl.glGetUniformBlockIndex(*program, *uniformBlockName);
  CGLUniformBlockIndex::AddMapping(program.Original(), *return_value, currentIndex);
}

inline void glGetProgramResourceIndex_WRAPRUN(CGLuint& return_value,
                                              CGLProgram& program,
                                              CGLenum& programInterface,
                                              const CGLchar::CSArray& name) {
  GLuint currentIndex = drv.gl.glGetProgramResourceIndex(*program, *programInterface, *name);
  if (*programInterface == GL_SHADER_STORAGE_BLOCK) {
    CGLStorageBlockIndex::AddMapping(program.Original(), *return_value, currentIndex);
  }
  if (*programInterface == GL_UNIFORM_BLOCK) {
    CGLUniformBlockIndex::AddMapping(program.Original(), *return_value, currentIndex);
  }
}

inline void glRenderbufferStorageMultisampleEXT_WRAPRUN(CGLenum& target,
                                                        CGLsizei& samples,
                                                        CGLenum& internalformat,
                                                        CGLsizei& width,
                                                        CGLsizei& height) {
  drv.gl.glRenderbufferStorageMultisampleEXT(
      *target, Config::Get().player.forceNoMSAA ? 1 : *samples, *internalformat, *width, *height);
}

inline void glRenderbufferStorageMultisample_WRAPRUN(CGLenum& target,
                                                     CGLsizei& samples,
                                                     CGLenum& internalformat,
                                                     CGLsizei& width,
                                                     CGLsizei& height) {
  drv.gl.glRenderbufferStorageMultisample(*target, Config::Get().player.forceNoMSAA ? 1 : *samples,
                                          *internalformat, *width, *height);
  glRenderbufferStorageMultisample_SD(*target, Config::Get().player.forceNoMSAA ? 1 : *samples,
                                      *internalformat, *width, *height);
}

inline void glRenderbufferStorageMultisampleANGLE_WRAPRUN(CGLenum& target,
                                                          CGLsizei& samples,
                                                          CGLenum& internalformat,
                                                          CGLsizei& width,
                                                          CGLsizei& height) {
  drv.gl.glRenderbufferStorageMultisampleANGLE(
      *target, Config::Get().player.forceNoMSAA ? 1 : *samples, *internalformat, *width, *height);
  glRenderbufferStorageMultisample_SD(*target, Config::Get().player.forceNoMSAA ? 1 : *samples,
                                      *internalformat, *width, *height);
}

inline void glRenderbufferStorageMultisampleIMG_WRAPRUN(
    CGLenum& arg0, CGLsizei& arg1, CGLenum& arg2, CGLsizei& arg3, CGLsizei& arg4) {
  drv.gl.glRenderbufferStorageMultisampleIMG(*arg0, Config::Get().player.forceNoMSAA ? 1 : *arg1,
                                             *arg2, *arg3, *arg4);
  glRenderbufferStorageMultisample_SD(*arg0, Config::Get().player.forceNoMSAA ? 1 : *arg1, *arg2,
                                      *arg3, *arg4);
}

inline void glUniformSubroutinesuiv_WRAPRUN(CGLenum& shadertype,
                                            CGLsizei& count,
                                            CGLuint::CSArray& indices) {
  static std::vector<GLuint> _indices;
  size_t _count = indices.Vector().size();
  _indices.resize(std::max<size_t>(_indices.size(), _count));

  GLint program = 0;
  drv.gl.glGetIntegerv(GL_CURRENT_PROGRAM, &program);

  for (size_t idx = 0; idx < _count; ++idx) {
    _indices[idx] = CGLSubroutineIndex::GetMapping(program, *shadertype, (*indices)[idx]);
  }

  drv.gl.glUniformSubroutinesuiv(*shadertype, (GLsizei)_count, &_indices[0]);
}

inline void glEGLImageTargetTexture2DOES_WRAPRUN(CGLenum& target, CEGLImageKHR& image) {
  // Images tend to be very delicate but not always actually necessary for stream playback.
  if (image.CheckMapping()) {
    drv.gl.glEGLImageTargetTexture2DOES(*target, *image);
  } else {
    Log(WARN) << "glEGLImageTargetTexture2DOES skipped due to not mapped eglImage (probably "
                 "eglImages are not supported on this system)";
  }
}

inline void glRenderbufferStorageMultisampleCoverageNV_WRAPRUN(CGLenum& target,
                                                               CGLsizei& coverageSamples,
                                                               CGLsizei& colorSamples,
                                                               CGLenum& internalformat,
                                                               CGLsizei& width,
                                                               CGLsizei& height) {
  drv.gl.glRenderbufferStorageMultisampleCoverageNV(
      *target, Config::Get().player.forceNoMSAA ? 0 : *coverageSamples,
      Config::Get().player.forceNoMSAA ? 0 : *colorSamples, *internalformat, *width, *height);
}

inline void glLinkProgram_WRAPRUN(CGLProgram& program) {
  OpenGL::CLibrary::Get().IncrementLinkProgramNumber();
  uint32_t linkNo = OpenGL::CLibrary::Get().GetLinkProgramNumber();

  if (Config::Get().player.linkUseProgBinary) {
    RestoreProgramBinary(*program, linkNo);
  } else {
    /*
        *  We rebind all the attributes in the shader here. This has to be done
        *  to correctly handle the case where application does not use explicit
        *  attribute locations nor does it use glBindAttribLocation itself.
        *  In such cases application has to depend on driver assigning locations
        *  (idices?) implicitly (and discover these with glGetAttribLocation)
        *  which means it does so in vendor dependent way.
        *  Simple tests at time of writing this show, that nvidia assigns locations
        *  in order of attributes delcaration, whereas intel in the order of
        *  attributes usage. Because matching attributes locations to actual
        *  data (glVertexAttribPointer index) is impossible in player (2 shaders
        *  refering to the same data in recorder may get different implicit locations
        *  for that attributes in player) we enforce recorder time locations
        *  by rebinding all shder attributes.
        *
        *  This is superflous for cases where glBindAttribLocation is used by
        *  the application, or explicit attrib location is used, but in no case
        *  will invalidate the stream.
        */

    drv.gl.glLinkProgram(*program);

    if (Config::Get().player.linkGetProgBinary) {
      SaveProgramBinary(*program, linkNo);
    }
  }

  glLinkProgram_SD(*program);
}

inline void glLinkProgramARB_WRAPRUN(CGLProgram& program) {
  OpenGL::CLibrary::Get().IncrementLinkProgramNumber();
  uint32_t linkNo = OpenGL::CLibrary::Get().GetLinkProgramNumber();

  if (Config::Get().player.linkUseProgBinary) {
    RestoreProgramBinary(*program, linkNo);
  } else {
    /*
        *  We rebind all the attributes in the shader here. This has to be done
        *  to correctly handle the case where application does not use explicit
        *  attribute locations nor does it use glBindAttribLocation itself.
        *  In such cases application has to depend on driver assigning locations
        *  (idices?) implicitly (and discover these with glGetAttribLocation)
        *  which means it does so in vendor dependent way.
        *  Simple tests at time of writing this show, that nvidia assigns locations
        *  in order of attributes delcaration, whereas intel in the order of
        *  attributes usage. Because matching attributes locations to actual
        *  data (glVertexAttribPointer index) is impossible in player (2 shaders
        *  refering to the same data in recorder may get different implicit locations
        *  for that attributes in player) we enforce recorder time locations
        *  by rebinding all shder attributes.
        *
        *  This is superflous for cases where glBindAttribLocation is used by
        *  the application, or explicit attrib location is used, but in no case
        *  will invalidate the stream.
        */

    drv.gl.glLinkProgramARB(*program);

    if (Config::Get().player.linkGetProgBinary) {
      SaveProgramBinary(*program, linkNo);
    }
  }

  glLinkProgram_SD(*program);
}

inline void glNormalPointerBounds_WRAPRUN(CGLenum& type,
                                          CGLsizei& stride,
                                          CAttribPtr& pointer,
                                          CGLsizei& count) {
  drv.gl.glNormalPointer(*type, *stride, *pointer);
  glNormalPointerBounds_SD(*type, *stride, *pointer, *count);
}

inline void glTexCoordPointerBounds_WRAPRUN(
    CGLint& size, CGLenum& type, CGLsizei& stride, CAttribPtr& pointer, CGLsizei& count) {
  drv.gl.glTexCoordPointer(*size, *type, *stride, *pointer);
  glTexCoordPointerBounds_SD(*size, *type, *stride, *pointer, *count);
}

inline void glColorPointerBounds_WRAPRUN(
    CGLint& size, CGLenum& type, CGLsizei& stride, CAttribPtr& pointer, CGLsizei& count) {
  drv.gl.glColorPointer(*size, *type, *stride, *pointer);
  glColorPointerBounds_SD(*size, *type, *stride, *pointer, *count);
}

inline void glVertexPointerBounds_WRAPRUN(
    CGLint& size, CGLenum& type, CGLsizei& stride, CAttribPtr& pointer, CGLsizei& count) {
  drv.gl.glVertexPointer(*size, *type, *stride, *pointer);
  glVertexPointerBounds_SD(*size, *type, *stride, *pointer, *count);
}
inline void glReadPixels_WRAPRUN(CGLint& x,
                                 CGLint& y,
                                 CGLsizei& width,
                                 CGLsizei& height,
                                 CGLenum& format,
                                 CGLenum& type,
                                 CGLvoid_ptr& pixels) {
  GLint buffer = 0;
  drv.gl.glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &buffer);

  GLboolean bufferBound = false;
  if (buffer != 0) {
    bufferBound = true;
  }

  if (bufferBound) {
    drv.gl.glReadPixels(*x, *y, *width, *height, *format, *type, *pixels);
  } else {
    // reserve space only if initialization is costly
    GLuint bufferSize = 0;
    static std::vector<unsigned char> dataSink;

    bufferSize = gits::OpenGL::getTexImageSize(*width, *height, 1, *format, *type);
    dataSink.resize(std::max<size_t>(dataSink.size(), bufferSize));

    drv.gl.glReadPixels(*x, *y, *width, *height, *format, *type, &dataSink[0]);
  }

  static unsigned read_pixels_num = 1;
  if (Config::Get().player.captureReadPixels[read_pixels_num]) {
    FrameBufferSave(read_pixels_num);
  }

  read_pixels_num++;
}
inline void glGetTexImage_WRAPRUN(
    CGLenum& target, CGLint& level, CGLenum& format, CGLenum& type, CGLvoid_ptr& pixels) {
  GLint buffer = 0;
  drv.gl.glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &buffer);
  GLboolean bufferBound = false;

  if (buffer != 0) {
    bufferBound = true;
  } else {
    bufferBound = false;
  }

  if (bufferBound) {
    drv.gl.glGetTexImage(*target, *level, *format, *type, *pixels);
  } else {
    GLsizei width = 0;
    GLsizei height = 0;
    GLsizei depth = 0;
    GLuint bufferSize = 0;
    auto& textureData = SD()._getTextureData;

    // Get dimensions
    drv.gl.glGetTexLevelParameteriv(*target, *level, GL_TEXTURE_WIDTH, &width);
    drv.gl.glGetTexLevelParameteriv(*target, *level, GL_TEXTURE_HEIGHT, &height);
    drv.gl.glGetTexLevelParameteriv(*target, *level, GL_TEXTURE_DEPTH, &depth);

    bufferSize = gits::OpenGL::getTexImageSize(width, height, depth, *format, *type);
    textureData.resize(std::max<size_t>(textureData.size(), bufferSize));

    drv.gl.glGetTexImage(*target, *level, *format, *type, &textureData[0]);
  }
}
inline void glGetTextureSubImage_WRAPRUN(CGLTexture& texture,
                                         CGLint& level,
                                         CGLint& xoffset,
                                         CGLint& yoffset,
                                         CGLint& zoffset,
                                         CGLsizei& width,
                                         CGLsizei& height,
                                         CGLsizei& depth,
                                         CGLenum& format,
                                         CGLenum& type,
                                         CGLsizei& bufSize,
                                         CGLvoid_ptr& pixels) {
  GLint buffer = 0;
  drv.gl.glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &buffer);
  GLboolean bufferBound = false;

  if (buffer != 0) {
    bufferBound = true;
  } else {
    bufferBound = false;
  }

  if (bufferBound) {
    drv.gl.glGetTextureSubImage(*texture, *level, *xoffset, *yoffset, *zoffset, *width, *height,
                                *depth, *format, *type, *bufSize, *pixels);
  } else {
    GLuint bufferSize = 0;
    auto& textureData = SD()._getTextureData;

    // Get dimensions
    bufferSize = gits::OpenGL::getTexImageSize(*width, *height, *depth, *format, *type);
    textureData.resize(std::max<size_t>(textureData.size(), bufferSize));

    drv.gl.glGetTextureSubImage(*texture, *level, *xoffset, *yoffset, *zoffset, *width, *height,
                                *depth, *format, *type, *bufSize, &textureData[0]);
  }
}
inline void glBegin_WRAPRUN(CFunction* ptr, CGLenum& mode) {
  drv.gl.glBegin(*mode);
  glBegin_SD(*mode);
}
inline void glEnd_WRAPRUN() {
  drv.gl.glEnd();
  glEnd_SD();
}

inline void glFinish_WRAPRUN() {
  drv.gl.glFinish();

  static unsigned finish_num = 1;
  if (Config::Get().player.captureFinishFrame[finish_num]) {
    FrameBufferSave(finish_num);
  }
  finish_num++;
}

inline void glFlush_WRAPRUN() {
  drv.gl.glFlush();

  static unsigned flush_num = 1;
  if (Config::Get().player.captureFlushFrame[flush_num]) {
    FrameBufferSave(flush_num);
  }
  flush_num++;
}

inline void glBindFramebuffer_WRAPRUN(CGLenum& target, CGLFramebuffer& framebuffer) {
  static unsigned bindfbo_num = 1;
  if (Config::Get().player.captureBindFboFrame[bindfbo_num]) {
    FrameBufferSave(bindfbo_num);
  }

  bindfbo_num++;
  drv.gl.glBindFramebuffer(*target, *framebuffer);
  glBindFramebuffer_SD(*target, *framebuffer);
}

inline void glDeleteFramebuffers_WRAPRUN(CGLsizei& n, CGLFramebuffer::CSUnmapArray& framebuffers) {
  drv.gl.glDeleteFramebuffers(*n, *framebuffers);
  glDeleteFramebuffers_SD(*n, *framebuffers);
  framebuffers.RemoveMapping();
}

inline void glDeleteProgram_WRAPRUN(CGLProgram& program) {
  CGLUniformLocation::RemoveMappings(program.Original());

  if (ConditionCurrentContextZero()) {
    drv.gl.glDeleteProgram(*program);
  }
  glDeleteProgram_SD(*program);
}

namespace {
void HandleUniformLocationMapping(const CRecUniformLocation& retVal,
                                  GLint actualLocation,
                                  CGLProgram& program,
                                  const GLchar* name) {
  // Location -1 will be mapped the same way as other locations,
  // not present in mapping structures - using identity functions
  if (retVal.Location() == -1) {
    return;
  }

  // We assume that there are no stream that during recording had optimized
  // all but the very first uniform array element - we assume that all uniform
  // arrays are at least 2 elements. Whole inactive arrays are also not handled
  // well ...
  if (retVal.ArraySize() == 1) {
    CGLUniformLocation::AddMapping(program.Original(), retVal.Location(), 1, actualLocation);
  } else {
    // Player array index location may be -1 if we have a smaller array in
    // player than in recorder. In this case evaluation and mapping of runtime
    // array is impossible. We return there. Hopefully there should be at least
    // one location queried which is valid for player array size.
    if (actualLocation == -1) {
      return;
    }

    // For arrays we need to check if some part of it are inactive or is there
    // more. active parts than there were during recording. To do so, we can
    // compare array size against runtime array size. Inactive parts will be
    // mapped to -1, while active to correct value.
    GLint array_size, offset;
    GetUniformArraySizeAndOffset(*program, name, actualLocation, array_size, offset);

    auto arrayOrigLocation = retVal.Location() - retVal.ArrayIndex();
    auto arrayCurrLocation = actualLocation - retVal.ArrayIndex();

    // Add common part of arrays.
    CGLUniformLocation::AddMapping(program.Original(), arrayOrigLocation,
                                   std::min(array_size, retVal.ArraySize()), arrayCurrLocation);

    if (retVal.ArraySize() > array_size) {
      // We had bigger array at record time, map rest of the array to -1,
      // so that setting those elements doesn't interfere with other uniforms.
      auto rest = retVal.ArraySize() - array_size;
      CGLUniformLocation::AddMapping(program.Original(), arrayOrigLocation + array_size, rest, -1);
    }

    if (retVal.ArraySize() < array_size) {
      // We have bigger array now (this means some uniforms didn't get optimized
      // away, but are unused in the shader). This can cause problems, if
      // application was setting values of optimized away uniforms in original
      // stream by computing their locations.
      CALL_ONCE[] {
        Log(WARN) << "Uniform array size during recording was smaller than it is now - corruptions "
                     "may appear"
                     " if application was setting inactive uniform array elements";
      };
    }
  }
}
} // namespace

inline void glGetUniformLocation_WRAPRUN(const CRecUniformLocation& retVal,
                                         CGLProgram& program,
                                         CGLchar::CSArray& name) {
  GLint actualLocation = drv.gl.glGetUniformLocation(*program, *name);

  HandleUniformLocationMapping(retVal, actualLocation, program, *name);
}

inline void glGetUniformLocationARB_WRAPRUN(CRecUniformLocation& retVal,
                                            CGLProgram& program,
                                            CGLchar::CSArray& name) {
  GLint actualLocation = drv.gl.glGetUniformLocationARB(*program, *name);

  HandleUniformLocationMapping(retVal, actualLocation, program, *name);
}

inline void glGetProgramResourceiv_WRAPRUN(CGLProgram& program,
                                           CGLenum& programInterface,
                                           CGLResourceIndex& index,
                                           CGLsizei& propCount,
                                           CGLenum::CSArray& props,
                                           CGLsizei& bufSize,
                                           CGLsizei::CSArray& length,
                                           CGLProgramResourceivHelper& params) {
  GLsizei totalPropsCount = std::min(*propCount, *bufSize);
  std::vector<GLint> actualParams(totalPropsCount);
  // TODO: This is a bug, we are asking the driver to write `bufSize` data into
  // vec that might be of size `propCount`.
  drv.gl.glGetProgramResourceiv(*program, *programInterface, *index, *propCount, *props, *bufSize,
                                nullptr, actualParams.data());

  if (GL_UNIFORM == *programInterface) {
    for (GLsizei i = 0; i < totalPropsCount; ++i) {
      if (GL_LOCATION == (*props)[i]) {
        // TODO: Do we assume here that actualParams[i] corresponds to
        // Locations()[i]? It might not, as not every param is 1 GLint long.
        HandleUniformLocationMapping(params.Locations()[i], actualParams[i], program,
                                     index.ResourceName());
      }
    }
  }
}

inline void glGetProgramResourceLocation_WRAPRUN(CRecUniformLocation& retVal,
                                                 CGLProgram& program,
                                                 CGLenum& programInterface,
                                                 CGLchar::CSArray& name) {
  if (GL_UNIFORM == *programInterface) {
    GLint actualLocation = drv.gl.glGetProgramResourceLocation(*program, *programInterface, *name);

    HandleUniformLocationMapping(retVal, actualLocation, program, *name);
  }
}

inline void glTexImage2D_WRAPRUN(CGLenum& target,
                                 CGLint& level,
                                 CGLint& internalformat,
                                 CGLsizei& width,
                                 CGLsizei& height,
                                 CGLint& border,
                                 CGLenum& format,
                                 CGLenum& type,
                                 CGLTexResource& pixels) {
  drv.gl.glTexImage2D(*target, *level, *internalformat, *width, *height, *border, *format, *type,
                      *pixels);

  glTexImage2D_SD(*target, *level, *internalformat, *width, *height, *border, *format, *type,
                  *pixels);

  static long texNum = 1;
  if (Config::Get().player.capture2DTexs[texNum]) {
    GLint boundTex = BoundTexture(*target);
    capture_bound_texture2D(*target, GetPathForImageDumping(),
                            "tex2D_" + std::to_string(boundTex) + "_" + std::to_string(texNum));
  }
  texNum++;
}

inline void glTexImage2DMultisample_WRAPRUN(CGLenum& target,
                                            CGLsizei& samples,
                                            CGLint& internalformat,
                                            CGLsizei& width,
                                            CGLsizei& height,
                                            CGLboolean& fixedsamplelocations) {
  drv.gl.glTexImage2DMultisample(*target, Config::Get().player.forceNoMSAA ? 1 : *samples,
                                 *internalformat, *width, *height, *fixedsamplelocations);
  glTexImage2DMultisample_SD(*target, Config::Get().player.forceNoMSAA ? 1 : *samples,
                             *internalformat, *width, *height, *fixedsamplelocations);
}

inline void glShaderSource_WRAPRUN(CGLProgram& shader,
                                   CGLsizei& count,
                                   CShaderSource& string,
                                   CGLintptrZero& length) {
  drv.gl.glShaderSource(*shader, *count, *string, *length);
  glShaderSource_SD(*shader, *count, *string, *length);
  if (ShouldLog(TRACE)) {
    std::string file_name = string.GetShaderFileName();
    Log(TRACE, NO_NEWLINE) << "code: " << file_name << "  ";
    if (*shader != 0 && ShouldLog(TRACEV)) {
      SD().GetCurrentSharedStateData().GLSLShaders().Get(*shader)->SetShaderName(file_name);
    }
  }
}
inline void glShaderSourceARB_WRAPRUN(CGLProgram& shaderObj,
                                      CGLsizei& count,
                                      CShaderSource& string,
                                      CGLintptrZero& length) {
  drv.gl.glShaderSourceARB(*shaderObj, *count, *string, *length);
  glShaderSource_SD(*shaderObj, *count, *string, *length);
  if (ShouldLog(TRACE)) {
    std::string file_name = string.GetShaderFileName();
    Log(TRACE, NO_NEWLINE) << "code: " << file_name << "  ";
    if (*shaderObj != 0 && ShouldLog(TRACEV)) {
      SD().GetCurrentSharedStateData().GLSLShaders().Get(*shaderObj)->SetShaderName(file_name);
    }
  }
}

inline void glCreateShaderProgramv_WRAPRUN(CGLProgram& return_value,
                                           CGLenum& type,
                                           CGLsizei& count,
                                           CShaderSource& strings) {
  return_value.Assign(drv.gl.glCreateShaderProgramv(*type, 1, *strings));
  glCreateShaderProgramv_SD(*return_value, *type, 1, *strings);
  if (ShouldLog(TRACE)) {
    std::string file_name = strings.GetShaderFileName();
    Log(TRACE, NO_NEWLINE) << "code: " << file_name << "  ";
  }
}
inline void glCreateShaderProgramvEXT_WRAPRUN(CGLProgram& return_value,
                                              CGLenum& type,
                                              CGLsizei& count,
                                              CShaderSource& strings) {
  return_value.Assign(drv.gl.glCreateShaderProgramvEXT(*type, 1, *strings));
  glCreateShaderProgramv_SD(*return_value, *type, 1, *strings);
  if (ShouldLog(TRACE)) {
    std::string file_name = strings.GetShaderFileName();
    Log(TRACE, NO_NEWLINE) << "code: " << file_name << "  ";
  }
}
inline void glGetCompressedTexImage_WRAPRUN(CGLenum& target, CGLint& level, CGLvoid_ptr& img) {
  GLint buffer = 0;
  drv.gl.glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &buffer);
  GLboolean bufferBound = false;

  if (buffer != 0) {
    bufferBound = true;
  } else {
    bufferBound = false;
  }

  if (bufferBound) {
    drv.gl.glGetCompressedTexImage(*target, *level, *img);
  } else {
    GLint imageSize = 0;
    auto& textureData = SD()._getTextureData;

    drv.gl.glGetTexLevelParameteriv(*target, *level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &imageSize);
    textureData.resize(std::max<size_t>(textureData.size(), imageSize));
    drv.gl.glGetCompressedTexImage(*target, *level, &textureData[0]);
  }
}

inline void glClientWaitSync_WRAPRUN(CGLenum& return_value,
                                     CGLsync& sync,
                                     CGLbitfield& flags,
                                     CGLuint64& timeout) {
  GLuint64 MAX_TIMEOUT = 100000000ull;
  GLenum recRetVal = *return_value;
  GLuint64 wait = *timeout;
  if (wait > MAX_TIMEOUT) {
    Log(WARN) << "Timeout provided for glClientWaitSync is ridiculously large. Clamping it to 100 "
                 "milliseconds.";
    wait = MAX_TIMEOUT;
  }
  GLenum playRetVal = drv.gl.glClientWaitSync(*sync, *flags, wait);
  if ((playRetVal == GL_TIMEOUT_EXPIRED || playRetVal == GL_WAIT_FAILED) &&
      (playRetVal != recRetVal)) {
    Log(TRACE) << "Calling additional glClientWaitSync, because original return different value as "
                  "in recorder.";
    playRetVal = drv.gl.glClientWaitSync(*sync, 0, 10 * MAX_TIMEOUT);
  }
}
} // namespace OpenGL
} // namespace gits
