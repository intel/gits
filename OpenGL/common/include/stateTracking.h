// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   stateTracking.h
*
* @brief Automatically generated declarations of OpenGL library simple function call wrappers.
*
*/

#pragma once

#include "gits.h"
#include "log.h"
#include "openglFunction.h"
#include "clientArrays.h"
#include "eglArguments.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "stateObjects.h"

namespace gits {
namespace OpenGL {
GLboolean checkBuff();
void setTargetForBuff(GLenum target, GLuint buffer);
GLint boundBuff(GLenum target);
void updateIndexedTargetInfo(
    GLenum target, GLuint index, GLuint buffer, GLintptr offset = 0, GLsizeiptr size = 0);
void ESCompressedTexDataTrack(GLenum target, GLint level, GLsizei imagesize, const GLvoid* data);
void stateObjAddFBOAttachent(GLenum target, GLenum attachment, GLint fbo = -1);
void stateObjRemoveFBOAttachment(GLenum target, GLenum attachment, GLint fbo = -1);
void stateObjAddFBOEXTAttachent(GLenum target, GLenum attachment);
void stateObjRemoveFBOEXTAttachent(GLenum target, GLenum attachment);
void OptionalBufferDataTrack(
    GLint buffer, GLintptr offset, GLsizeiptr size, const GLvoid* data, bool recording);
void attributeIndexTrack(GLuint index);
std::vector<uint8_t> GetBufferData(GLenum target, GLintptr offset, GLsizeiptr size);
std::vector<uint8_t> GetNamedBufferData(GLuint buffer, GLintptr offset, GLsizeiptr size);

inline void glColorPointerBounds_SD(GLint size,
                                    GLenum type,
                                    GLsizei stride,
                                    const GLvoid* pointer,
                                    GLsizei count,
                                    GLboolean recording = 0) {
  SD().GetCurrentContextStateData().ClientArrays().ColorArray().Params(checkBuff(), size, type,
                                                                       stride, pointer);
  SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);
}

inline void glVertexPointerBounds_SD(GLint size,
                                     GLenum type,
                                     GLsizei stride,
                                     const GLvoid* pointer,
                                     GLsizei count,
                                     GLboolean recording = 0) {
  SD().GetCurrentContextStateData().ClientArrays().VertexArray().Params(checkBuff(), size, type,
                                                                        stride, pointer);
  SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);
}

inline void glNormalPointerBounds_SD(
    GLenum type, GLsizei stride, const GLvoid* pointer, GLsizei count, GLboolean recording = 0) {
  SD().GetCurrentContextStateData().ClientArrays().NormalArray().Params(checkBuff(), 3, type,
                                                                        stride, pointer);
  SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);
}

inline void glTexCoordPointerBounds_SD(GLint size,
                                       GLenum type,
                                       GLsizei stride,
                                       const GLvoid* pointer,
                                       GLsizei count,
                                       GLboolean recording = 0) {
  GLint activeTexture =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.clientActiveTexture;
  SD().GetCurrentContextStateData()
      .ClientArrays()
      .TexCoordArray(activeTexture)
      .Params(checkBuff(), size, type, stride, pointer);
}

inline void glBindProgramPipeline_SD(GLuint pipeline, GLboolean recording = 0) {
  auto version = SD().GetCurrentContextStateData().Version();
  if ((curctx::IsOgl()) && (version >= 400)) {
    GLint program = 0;
    drv.gl.glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program == 0) {
      program = 0;
      drv.gl.glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &program);
      if (program != 0) {
        SD().GetCurrentContextStateData().Bindings().GLSLProgram(program);
      }
    }
  }
  SD().GetCurrentContextStateData().Bindings().GLSLPipeline(pipeline);
}

inline void glActiveTexture_SD(GLenum texture, GLboolean recording = 0) {
  SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.activeTexture = texture;
}

inline void glBufferData_SD(
    GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage, GLboolean recording = 0) {
  if (size < 0 || target == 0) {
    return;
  }

  GLint buffer = boundBuff(target);
  if (Config::Get().IsRecorder()) {
    OptionalBufferDataTrack(buffer, 0, size, data, recording);
  }

  if (buffer > 0) {
    CBufferStateObj* bufferState = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferState != nullptr) {
      bufferState->SizeSet(ensure_signed32bit_representible<GLsizeiptrARB>(size));
      bufferState->UsageSet(usage);
    }
  }
}

inline void glBufferStorage_SD(
    GLenum target, GLsizeiptr size, const GLvoid* data, GLbitfield flags, GLboolean recording = 0) {
  flags = flags | Config::Get().opengl.recorder.bufferStorageFlagsMask;
  if (size < 0 || target == 0) {
    return;
  }
  if ((flags & GL_MAP_COHERENT_BIT) ||
      ((flags & GL_MAP_PERSISTENT_BIT) && Config::Get().opengl.recorder.coherentMapBehaviorWA)) {
    SD().GetCurrentSharedStateData().coherentBufferMapping = true;
  }

  GLint buffer = boundBuff(target);
  if (Config::Get().IsRecorder()) {
    OptionalBufferDataTrack(buffer, 0, size, data, recording);
  }

  if (buffer > 0) {
    CBufferStateObj* bufferState = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferState != nullptr) {
      bufferState->SizeSet(ensure_signed32bit_representible<GLsizeiptrARB>(size));
      bufferState->FlagsSet(flags);
    }
  }
}

inline void glUseProgramObjectARB_SD(GLhandleARB programObj, GLboolean recording = 0) {
  SD().GetCurrentContextStateData().Bindings().GLSLProgram(programObj);
}

inline void glBegin_SD(GLenum mode, GLboolean recording = 0) {
  SD().GetCurrentContextStateData().glBeginState = true;
}

inline void glBindBuffer_SD(GLenum target, GLuint buffer, GLboolean recording = 0) {
  setTargetForBuff(target, buffer);
  SD().GetCurrentContextStateData().Bindings().BoundBuffers()[target] = (GLint)buffer;
}

inline void glBindFramebuffer_SD(GLenum target, GLuint framebuffer, GLboolean recording = 0) {
  CFramebufferStateObj* framebufferStateObj =
      SD().GetCurrentContextStateData().Framebuffers().Get(framebuffer);

  // In ES2.0+ BindFramebuffer takes just unused fbo to create a new one (it
  // does not have to be generated previously).
  if (curctx::IsEs2Plus() && framebufferStateObj == 0 && framebuffer != 0) {
    SD().GetCurrentContextStateData().Framebuffers().Generate(1, &framebuffer);
    framebufferStateObj = SD().GetCurrentContextStateData().Framebuffers().Get(framebuffer);
  }

  if ((framebuffer != 0) && (framebufferStateObj != 0)) {
    framebufferStateObj->SetTarget(target);
  }
}

inline void glClientActiveTexture_SD(GLenum texture, GLboolean recording = 0) {
  SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.clientActiveTexture =
      texture;
}

inline void glDeleteBuffers_SD(GLsizei n, const GLuint buffers[], GLboolean recording = 0) {
  if (SD().GetCurrentContext() == 0) {
    return;
  }

  // Remove deleted buffers from bindings
  auto& boundBuffers = SD().GetCurrentContextStateData().Bindings().BoundBuffers();
  for (std::unordered_map<GLenum, GLint>::iterator it = boundBuffers.begin();
       it != boundBuffers.end(); ++it) {
    if (it->second != 0) {
      for (GLsizei i = 0; i < n; ++i) {
        if (it->second == (GLint)buffers[i]) {
          it->second = 0;
          break;
        }
      }
    }
  }

  SD().GetCurrentSharedStateData().Buffers().Remove((unsigned)n, buffers);
}

inline void glDeleteFramebuffers_SD(GLsizei n,
                                    const GLuint framebuffers[],
                                    GLboolean recording = 0) {
  SD().GetCurrentContextStateData().Framebuffers().Remove(unsigned(n), framebuffers);
}

inline void glDisableClientState_SD(GLenum array, GLboolean recording = 0) {
  CClientArraysStateObj& arrays = SD().GetCurrentContextStateData().ClientArrays();

  switch (array) {
  case GL_VERTEX_ARRAY:
    arrays.VertexArray().Enabled(false);
    break;
  case GL_COLOR_ARRAY:
    arrays.ColorArray().Enabled(false);
    break;
  case GL_NORMAL_ARRAY:
    arrays.NormalArray().Enabled(false);
    break;
  case GL_TEXTURE_COORD_ARRAY: {
    if (!curctx::IsOgl11()) {
      GLint clientActiveTexture = -1;
      drv.gl.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &clientActiveTexture);
      arrays.TexCoordArray(clientActiveTexture).Enabled(false);
    }
  } break;
  case GL_SECONDARY_COLOR_ARRAY:
    arrays.SecondaryColorArray().Enabled(false);
    break;
  default:
    break;
  }
}

inline void glDisableClientStateIndexedEXT_SD(GLenum array, GLuint index, GLboolean recording = 0) {
  if (array == GL_TEXTURE_COORD_ARRAY) {
    SD().GetCurrentContextStateData()
        .ClientArrays()
        .TexCoordArray(GL_TEXTURE0 + index)
        .Enabled(false);
  }
}

inline void glEnableClientState_SD(GLenum array, GLboolean recording = 0) {
  CClientArraysStateObj& arrays = SD().GetCurrentContextStateData().ClientArrays();

  switch (array) {
  case GL_VERTEX_ARRAY:
    arrays.VertexArray().Enabled(true);
    break;
  case GL_COLOR_ARRAY:
    arrays.ColorArray().Enabled(true);
    break;
  case GL_NORMAL_ARRAY:
    arrays.NormalArray().Enabled(true);
    break;
  case GL_TEXTURE_COORD_ARRAY: {
    GLint clientActiveTexture = -1;
    drv.gl.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &clientActiveTexture);
    arrays.TexCoordArray(clientActiveTexture).Enabled(true);
  } break;
  case GL_SECONDARY_COLOR_ARRAY:
    arrays.SecondaryColorArray().Enabled(true);
    break;
  default:
    break;
  }
}

inline void glEnableClientStateIndexedEXT_SD(GLenum array, GLuint index, GLboolean recording = 0) {
  if (array == GL_TEXTURE_COORD_ARRAY) {
    SD().GetCurrentContextStateData()
        .ClientArrays()
        .TexCoordArray(GL_TEXTURE0 + index)
        .Enabled(true);
  }
}

inline void glEnd_SD(GLboolean recording = 0) {
  SD().GetCurrentContextStateData().glBeginState = false;
}

inline void glGenFramebuffers_SD(GLsizei n, GLuint framebuffers[], GLboolean recording = 0) {
  SD().GetCurrentContextStateData().Framebuffers().Generate((unsigned)n, framebuffers);
}

inline void glCreateFramebuffers_SD(GLsizei n, GLuint framebuffers[], GLboolean recording = 0) {
  SD().GetCurrentContextStateData().Framebuffers().Generate((unsigned)n, framebuffers);
}

inline void glUseProgram_SD(GLuint program, GLboolean recording = 0) {
  SD().GetCurrentContextStateData().Bindings().GLSLProgram(program);
}

inline void glVertexAttribDivisor_SD(GLuint index, GLuint divisor, GLboolean recording = 0) {
  SD().GetCurrentContextStateData().ClientArrays().VertexAttribArray(index).Data().track.divisor =
      divisor;
}

inline void glColorPointer_SD(
    GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, GLboolean recording = 0) {
  SD().GetCurrentContextStateData().ClientArrays().ColorArray().Params(checkBuff(), size, type,
                                                                       stride, pointer);
  SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);
}

inline void glInterleavedArrays_SD(GLenum format,
                                   GLsizei stride,
                                   const GLvoid* pointer,
                                   GLboolean recording = 0) {
  SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Params(checkBuff(), 1, format,
                                                                             stride, pointer);
  SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(true);
}

inline void glNormalPointer_SD(GLenum type,
                               GLsizei stride,
                               const GLvoid* pointer,
                               GLboolean recording = 0) {
  SD().GetCurrentContextStateData().ClientArrays().NormalArray().Params(checkBuff(), 3, type,
                                                                        stride, pointer);
  SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);
}

inline void glSecondaryColorPointer_SD(
    GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, GLboolean recording = 0) {
  SD().GetCurrentContextStateData().ClientArrays().SecondaryColorArray().Params(
      checkBuff(), size, type, stride, pointer);
  SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);
}

inline void glTexCoordPointer_SD(
    GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, GLboolean recording = 0) {
  GLint activeTexture =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.clientActiveTexture;
  SD().GetCurrentContextStateData()
      .ClientArrays()
      .TexCoordArray(activeTexture)
      .Params(checkBuff(), size, type, stride, pointer);
}

inline void glMultiTexCoordPointerEXT_SD(GLenum texunit,
                                         GLint size,
                                         GLenum type,
                                         GLsizei stride,
                                         const GLvoid* pointer,
                                         GLboolean recording = 0) {
  SD().GetCurrentContextStateData().ClientArrays().TexCoordArray(texunit).Params(
      checkBuff(), size, type, stride, pointer);
}

inline void glVertexAttribIPointer_SD(GLuint index,
                                      GLint size,
                                      GLenum type,
                                      GLsizei stride,
                                      const GLvoid* pointer,
                                      GLboolean recording = 0) {
  SD().GetCurrentContextStateData().ClientArrays().VertexAttribArray(index).Params(
      checkBuff(), size, type, stride, pointer);
  SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);

  attributeIndexTrack(index);
}

inline void glVertexAttribPointer_SD(GLuint index,
                                     GLint size,
                                     GLenum type,
                                     GLboolean normalized,
                                     GLsizei stride,
                                     const GLvoid* pointer,
                                     GLboolean recording = 0) {
  SD().GetCurrentContextStateData().ClientArrays().VertexAttribArray(index).Params(
      checkBuff(), size, type, stride, pointer);
  SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);

  attributeIndexTrack(index);
}

inline void glVertexPointer_SD(
    GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, GLboolean recording = 0) {
  SD().GetCurrentContextStateData().ClientArrays().VertexArray().Params(checkBuff(), size, type,
                                                                        stride, pointer);
  SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);
}

inline void glUnmapTexture2DINTEL_SD(GLuint texture, GLint level, GLboolean recording = 0) {
  SD().GetCurrentSharedStateData().GetMappedTextures().RemoveTexture(texture, level);
  SD().GetCurrentSharedStateData().GetMappedTextures().UnmapTexture(texture, level);
}

inline void glPrimitiveRestartIndex_SD(GLuint index, GLboolean recording = 0) {
  SD().GetCurrentContextStateData().restartIndexValue = index;
}

inline void glPushClientAttrib_SD(GLbitfield mask, GLboolean recording = 0) {
  SD().GetCurrentContextStateData().pushUsed = true;
}

inline void glBindFramebufferEXT_SD(GLenum target, GLuint framebuffer, GLboolean recording = 0) {
  if (framebuffer != 0) {
    SD().GetCurrentContextStateData().FramebuffersEXT().Add(
        CFramebufferStateObj(framebuffer, target));
  }
}

inline void glDeleteFramebuffersEXT_SD(GLsizei n,
                                       const GLuint* framebuffers,
                                       GLboolean recording = 0) {
  SD().GetCurrentContextStateData().FramebuffersEXT().Remove(unsigned(n), framebuffers);
}

inline void glNamedBufferData_SD(
    GLuint buffer, GLsizeiptr size, const GLvoid* data, GLenum usage, GLboolean recording = 0) {
  if (size < 0) {
    return;
  }
  // We are saving GL_ARRAY_BUFFER as default, it should be changed with the
  // first call to glBindBuffer before any drawings
  setTargetForBuff(GL_ARRAY_BUFFER, buffer);
  if (Config::Get().IsRecorder()) {
    OptionalBufferDataTrack(buffer, 0, size, data, recording);
  }

  if (buffer > 0) {
    CBufferStateObj* bufferState = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferState != nullptr) {
      bufferState->SizeSet(ensure_signed32bit_representible<GLsizeiptrARB>(size));
      bufferState->UsageSet(usage);
    }
  }
}

inline void glNamedBufferStorage_SD(
    GLuint buffer, GLsizeiptr size, const GLvoid* data, GLbitfield flags, GLboolean recording = 0) {
  flags = flags | Config::Get().opengl.recorder.bufferStorageFlagsMask;
  if (size < 0) {
    return;
  }
  if ((flags & GL_MAP_COHERENT_BIT) ||
      ((flags & GL_MAP_PERSISTENT_BIT) && Config::Get().opengl.recorder.coherentMapBehaviorWA)) {
    SD().GetCurrentSharedStateData().coherentBufferMapping = true;
  }
  // We are saving GL_ARRAY_BUFFER as default, it should be changed with the
  // first call to glBindBuffer before any drawings
  setTargetForBuff(GL_ARRAY_BUFFER, buffer);
  if (Config::Get().IsRecorder()) {
    OptionalBufferDataTrack(buffer, 0, size, data, recording);
  }

  if (buffer > 0) {
    CBufferStateObj* bufferState = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferState != nullptr) {
      bufferState->SizeSet(ensure_signed32bit_representible<GLsizeiptrARB>(size));
      bufferState->FlagsSet(flags);
    }
  }
}

inline void glBindTexture_SD(GLenum target, GLuint texture, GLboolean recording = 0) {
  if (SD().GetCurrentContext() == 0) {
    return;
  }

  if (isTrackTextureBindingWAUsed()) {
    auto unit =
        SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.activeTexture;
    SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.boundTextures[unit]
                                                                                        [target] =
        texture;
  }

  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();
  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    SetTargetForTexture(target, texture);
  }
}

inline void glBindMultiTextureEXT_SD(GLenum texunit,
                                     GLenum target,
                                     GLuint texture,
                                     GLboolean recording = 0) {
  if (isTrackTextureBindingWAUsed()) {
    SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.boundTextures[texunit]
                                                                                        [target] =
        texture;
  }

  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    SetTargetForTexture(target, texture);
  }
}

inline void glTexStorage1D_SD(
    GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (!isTargetProxy(target) && (Config::Get().IsRecorder() ||
                                 (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex))) {
    TextureStateObject(target).SetTexLevelParams(0, width, 1, 1, internalformat, 0, 0, 0, 1, 0, 0,
                                                 0);
    GenMipMapStateData(target, levels);
  }
}

inline void glTexStorage2D_SD(GLenum target,
                              GLsizei levels,
                              GLenum internalformat,
                              GLsizei width,
                              GLsizei height,
                              GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (!isTargetProxy(target) && (Config::Get().IsRecorder() ||
                                 (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex))) {
    TextureStateObject(target).SetTexLevelParams(0, width, height, 1, internalformat, 0, 0, 0, 1, 0,
                                                 0, 0);
    GenMipMapStateData(target, levels);
  }
}

inline void glTexStorage2DMultisample_SD(GLenum target,
                                         GLsizei samples,
                                         GLenum internalformat,
                                         GLsizei width,
                                         GLsizei height,
                                         GLboolean fixedsamplelocations,
                                         GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (!isTargetProxy(target) && (Config::Get().IsRecorder() ||
                                 (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex))) {
    TextureStateObject(target).SetTexLevelParams(0, width, height, 1, internalformat, 0, 0, 0,
                                                 samples, fixedsamplelocations, 0, 0);
  }
}

inline void glTexStorage3D_SD(GLenum target,
                              GLsizei levels,
                              GLenum internalformat,
                              GLsizei width,
                              GLsizei height,
                              GLsizei depth,
                              GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (!isTargetProxy(target) && (Config::Get().IsRecorder() ||
                                 (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex))) {
    TextureStateObject(target).SetTexLevelParams(0, width, height, depth, internalformat, 0, 0, 0,
                                                 1, 0, 0, 0);
    GenMipMapStateData(target, levels);
  }
}

inline void glTextureImage2DMultisampleNV_SD(GLuint texture,
                                             GLenum target,
                                             GLsizei samples,
                                             GLint internalFormat,
                                             GLsizei width,
                                             GLsizei height,
                                             GLboolean fixedSampleLocations,
                                             GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target, texture)
        .SetTexLevelParams(0, width, height, 1, internalFormat, 0, 0, 0, 1, fixedSampleLocations, 0,
                           0);
  }
}

inline void glTextureImage3DMultisampleNV_SD(GLuint texture,
                                             GLenum target,
                                             GLsizei samples,
                                             GLint internalFormat,
                                             GLsizei width,
                                             GLsizei height,
                                             GLsizei depth,
                                             GLboolean fixedSampleLocations,
                                             GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target, texture)
        .SetTexLevelParams(0, width, height, depth, internalFormat, 0, 0, 0, 1,
                           fixedSampleLocations, 0, 0);
  }
}

inline void glTextureStorage1D_SD(
    GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(GL_TEXTURE_1D, texture)
        .SetTexLevelParams(0, width, 1, 1, internalformat, 0, 0, 0, 1, 0, 0, 0);
    GenMipMapStateData(texture, GL_TEXTURE_1D, levels);
  }
}

inline void glTextureStorage1DEXT_SD(GLuint texture,
                                     GLenum target,
                                     GLsizei levels,
                                     GLenum internalformat,
                                     GLsizei width,
                                     GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target, texture)
        .SetTexLevelParams(0, width, 1, 1, internalformat, 0, 0, 0, 1, 0, 0, 0);
    GenMipMapStateData(texture, target, levels);
  }
}

inline void glTextureStorage2D_SD(GLuint texture,
                                  GLsizei levels,
                                  GLenum internalformat,
                                  GLsizei width,
                                  GLsizei height,
                                  GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    CTextureStateObj* textureState = SD().GetCurrentSharedStateData().Textures().Get(texture);
    if (textureState == nullptr) {
      throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown texture");
    }
    GLenum target = textureState->Target();
    TextureStateObject(target, texture)
        .SetTexLevelParams(0, width, height, 1, internalformat, 0, 0, 0, 1, 0, 0, 0);
    GenMipMapStateData(texture, target, levels);
  }
}

inline void glTextureStorage2DEXT_SD(GLuint texture,
                                     GLenum target,
                                     GLsizei levels,
                                     GLenum internalformat,
                                     GLsizei width,
                                     GLsizei height,
                                     GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    SetTargetForTexture(target, texture);
    TextureStateObject(target, texture)
        .SetTexLevelParams(0, width, height, 1, internalformat, 0, 0, 0, 1, 0, 0, 0);
    GenMipMapStateData(texture, target, levels);
  }
}

inline void glTextureStorage2DMultisample_SD(GLuint texture,
                                             GLsizei samples,
                                             GLenum internalformat,
                                             GLsizei width,
                                             GLsizei height,
                                             GLboolean fixedsamplelocations,
                                             GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    CTextureStateObj* textureState = SD().GetCurrentSharedStateData().Textures().Get(texture);
    if (textureState == nullptr) {
      throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown texture");
    }
    GLenum target = textureState->Target();
    TextureStateObject(target, texture)
        .SetTexLevelParams(0, width, height, 1, internalformat, 0, 0, 0, samples,
                           fixedsamplelocations, 0, 0);
  }
}

inline void glTextureStorage2DMultisampleEXT_SD(GLuint texture,
                                                GLenum target,
                                                GLsizei samples,
                                                GLenum internalformat,
                                                GLsizei width,
                                                GLsizei height,
                                                GLboolean fixedsamplelocations,
                                                GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target, texture)
        .SetTexLevelParams(0, width, height, 1, internalformat, 0, 0, 0, samples,
                           fixedsamplelocations, 0, 0);
  }
}

inline void glTextureStorage3D_SD(GLuint texture,
                                  GLsizei levels,
                                  GLenum internalformat,
                                  GLsizei width,
                                  GLsizei height,
                                  GLsizei depth,
                                  GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    CTextureStateObj* textureState = SD().GetCurrentSharedStateData().Textures().Get(texture);
    if (textureState == nullptr) {
      throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown texture");
    }
    GLenum target = textureState->Target();
    TextureStateObject(target, texture)
        .SetTexLevelParams(0, width, height, depth, internalformat, 0, 0, 0, 1, 0, 0, 0);
    GenMipMapStateData(texture, target, levels);
  }
}

inline void glTextureStorage3DMultisample_SD(GLuint texture,
                                             GLsizei samples,
                                             GLenum internalformat,
                                             GLsizei width,
                                             GLsizei height,
                                             GLsizei depth,
                                             GLboolean fixedsamplelocations,
                                             GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    CTextureStateObj* textureState = SD().GetCurrentSharedStateData().Textures().Get(texture);
    if (textureState == nullptr) {
      throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown texture");
    }
    GLenum target = textureState->Target();
    TextureStateObject(target, texture)
        .SetTexLevelParams(0, width, height, depth, internalformat, 0, 0, 0, 1,
                           fixedsamplelocations, 0, 0);
  }
}

inline void glTextureStorage3DEXT_SD(GLuint texture,
                                     GLenum target,
                                     GLsizei levels,
                                     GLenum internalformat,
                                     GLsizei width,
                                     GLsizei height,
                                     GLsizei depth,
                                     GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target, texture)
        .SetTexLevelParams(0, width, height, depth, internalformat, 0, 0, 0, 1, 0, 0, 0);
    GenMipMapStateData(texture, target, levels);
  }
}

inline void glTextureView_SD(GLuint texture,
                             GLenum target,
                             GLuint origtexture,
                             GLenum internalformat,
                             GLuint minlevel,
                             GLuint numlevels,
                             GLuint minlayer,
                             GLuint numlayers,
                             GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();
  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    SetTargetForTexture(target, texture);
    // TODO: Should we copy over the orig texture state and modify things that changed?
    //       This should be more accurate, but do we need state other than the target?
  }
}

inline void glCopyTexImage1D_SD(GLenum target,
                                GLint level,
                                GLenum internalformat,
                                GLint x,
                                GLint y,
                                GLsizei width,
                                GLint border,
                                GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (!isTargetProxy(target) && (Config::Get().IsRecorder() ||
                                 (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex))) {
    TextureStateObject(target).SetTexLevelParams(level, width, 1, 1, internalformat, border, 0, 0,
                                                 1, 0, 0, 0);
  }
}

inline void glCopyTexImage2D_SD(GLenum target,
                                GLint level,
                                GLenum internalformat,
                                GLint x,
                                GLint y,
                                GLsizei width,
                                GLsizei height,
                                GLint border,
                                GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (!isTargetProxy(target) && (Config::Get().IsRecorder() ||
                                 (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex))) {
    TextureStateObject(target).SetTexLevelParams(level, width, height, 1, internalformat, border, 0,
                                                 0, 1, 0, 0, 0);
  }
}

inline void glTexImage2DMultisample_SD(GLenum target,
                                       GLsizei samples,
                                       GLint internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLboolean fixedsamplelocations,
                                       GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (!isTargetProxy(target) && (Config::Get().IsRecorder() ||
                                 (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex))) {
    TextureStateObject(target).SetTexLevelParams(0, width, height, 1, internalformat, 0, 0, 0,
                                                 samples, fixedsamplelocations, 0, 0);
  }
}

inline void glTexImage3DMultisample_SD(GLenum target,
                                       GLsizei samples,
                                       GLint internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLsizei depth,
                                       GLboolean fixedsamplelocations,
                                       GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (!isTargetProxy(target) && (Config::Get().IsRecorder() ||
                                 (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex))) {
    TextureStateObject(target).SetTexLevelParams(0, width, height, depth, internalformat, 0, 0, 0,
                                                 samples, fixedsamplelocations, 0, 0);
  }
}

inline void glTexImage1D_SD(GLenum target,
                            GLint level,
                            GLint internalformat,
                            GLsizei width,
                            GLint border,
                            GLenum format,
                            GLenum type,
                            const GLvoid* pixels,
                            GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (!isTargetProxy(target) && (Config::Get().IsRecorder() ||
                                 (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex))) {
    TextureStateObject(target).SetTexLevelParams(level, width, 1, 1, internalformat, border, 0, 0,
                                                 1, 0, format, type);
  }
}

inline void glTexImage2D_SD(GLenum target,
                            GLint level,
                            GLint internalformat,
                            GLsizei width,
                            GLsizei height,
                            GLint border,
                            GLenum format,
                            GLenum type,
                            const GLvoid* pixels,
                            GLboolean recording = 0) {
  // WA for Antutu Android Benchmark invalid enums;
  if (format <= 0 || type <= 0) {
    return;
  }

  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();
  void* currentContext = SD().GetCurrentContext();

  if (!isTargetProxy(target) && ((Config::Get().IsRecorder() && currentContext != 0) ||
                                 (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex))) {
    TextureStateObject(target).SetTexLevelParams(level, width, height, 1, internalformat, border, 0,
                                                 0, 1, 0, format, type);
  }
}

inline void glTexImage3D_SD(GLenum target,
                            GLint level,
                            GLenum internalformat,
                            GLsizei width,
                            GLsizei height,
                            GLsizei depth,
                            GLint border,
                            GLenum format,
                            GLenum type,
                            const GLvoid* pixels,
                            GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (!isTargetProxy(target) && (Config::Get().IsRecorder() ||
                                 (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex))) {
    TextureStateObject(target).SetTexLevelParams(level, width, height, depth, internalformat,
                                                 border, 0, 0, 1, 0, format, type);
  }
}

inline void glCompressedMultiTexImage1DEXT_SD(GLenum texunit,
                                              GLenum target,
                                              GLint level,
                                              GLenum internalformat,
                                              GLsizei width,
                                              GLint border,
                                              GLsizei imageSize,
                                              const GLvoid* bits,
                                              GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target, texunit)
        .SetTexLevelParams(level, width, 1, 1, internalformat, border, 1, imageSize, 0, 0, 0, 0);
  }
}

inline void glCompressedMultiTexImage2DEXT_SD(GLenum texunit,
                                              GLenum target,
                                              GLint level,
                                              GLenum internalformat,
                                              GLsizei width,
                                              GLsizei height,
                                              GLint border,
                                              GLsizei imageSize,
                                              const GLvoid* bits,
                                              GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target, texunit)
        .SetTexLevelParams(level, width, height, 1, internalformat, border, 1, imageSize, 0, 0, 0,
                           0);
  }
}

inline void glCompressedMultiTexImage3DEXT_SD(GLenum texunit,
                                              GLenum target,
                                              GLint level,
                                              GLenum internalformat,
                                              GLsizei width,
                                              GLsizei height,
                                              GLsizei depth,
                                              GLint border,
                                              GLsizei imageSize,
                                              const GLvoid* bits,
                                              GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target, texunit)
        .SetTexLevelParams(level, width, height, depth, internalformat, border, 1, imageSize, 0, 0,
                           0, 0);
  }
}

inline void glMultiTexImage1DEXT_SD(GLenum texunit,
                                    GLenum target,
                                    GLint level,
                                    GLenum internalformat,
                                    GLsizei width,
                                    GLint border,
                                    GLenum format,
                                    GLenum type,
                                    const GLvoid* pixels,
                                    GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target, texunit)
        .SetTexLevelParams(level, width, 1, 1, internalformat, border, 0, 0, 1, 0, format, type);
  }
}

inline void glMultiTexImage2DEXT_SD(GLenum texunit,
                                    GLenum target,
                                    GLint level,
                                    GLenum internalformat,
                                    GLsizei width,
                                    GLsizei height,
                                    GLint border,
                                    GLenum format,
                                    GLenum type,
                                    const GLvoid* pixels,
                                    GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target, texunit)
        .SetTexLevelParams(level, width, height, 1, internalformat, border, 0, 0, 1, 0, format,
                           type);
  }
}

inline void glMultiTexImage3DEXT_SD(GLenum texunit,
                                    GLenum target,
                                    GLint level,
                                    GLenum internalformat,
                                    GLsizei width,
                                    GLsizei height,
                                    GLsizei depth,
                                    GLint border,
                                    GLenum format,
                                    GLenum type,
                                    const GLvoid* pixels,
                                    GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target, texunit)
        .SetTexLevelParams(level, width, height, depth, internalformat, border, 0, 0, 1, 0, format,
                           type);
  }
}

inline void glCompressedTexImage1D_SD(GLenum target,
                                      GLint level,
                                      GLenum internalformat,
                                      GLsizei width,
                                      GLint border,
                                      GLsizei imageSize,
                                      const GLvoid* data,
                                      GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target).SetTexLevelParams(level, width, 1, 1, internalformat, border, 1,
                                                 imageSize, 1, 0, 0, 0);
  }
}

inline void glCompressedTexImage2D_SD(GLenum target,
                                      GLint level,
                                      GLenum internalformat,
                                      GLsizei width,
                                      GLsizei height,
                                      GLint border,
                                      GLsizei imageSize,
                                      const GLvoid* data,
                                      GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target).SetTexLevelParams(level, width, height, 1, internalformat, border, 1,
                                                 imageSize, 1, 0, 0, 0);
  }

  // We track data only when not recording to not keep too much garbage in
  // resources
  if (Config::Get().IsRecorder() && !curctx::IsOgl() && recording &&
      Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
    ESCompressedTexDataTrack(target, level, imageSize, data);
  }
}

inline void glCompressedTexImage3D_SD(GLenum target,
                                      GLint level,
                                      GLenum internalformat,
                                      GLsizei width,
                                      GLsizei height,
                                      GLsizei depth,
                                      GLint border,
                                      GLsizei imageSize,
                                      const GLvoid* data,
                                      GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target).SetTexLevelParams(level, width, height, depth, internalformat,
                                                 border, 1, imageSize, 1, 0, 0, 0);
  }

  // We track data only when not recording to not keep too much garbage in
  // resources
  if (Config::Get().IsRecorder() && !curctx::IsOgl() && recording &&
      Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
    ESCompressedTexDataTrack(target, level, imageSize, data);
  }
}

inline void glCompressedTextureImage1DEXT_SD(GLuint texture,
                                             GLenum target,
                                             GLint level,
                                             GLenum internalformat,
                                             GLsizei width,
                                             GLint border,
                                             GLsizei imageSize,
                                             const GLvoid* bits,
                                             GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    SetTargetForTexture(target, texture);
    TextureStateObject(target, texture)
        .SetTexLevelParams(level, width, 1, 1, internalformat, border, 1, imageSize, 0, 0, 0, 0);
  }
}

inline void glCompressedTextureImage2DEXT_SD(GLuint texture,
                                             GLenum target,
                                             GLint level,
                                             GLenum internalformat,
                                             GLsizei width,
                                             GLsizei height,
                                             GLint border,
                                             GLsizei imageSize,
                                             const GLvoid* bits,
                                             GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    SetTargetForTexture(target, texture);
    TextureStateObject(target, texture)
        .SetTexLevelParams(level, width, height, 1, internalformat, border, 1, imageSize, 0, 0, 0,
                           0);
  }
}

inline void glCompressedTextureImage3DEXT_SD(GLuint texture,
                                             GLenum target,
                                             GLint level,
                                             GLenum internalformat,
                                             GLsizei width,
                                             GLsizei height,
                                             GLsizei depth,
                                             GLint border,
                                             GLsizei imageSize,
                                             const GLvoid* bits,
                                             GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    SetTargetForTexture(target, texture);
    TextureStateObject(target, texture)
        .SetTexLevelParams(level, width, height, depth, internalformat, border, 1, imageSize, 0, 0,
                           0, 0);
  }
}

inline void glTextureImage1DEXT_SD(GLuint texture,
                                   GLenum target,
                                   GLint level,
                                   GLenum internalformat,
                                   GLsizei width,
                                   GLint border,
                                   GLenum format,
                                   GLenum type,
                                   const GLvoid* pixels,
                                   GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    TextureStateObject(target, texture)
        .SetTexLevelParams(level, width, 1, 1, internalformat, border, 0, 0, 1, 0, format, type);
  }
}

inline void glTextureImage2DEXT_SD(GLuint texture,
                                   GLenum target,
                                   GLint level,
                                   GLenum internalformat,
                                   GLsizei width,
                                   GLsizei height,
                                   GLint border,
                                   GLenum format,
                                   GLenum type,
                                   const GLvoid* pixels,
                                   GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    SetTargetForTexture(target, texture);
    TextureStateObject(target, texture)
        .SetTexLevelParams(level, width, height, 1, internalformat, border, 0, 0, 1, 0, format,
                           type);
  }
}

inline void glTextureImage3DEXT_SD(GLuint texture,
                                   GLenum target,
                                   GLint level,
                                   GLenum internalformat,
                                   GLsizei width,
                                   GLsizei height,
                                   GLsizei depth,
                                   GLint border,
                                   GLenum format,
                                   GLenum type,
                                   const GLvoid* pixels,
                                   GLboolean recording = 0) {
  bool isCaptureTex = !Config::Get().opengl.player.capture2DTexs.empty() ||
                      !Config::Get().opengl.player.captureDraws2DTexs.empty();

  if (Config::Get().IsRecorder() || (Config::Get().IsPlayer() && curctx::IsEs() && isCaptureTex)) {
    SetTargetForTexture(target, texture);
    TextureStateObject(target, texture)
        .SetTexLevelParams(level, width, height, depth, internalformat, border, 0, 0, 1, 0, format,
                           type);
  }
}

inline void glLineStipple_SD(GLint factor, GLushort pattern, GLboolean recording = 0) {
  CGeneralStateData::Track::CLineStipple& lineStipple =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.lineStipple;
  lineStipple.used = true;
  lineStipple.factor = factor;
  lineStipple.pattern = pattern;
}

inline void glBindBufferBase_SD(GLenum target,
                                GLuint index,
                                GLuint buffer,
                                GLboolean recording = 0) {
  setTargetForBuff(target, buffer);
  updateIndexedTargetInfo(target, index, buffer);
  SD().GetCurrentContextStateData().Bindings().BoundBuffers()[target] = (GLint)buffer;
}

inline void glBindBuffersBase_SD(
    GLenum target, GLuint first, GLsizei count, const GLuint* buffers, GLboolean recording = 0) {
  for (int i = 0; i < count; i++) {
    auto buffer = buffers != NULL ? buffers[i] : 0;
    setTargetForBuff(target, buffer);
    updateIndexedTargetInfo(target, first + i, buffer);
    SD().GetCurrentContextStateData().Bindings().BoundBuffers()[target] = (GLint)buffer;
  }
}

inline void glBindBufferOffsetEXT_SD(
    GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLboolean recording = 0) {
  setTargetForBuff(target, buffer);
  updateIndexedTargetInfo(target, index, buffer, offset);
  SD().GetCurrentContextStateData().Bindings().BoundBuffers()[target] = (GLint)buffer;
}

inline void glBindBufferRange_SD(GLenum target,
                                 GLuint index,
                                 GLuint buffer,
                                 GLintptr offset,
                                 GLsizeiptr size,
                                 GLboolean recording = 0) {
  setTargetForBuff(target, buffer);
  updateIndexedTargetInfo(target, index, buffer, offset, size);
  SD().GetCurrentContextStateData().Bindings().BoundBuffers()[target] = (GLint)buffer;
}

inline void glMapBufferOES_SD(GLvoid* return_value,
                              GLenum target,
                              GLenum access,
                              GLboolean recording = 0) {
  bool error = false;
  GLint buffer = boundBuff(target);
  if (buffer > 0) {
    CBufferStateObj* bufferState = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferState != nullptr) {
      if (Config::Get().IsRecorder()) {
        bufferState->InitBufferMapRecOES(MapAccessEnumToBitField(access));
      } else {
        bufferState->InitBufferMapPlayOES(MapAccessEnumToBitField(access));
      }
    } else {
      error = true;
    }
  } else {
    error = true;
  }

  if (error) {
    throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown buffer");
  }
}

inline void glMapBufferARB_SD(GLvoid* return_value,
                              GLenum target,
                              GLenum access,
                              GLboolean recording = 0) {
  bool error = false;
  GLint buffer = boundBuff(target);
  if (buffer > 0) {
    CBufferStateObj* bufferState = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferState != nullptr) {
      if (Config::Get().IsRecorder()) {
        bufferState->InitBufferMapRecARB(MapAccessEnumToBitField(access));
      } else {
        bufferState->InitBufferMapPlayARB(MapAccessEnumToBitField(access));
      }
    } else {
      error = true;
    }
  } else {
    error = true;
  }

  if (error) {
    throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown buffer");
  }
}

inline void glMapBuffer_SD(GLvoid* return_value,
                           GLenum target,
                           GLenum access,
                           GLboolean recording = 0) {
  bool error = false;
  GLint buffer = boundBuff(target);
  if (buffer > 0) {
    CBufferStateObj* bufferState = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferState != nullptr) {
      if (Config::Get().IsRecorder()) {
        bufferState->InitBufferMapRec(MapAccessEnumToBitField(access));
      } else {
        bufferState->InitBufferMapPlay(MapAccessEnumToBitField(access));
      }
    } else {
      error = true;
    }
  } else {
    error = true;
  }

  if (error) {
    throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown buffer");
  }
}

inline void glMapBufferRange_SD(GLvoid* return_value,
                                GLenum target,
                                GLintptr offset,
                                GLsizeiptr length,
                                GLbitfield access,
                                GLboolean recording = 0) {
  access = access & Config::Get().opengl.recorder.bufferMapAccessMask;
  bool error = false;
  GLint buffer = boundBuff(target);
  if (buffer > 0) {
    CBufferStateObj* bufferState = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferState != nullptr) {
      if (Config::Get().IsRecorder()) {
        bufferState->InitBufferMapRec(access, false, GLint(length), GLint(offset));
      } else {
        bufferState->InitBufferMapPlay(access, false, GLint(length), GLint(offset));
      }
    } else {
      error = true;
    }
  } else {
    error = true;
  }

  if (error) {
    throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown buffer");
  }
}

inline void glMapNamedBuffer_SD(GLvoid* return_value,
                                GLuint buffer,
                                GLenum access,
                                GLboolean recording = 0) {
  bool error = false;
  if (buffer > 0) {
    CBufferStateObj* bufferState = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferState != nullptr) {
      if (Config::Get().IsRecorder()) {
        bufferState->InitBufferMapRecEXT(MapAccessEnumToBitField(access), true);
      } else {
        bufferState->InitBufferMapPlayEXT(MapAccessEnumToBitField(access), true);
      }
    } else {
      error = true;
    }
  } else {
    error = true;
  }

  if (error) {
    throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown buffer");
  }
}

inline void glMapNamedBufferRange_SD(GLvoid* return_value,
                                     GLuint buffer,
                                     GLintptr offset,
                                     GLsizeiptr length,
                                     GLbitfield access,
                                     GLboolean recording = 0) {
  access = access & Config::Get().opengl.recorder.bufferMapAccessMask;
  bool error = false;
  if (buffer > 0) {
    CBufferStateObj* bufferState = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferState != nullptr) {
      if (Config::Get().IsRecorder()) {
        bufferState->InitBufferMapRecEXT(access, true, GLint(length), GLint(offset));
      } else {
        bufferState->InitBufferMapPlayEXT(access, true, GLint(length), GLint(offset));
      }
    } else {
      error = true;
    }
  } else {
    error = true;
  }

  if (error) {
    throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown buffer");
  }
}

inline void glCompressedTexSubImage1D_SD(GLenum target,
                                         GLint level,
                                         GLint xoffset,
                                         GLsizei width,
                                         GLenum format,
                                         GLsizei imageSize,
                                         const GLvoid* data,
                                         GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    // OGLES Update compressed image size and data if it is a whole texture data
    // initialization
    CTextureStateData::TMipmapTextureData& mipmapData =
        TextureStateObject(target).Data().track.mipmapData[level];
    if (width == mipmapData.width) {
      mipmapData.compressedImageSize = imageSize;
    }
    if (!curctx::IsOgl()) {
      if (width == mipmapData.width) {
        if (recording && Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
          // Track data only when not recording as it is useless and heavy in
          // long streams
          ESCompressedTexDataTrack(target, level, imageSize, data);
        }
      } else if (recording &&
                 Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
        // No support for restoration of  sub imaged compressed textures in ES
        throw ENotImplemented(EXCEPTION_MESSAGE);
      }
    }
  }
}

inline void glCompressedTexSubImage2D_SD(GLenum target,
                                         GLint level,
                                         GLint xoffset,
                                         GLint yoffset,
                                         GLsizei width,
                                         GLsizei height,
                                         GLenum format,
                                         GLsizei imageSize,
                                         const GLvoid* data,
                                         GLboolean recording = 0) {
  if (Config::Get().IsRecorder()) {
    // OGLES Update compressed image size and data if it is a whole texture data
    // initialization
    CTextureStateData::TMipmapTextureData& mipmapData =
        TextureStateObject(target).Data().track.mipmapData[level];
    mipmapData.compressed = GL_TRUE;

    if (width == mipmapData.width && height == mipmapData.height && imageSize > 0) {
      mipmapData.compressedImageSize = imageSize;
    }
    if (!curctx::IsOgl()) {
      if (width == mipmapData.width && height == mipmapData.height) {
        if (recording && Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
          // Track data only when not recording as it is useless and heavy in
          // long streams
          ESCompressedTexDataTrack(target, level, imageSize, data);
        }
      } else if (recording &&
                 Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
        // No support for restoration of  sub imaged compressed textures in ES
        throw ENotImplemented(EXCEPTION_MESSAGE);
      }
    }
  }
}

inline void glCompressedTexSubImage3D_SD(GLenum target,
                                         GLint level,
                                         GLint xoffset,
                                         GLint yoffset,
                                         GLint zoffset,
                                         GLsizei width,
                                         GLsizei height,
                                         GLsizei depth,
                                         GLenum format,
                                         GLsizei imageSize,
                                         const GLvoid* data,
                                         GLboolean recording = 0) {
  if (Config::Get().IsRecorder()) {
    // OGLES Update compressed image size and data if it is a whole texture data
    // initialization
    CTextureStateData::TMipmapTextureData& mipmapData =
        TextureStateObject(target).Data().track.mipmapData[level];
    if (width == mipmapData.width && height == mipmapData.height && depth == mipmapData.depth) {
      mipmapData.compressedImageSize = imageSize;
    }
    if (!curctx::IsOgl()) {
      if (width == mipmapData.width && height == mipmapData.height && depth == mipmapData.depth) {
        if (recording && Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
          // Track data only when not recording as it is useless and heavy in
          // long streams
          ESCompressedTexDataTrack(target, level, imageSize, data);
        }
      } else if (recording &&
                 Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
        // No support for restoration of  sub imaged compressed textures in ES
        throw ENotImplemented(EXCEPTION_MESSAGE);
      }
    }
  }
}

inline void glCompressedTextureSubImage1DEXT_SD(GLuint texture,
                                                GLenum target,
                                                GLint level,
                                                GLint xoffset,
                                                GLsizei width,
                                                GLenum format,
                                                GLsizei imageSize,
                                                const GLvoid* bits,
                                                GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    // OGLES Update compressed image size and data if it is a whole texture data
    // initialization
    CTextureStateData::TMipmapTextureData& mipmapData =
        TextureStateObject(target, texture).Data().track.mipmapData[level];
    if (width == mipmapData.width) {
      mipmapData.compressedImageSize = imageSize;
    }
    if (!curctx::IsOgl()) {
      if (width == mipmapData.width) {
        if (recording && Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
          // Track data only when not recording as it is useless and heavy in
          // long streams
          ESCompressedTexDataTrack(target, level, imageSize, bits);
        }
      } else if (recording &&
                 Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
        // No support for restoration of  sub imaged compressed textures in ES
        throw ENotImplemented(EXCEPTION_MESSAGE);
      }
    }
  }
}

inline void glCompressedTextureSubImage2D_SD(GLuint texture,
                                             GLint level,
                                             GLint xoffset,
                                             GLint yoffset,
                                             GLsizei width,
                                             GLsizei height,
                                             GLenum format,
                                             GLsizei imageSize,
                                             const GLvoid* bits,
                                             GLboolean recording = 0 /*= false*/) {
  if (!Config::Get().IsRecorder()) {
    return;
  }

  // OGLES Update compressed image size and data if it is a whole texture
  // data initialization.
  const CTextureStateObj* const textureState =
      SD().GetCurrentSharedStateData().Textures().Get(texture);
  if (textureState == nullptr) {
    throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown texture");
  }
  const GLenum target = textureState->Target();
  CTextureStateData::TMipmapTextureData& mipmapData =
      TextureStateObject(target, texture).Data().track.mipmapData[level];
  const bool wholeTextureUsed = (width == mipmapData.width && height == mipmapData.height);
  if (wholeTextureUsed) {
    mipmapData.compressedImageSize = imageSize;
  }

  if (!curctx::IsOgl()) {
    // Track data only when not recording as it is useless and heavy in long streams.
    const auto& texState = Config::Get().opengl.recorder.texturesState;
    if (recording && texState == TTexturesState::RESTORE) {
      if (wholeTextureUsed) {
        ESCompressedTexDataTrack(target, level, imageSize, bits);
      } else {
        // No support for restoration of sub imaged compressed textures in ES.
        throw ENotImplemented(EXCEPTION_MESSAGE);
      }
    }
  }
}

inline void glCompressedTextureSubImage2DEXT_SD(GLuint texture,
                                                GLenum target,
                                                GLint level,
                                                GLint xoffset,
                                                GLint yoffset,
                                                GLsizei width,
                                                GLsizei height,
                                                GLenum format,
                                                GLsizei imageSize,
                                                const GLvoid* bits,
                                                GLboolean recording = 0 /*= false*/) {
  if (!Config::Get().IsRecorder()) {
    return;
  }

  // OGLES Update compressed image size and data if it is a whole texture
  // data initialization.
  CTextureStateData::TMipmapTextureData& mipmapData =
      TextureStateObject(target, texture).Data().track.mipmapData[level];
  const bool wholeTextureUsed = (width == mipmapData.width && height == mipmapData.height);
  if (wholeTextureUsed) {
    mipmapData.compressedImageSize = imageSize;
  }

  if (!curctx::IsOgl()) {
    // Track data only when not recording as it is useless and heavy in long streams.
    const auto& texState = Config::Get().opengl.recorder.texturesState;
    if (recording && texState == TTexturesState::RESTORE) {
      if (wholeTextureUsed) {
        ESCompressedTexDataTrack(target, level, imageSize, bits);
      } else {
        // No support for restoration of sub imaged compressed textures in ES.
        throw ENotImplemented(EXCEPTION_MESSAGE);
      }
    }
  }
}

inline void glCompressedTextureSubImage3DEXT_SD(GLuint texture,
                                                GLenum target,
                                                GLint level,
                                                GLint xoffset,
                                                GLint yoffset,
                                                GLint zoffset,
                                                GLsizei width,
                                                GLsizei height,
                                                GLsizei depth,
                                                GLenum format,
                                                GLsizei imageSize,
                                                const GLvoid* bits,
                                                GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    // OGLES Update compressed image size and data if it is a whole texture data
    // initialization
    CTextureStateData::TMipmapTextureData& mipmapData =
        TextureStateObject(target, texture).Data().track.mipmapData[level];
    if (width == mipmapData.width && height == mipmapData.height && depth == mipmapData.depth) {
      mipmapData.compressedImageSize = imageSize;
    }
    if (!curctx::IsOgl()) {
      if (width == mipmapData.width && height == mipmapData.height && depth == mipmapData.depth) {
        if (recording && Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
          // Track data only when not recording as it is useless and heavy in
          // long streams
          ESCompressedTexDataTrack(target, level, imageSize, bits);
        }
      } else if (recording &&
                 Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
        // No support for restoration of  sub imaged compressed textures in ES
        throw ENotImplemented(EXCEPTION_MESSAGE);
      }
    }
  }
}

inline void glAttachObjectARB_SD(GLhandleARB containerObj,
                                 GLhandleARB obj,
                                 GLboolean recording = 0) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV)) {
    SD().GetCurrentSharedStateData()
        .GLSLPrograms()
        .Get(containerObj)
        ->AttachShader(SD().GetCurrentSharedStateData().GLSLShaders().Get(obj)->DataShared());
  }
}

inline void glAttachShader_SD(GLuint program, GLuint shader, GLboolean recording = 0) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV) || Config::Get().common.shared.useEvents) {
    if (shader == 0) {
      return;
    }
    SD().GetCurrentSharedStateData().GLSLPrograms().Get(program)->AttachShader(
        SD().GetCurrentSharedStateData().GLSLShaders().Get(shader)->DataShared());
  }
}

inline void glBindFragDataLocation_SD(GLuint program,
                                      GLuint color,
                                      const GLchar* name,
                                      GLboolean recording = 0) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV)) {
    CGLSLProgramStateObj* programStateObj =
        SD().GetCurrentSharedStateData().GLSLPrograms().Get(program);
    if (programStateObj == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    } else {
      programStateObj->Data().track.fragDataLocationBindings[color] = name;
    }
  }
}

inline void glBindProgramARB_SD(GLenum target, GLuint program, GLboolean recording = 0) {
  if (Config::Get().IsRecorder()) {
    if (SD().GetCurrentSharedStateData().ARBPrograms().Get(program) == 0) {
      // add new entry only if it does not exist
      SD().GetCurrentSharedStateData().ARBPrograms().Add(CARBProgramStateObj(program, target));
    }
  }
}

inline void glBindRenderbuffer_SD(GLenum target, GLuint renderbuffer, GLboolean recording = 0) {
  if (Config::Get().IsRecorder()) {
    CRenderbufferStateObj* renderbufferStateObj =
        SD().GetCurrentSharedStateData().Renderbuffers().Get(renderbuffer);

    if (renderbufferStateObj != 0) {
      renderbufferStateObj->SetTarget(target);
    }
  }
}

inline void glBindRenderbufferEXT_SD(GLenum target, GLuint renderbuffer, GLboolean recording = 0) {
  if (Config::Get().IsRecorder()) {
    CRenderbufferStateObj* renderbufferStateObj =
        SD().GetCurrentSharedStateData().RenderbuffersEXT().Get(renderbuffer);

    if (renderbufferStateObj != 0) {
      renderbufferStateObj->SetTarget(target);
    } else {
      SD().GetCurrentSharedStateData().RenderbuffersEXT().Add(
          CRenderbufferStateObj(renderbuffer, target));
    }
  }
}

inline void glBufferSubData_SD(
    GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data, GLboolean recording = 0) {
  if (Config::Get().IsRecorder()) {
    OptionalBufferDataTrack(boundBuff(target), offset, size, data, recording);
  }
}

inline void glClearBufferSubData_SD(GLenum target,
                                    GLenum internalformat,
                                    GLintptr offset,
                                    GLsizeiptr size,
                                    GLenum format,
                                    GLenum type,
                                    const void* data,
                                    GLboolean recording = 0) {
  if (Config::Get().IsRecorder()) {
    std::vector<uint8_t> tmpData = GetBufferData(target, offset, size);

    OptionalBufferDataTrack(boundBuff(target), offset, size, tmpData.data(), recording);
  }
}

inline void glClearNamedBufferData_SD(GLuint buffer,
                                      GLenum internalformat,
                                      GLenum format,
                                      GLenum type,
                                      const void* data,
                                      GLboolean recording = 0) {
  if (Config::Get().IsRecorder()) {
    auto* bufferStateData = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferStateData == nullptr) {
      return;
    }
    auto size = bufferStateData->Size();
    GLintptr offset = 0;

    std::vector<uint8_t> tmpData = GetNamedBufferData(buffer, offset, size);

    OptionalBufferDataTrack(buffer, offset, size, tmpData.data(), recording);
  }
}

inline void glClearNamedBufferSubData_SD(GLuint buffer,
                                         GLenum internalformat,
                                         GLintptr offset,
                                         GLsizeiptr size,
                                         GLenum format,
                                         GLenum type,
                                         const void* data,
                                         GLboolean recording = 0) {
  if (Config::Get().IsRecorder()) {
    std::vector<uint8_t> tmpData = GetNamedBufferData(buffer, offset, size);

    OptionalBufferDataTrack(buffer, offset, size, tmpData.data(), recording);
  }
}

inline void glNamedBufferSubData_SD(GLuint buffer,
                                    GLintptr offset,
                                    GLsizeiptr size,
                                    const GLvoid* data,
                                    GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    OptionalBufferDataTrack(buffer, offset, size, data, recording);
  }
}

inline void glColorPointerEXT_SD(GLint size,
                                 GLenum type,
                                 GLsizei stride,
                                 GLsizei count,
                                 const GLvoid* pointer,
                                 GLboolean recording = 0) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);
  }
}

inline void glCompileShader_SD(GLuint shader, GLboolean recording = 0) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV) || Config::Get().common.shared.useEvents) {
    SD().GetCurrentSharedStateData().GLSLShaders().Get(shader)->Compile();
  }
}

inline void glCreateProgram_SD(GLuint retVal, GLboolean recording = 0) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV) || Config::Get().common.shared.useEvents) {
    SD().GetCurrentSharedStateData().GLSLPrograms().Add(CGLSLProgramStateObj(retVal));
  }
}

inline void glCreateShader_SD(GLuint retVal, GLenum type, GLboolean recording = 0) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV) || Config::Get().common.shared.useEvents) {
    SD().GetCurrentSharedStateData().GLSLShaders().Add(CGLSLShaderStateObj(retVal, type));
  }
}

inline void glDeleteObjectARB_SD(GLhandleARB obj, GLboolean recording = 0 /*= false*/) {
  if (SD().GetCurrentContext() == 0) {
    return;
  }

  if (Config::Get().IsRecorder() || ShouldLog(TRACEV)) {
    // can be either program or shader
    CSharedStateDynamic::CGLSLPrograms& programsState =
        SD().GetCurrentSharedStateData().GLSLPrograms();
    CSharedStateDynamic::CGLSLShaders& shadersState =
        SD().GetCurrentSharedStateData().GLSLShaders();
    // program

    if (programsState.Get(obj) != 0) {
      // Remove shaders that were marked to delete and this is the last program
      // they were attached to
      std::vector<GLuint> attachedShaders = programsState.Get(obj)->AttachedShaders();
      for (auto shader : attachedShaders) {
        programsState.Get(obj)->DetachShader(shader);
      }
      programsState.Remove(1, &obj);
    }

    // shader
    CGLSLShaderStateObj* shaderStateObjPtr = shadersState.Get(obj);
    if (shaderStateObjPtr == 0) {
      return;
    }

    if (shaderStateObjPtr->IsAttached()) {
      shaderStateObjPtr->MarkToDelete();
    } else {
      shadersState.Remove(1, &obj);
    }
  }
}

inline void glDeleteShader_SD(GLuint shader, GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV) || Config::Get().common.shared.useEvents) {
    SD().GetCurrentSharedStateData().GLSLShaders().Remove(1, &shader);
  }
}

inline void glDeleteProgram_SD(GLuint program, GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV) || Config::Get().common.shared.useEvents) {
    CSharedStateDynamic::CGLSLPrograms& programsState =
        SD().GetCurrentSharedStateData().GLSLPrograms();

    if (programsState.Get(CGLSLProgramStateObj(program)) == 0) {
      Log(WARN) << "Deleting unknown program: " << program
                << ". This event will be not tracked by GITS state tracking system.";
      return;
    }

    // Remove shaders that were marked to delete and this is the last program
    // they were attached to
    std::vector<GLuint> attachedShaders = programsState.Get(program)->AttachedShaders();
    for (auto shader : attachedShaders) {
      programsState.Get(program)->DetachShader(shader);
    }

    if (programsState.Get(program)->IsPartOfPipeline()) {
      programsState.Get(program)->MarkToDelete();
    } else {
      programsState.Remove(1, &program);
    }
  }
}

inline void glDeleteProgramsARB_SD(GLsizei n,
                                   const GLuint* programs,
                                   GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().ARBPrograms().Remove(n, programs);
  }
}

inline void glDeleteRenderbuffers_SD(GLsizei n,
                                     const GLuint* renderbuffers,
                                     GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().Renderbuffers().Remove(unsigned(n), renderbuffers);
  }
}

inline void glDeleteRenderbuffersEXT_SD(GLsizei n,
                                        const GLuint* renderbuffers,
                                        GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().RenderbuffersEXT().Remove((unsigned)n, renderbuffers);
  }
}

inline void glDeleteSamplers_SD(GLsizei count,
                                const GLuint* samplers,
                                GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().Samplers().Remove((unsigned)count, samplers);
  }
}

inline void glDeleteTextures_SD(GLsizei n,
                                const GLuint* textures,
                                GLboolean recording = 0 /*= false*/) {
  if (SD().GetCurrentContext() == 0) {
    return;
  }

  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().Textures().Remove((unsigned)n, textures);
  }
}

inline void glDeleteVertexArrays_SD(GLsizei n,
                                    const GLuint* arrays,
                                    GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentContextStateData().VertexArrays().Remove(n, arrays);
  }
}

inline void glDetachShader_SD(GLuint program, GLuint shader, GLboolean recording = 0 /*= false*/) {
  if (SD().GetCurrentContext() == 0) {
    return;
  }

  if ((Config::Get().IsRecorder() &&
       SD().GetCurrentSharedStateData().GLSLPrograms().Get(program) != 0) ||
      ShouldLog(TRACEV)) {
    SD().GetCurrentSharedStateData().GLSLPrograms().Get(program)->DetachShader(shader);
  }
}

inline void glDetachObjectARB_SD(GLhandleARB containerObj,
                                 GLhandleARB attachedObj,
                                 GLboolean recording = 0 /*= false*/) {
  if (SD().GetCurrentContext() == 0) {
    return;
  }

  if ((Config::Get().IsRecorder() &&
       SD().GetCurrentSharedStateData().GLSLPrograms().Get(containerObj) != 0) ||
      ShouldLog(TRACEV)) {
    SD().GetCurrentSharedStateData().GLSLPrograms().Get(containerObj)->DetachShader(attachedObj);
  }
}

inline void glDisableVertexAttribArray_SD(GLuint index, GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    // Vertex Array Objects can't contain client arrays - client arrays data are
    // stored only when VAO=0
    GLint vertexArrayObject = 0;
    if (curctx::IsOgl()) {
      drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertexArrayObject);
    }

    if (vertexArrayObject == 0) {
      CClientArraysStateObj& arrays = SD().GetCurrentContextStateData().ClientArrays();
      arrays.VertexAttribArray(index).Enabled(false);
    }
  }
}

inline void glEnableVertexAttribArray_SD(GLuint index, GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    // Vertex Array Objects can't contain client arrays - client arrays data are
    // stored only when VAO=0
    GLint vertexArrayObject = 0;
    if (curctx::IsOgl()) {
      drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertexArrayObject);
    }

    if (vertexArrayObject == 0) {
      CClientArraysStateObj& arrays = SD().GetCurrentContextStateData().ClientArrays();
      arrays.VertexAttribArray(index).Enabled(true);
    }
  }
}

inline void glEnableVertexArrayAttrib_SD(GLuint vao,
                                         GLuint index,
                                         GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (vao == 0) {
      CClientArraysStateObj& arrays = SD().GetCurrentContextStateData().ClientArrays();
      arrays.VertexAttribArray(index).Enabled(true);
    }
  }
}

inline void glGenBuffers_SD(GLsizei n, GLuint* buffers, GLboolean recording = 0 /*= false*/) {
  SD().GetCurrentSharedStateData().Buffers().Generate((unsigned)n, buffers);
}

inline void glCreateBuffers_SD(GLsizei n, GLuint* buffers, GLboolean recording = 0 /*= false*/) {
  SD().GetCurrentSharedStateData().Buffers().Generate((unsigned)n, buffers);
}

inline void glGenRenderbuffers_SD(GLsizei n,
                                  GLuint* renderbuffers,
                                  GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().Renderbuffers().Generate((unsigned)n, renderbuffers);
  }
}

inline void glGenRenderbuffersEXT_SD(GLsizei n,
                                     GLuint* renderbuffers,
                                     GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().RenderbuffersEXT().Generate((unsigned)n, renderbuffers);
  }
}

inline void glGenSamplers_SD(GLsizei count, GLuint* samplers, GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().Samplers().Generate((unsigned)count, samplers);
  }
}

inline void glCreateSamplers_SD(GLsizei count,
                                GLuint* samplers,
                                GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().Samplers().Generate((unsigned)count, samplers);
  }
}

inline void glGenTextures_SD(GLsizei n, GLuint* textures, GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().Textures().Generate((unsigned)n, textures);
  }
}

inline void glCreateTextures_SD(GLenum target,
                                GLsizei n,
                                GLuint* textures,
                                GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().Textures().Generate((unsigned)n, textures);
    for (GLsizei i = 0; i < n; ++i) {
      SetTargetForTexture(target, textures[i]);
    }
  }
}

inline void glGenVertexArrays_SD(GLsizei n, GLuint* arrays, GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentContextStateData().VertexArrays().Generate((unsigned)n, arrays);
  }
}

inline void glCreateVertexArrays_SD(GLsizei n,
                                    GLuint* arrays,
                                    GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentContextStateData().VertexArrays().Generate((unsigned)n, arrays);
  }
}

inline void checkVertexArrayTrackingHelper(GLint vaobj) {
  // In Compatibility profile, VAO number 0 is a valid and default vertex array
  // object, here we create the tracking data for it if it isn't created yet
  if (vaobj == 0) {
    auto& vertexArrays = SD().GetCurrentContextStateData().VertexArrays();
    if (!SD().IsCurrentContextCore() && vertexArrays.Get(0) == 0) {
      vertexArrays.Add(CVertexArraysStateObj(0));
    }
  }
}

inline void glVertexAttribFormat_SD(GLuint attribindex,
                                    GLint size,
                                    GLenum type,
                                    GLboolean normalized,
                                    GLuint relativeoffset,
                                    GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    GLuint vaobj;
    drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&vaobj);
    if (vaobj != 0 || !SD().IsCurrentContextCore()) {
      // A vertex array object is currently bound (Core profile) or track data
      // for VAO number 0 - valid and default VAO in Compatibility profile
      checkVertexArrayTrackingHelper(vaobj);
      SD().GetCurrentContextStateData().VertexArrays().Get(vaobj)->Data().track.attribs.insert(
          attribindex);
    }
  }
}

inline void glVertexAttribIFormat_SD(GLuint attribindex,
                                     GLint size,
                                     GLenum type,
                                     GLuint relativeoffset,
                                     GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    GLuint vaobj;
    drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&vaobj);
    if (vaobj != 0 || !SD().IsCurrentContextCore()) {
      // A vertex array object is currently bound (Core profile) or track data
      // for VAO number 0 - valid and default VAO in Compatibility profile
      checkVertexArrayTrackingHelper(vaobj);
      SD().GetCurrentContextStateData().VertexArrays().Get(vaobj)->Data().track.attribs.insert(
          attribindex);
    }
  }
}

inline void glVertexAttribLFormat_SD(GLuint attribindex,
                                     GLint size,
                                     GLenum type,
                                     GLuint relativeoffset,
                                     GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    GLuint vaobj;
    drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&vaobj);
    if (vaobj != 0 || !SD().IsCurrentContextCore()) {
      // A vertex array object is currently bound (Core profile) or track data
      // for VAO number 0 - valid and default VAO in Compatibility profile
      checkVertexArrayTrackingHelper(vaobj);
      SD().GetCurrentContextStateData().VertexArrays().Get(vaobj)->Data().track.attribs.insert(
          attribindex);
    }
  }
}

inline void glVertexArrayAttribFormat_SD(GLuint vaobj,
                                         GLuint attribindex,
                                         GLint size,
                                         GLenum type,
                                         GLboolean normalized,
                                         GLuint relativeoffset,
                                         GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (!SD().IsCurrentContextCore()) {
      checkVertexArrayTrackingHelper(vaobj);
    }
    SD().GetCurrentContextStateData().VertexArrays().Get(vaobj)->Data().track.attribs.insert(
        attribindex);
  }
}

inline void glVertexArrayAttribIFormat_SD(GLuint vaobj,
                                          GLuint attribindex,
                                          GLint size,
                                          GLenum type,
                                          GLuint relativeoffset,
                                          GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (!SD().IsCurrentContextCore()) {
      checkVertexArrayTrackingHelper(vaobj);
    }
    SD().GetCurrentContextStateData().VertexArrays().Get(vaobj)->Data().track.attribs.insert(
        attribindex);
  }
}

inline void glVertexArrayAttribLFormat_SD(GLuint vaobj,
                                          GLuint attribindex,
                                          GLint size,
                                          GLenum type,
                                          GLuint relativeoffset,
                                          GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (!SD().IsCurrentContextCore()) {
      checkVertexArrayTrackingHelper(vaobj);
    }
    SD().GetCurrentContextStateData().VertexArrays().Get(vaobj)->Data().track.attribs.insert(
        attribindex);
  }
}

inline void glLinkProgram_SD(GLuint program, GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV) || Config::Get().common.shared.useEvents) {
    SD().GetCurrentSharedStateData().GLSLPrograms().Get(program)->Link();
  }
}

inline void glNamedProgramLocalParameter4fvEXT_SD(GLuint program,
                                                  GLenum target,
                                                  GLuint index,
                                                  const GLfloat* params,
                                                  GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (SD().GetCurrentSharedStateData().ARBPrograms().Get(program) == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    SD().GetCurrentSharedStateData().ARBPrograms().Get(program)->LocalParameterUsed(index);
  }
}

inline void glNamedProgramLocalParameters4fvEXT_SD(GLuint program,
                                                   GLenum target,
                                                   GLuint index,
                                                   GLsizei count,
                                                   const GLfloat* params,
                                                   GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (SD().GetCurrentSharedStateData().ARBPrograms().Get(program) == 0) {
      Log(WARN) << "glNamedProgramLocalParameters4fvEXT called without program binding";
      return;
    }
    for (GLsizei i = 0; i < count; i++) {
      SD().GetCurrentSharedStateData().ARBPrograms().Get(program)->LocalParameterUsed(index + i);
    }
  }
}

inline void glNormalPointerEXT_SD(GLenum type,
                                  GLsizei stride,
                                  GLsizei count,
                                  const GLvoid* pointer,
                                  GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);
  }
}

inline void glPopClientAttrib_SD(GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentContextStateData().ClientArrays().RestoreClientAttribs();
  }
}

inline void glProgramEnvParameter4dARB_SD(GLenum target,
                                          GLuint index,
                                          GLdouble x,
                                          GLdouble y,
                                          GLdouble z,
                                          GLdouble w,
                                          GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().ARBProgEnvParams().Add(
        CARBProgramEnvParamsStateObj(target, index));
  }
}

inline void glProgramEnvParameter4dvARB_SD(GLenum target,
                                           GLuint index,
                                           const GLdouble* params,
                                           GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().ARBProgEnvParams().Add(
        CARBProgramEnvParamsStateObj(target, index));
  }
}

inline void glProgramEnvParameter4fARB_SD(GLenum target,
                                          GLuint index,
                                          GLfloat x,
                                          GLfloat y,
                                          GLfloat z,
                                          GLfloat w,
                                          GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().ARBProgEnvParams().Add(
        CARBProgramEnvParamsStateObj(target, index));
  }
}

inline void glProgramEnvParameter4fvARB_SD(GLenum target,
                                           GLuint index,
                                           const GLfloat* params,
                                           GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().ARBProgEnvParams().Add(
        CARBProgramEnvParamsStateObj(target, index));
  }
}

inline void glProgramEnvParameters4fvEXT_SD(GLenum target,
                                            GLuint index,
                                            GLsizei count,
                                            const GLfloat* params,
                                            GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    for (unsigned int i = index; i < (index + count); i++) {
      SD().GetCurrentSharedStateData().ARBProgEnvParams().Add(
          CARBProgramEnvParamsStateObj(target, i));
    }
  }
}

inline void glProgramLocalParameter4dARB_SD(GLenum target,
                                            GLuint index,
                                            GLdouble x,
                                            GLdouble y,
                                            GLdouble z,
                                            GLdouble w,
                                            GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    // set parameter as used for current program
    GLint programId = -1;
    drv.gl.glGetProgramivARB(target, GL_PROGRAM_BINDING_ARB, &programId);

    if (SD().GetCurrentSharedStateData().ARBPrograms().Get(programId) == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    SD().GetCurrentSharedStateData().ARBPrograms().Get(programId)->LocalParameterUsed(index);
  }
}

inline void glProgramLocalParameter4dvARB_SD(GLenum target,
                                             GLuint index,
                                             const GLdouble* params,
                                             GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    // set parameter as used for current program
    GLint programId = -1;
    drv.gl.glGetProgramivARB(target, GL_PROGRAM_BINDING_ARB, &programId);

    if (SD().GetCurrentSharedStateData().ARBPrograms().Get(programId) == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    SD().GetCurrentSharedStateData().ARBPrograms().Get(programId)->LocalParameterUsed(index);
  }
}

inline void glProgramLocalParameter4fARB_SD(GLenum target,
                                            GLuint index,
                                            GLfloat x,
                                            GLfloat y,
                                            GLfloat z,
                                            GLfloat w,
                                            GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    // set parameter as used for current program
    GLint programId = -1;
    drv.gl.glGetProgramivARB(target, GL_PROGRAM_BINDING_ARB, &programId);

    if (SD().GetCurrentSharedStateData().ARBPrograms().Get(programId) == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    SD().GetCurrentSharedStateData().ARBPrograms().Get(programId)->LocalParameterUsed(index);
  }
}

inline void glProgramLocalParameter4fvARB_SD(GLenum target,
                                             GLuint index,
                                             const GLfloat* params,
                                             GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    // set parameter as used for current program
    GLint programId = -1;
    drv.gl.glGetProgramivARB(target, GL_PROGRAM_BINDING_ARB, &programId);

    if (SD().GetCurrentSharedStateData().ARBPrograms().Get(programId) == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    SD().GetCurrentSharedStateData().ARBPrograms().Get(programId)->LocalParameterUsed(index);
  }
}

inline void glProgramLocalParameters4fvEXT_SD(GLenum target,
                                              GLuint index,
                                              GLsizei count,
                                              const GLfloat* params,
                                              GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    // set parameter as used for current program
    GLint programId = -1;
    drv.gl.glGetProgramivARB(target, GL_PROGRAM_BINDING_ARB, &programId);

    if (SD().GetCurrentSharedStateData().ARBPrograms().Get(programId) == 0) {
      Log(WARN) << "glProgramLocalParameters4fv called without program binding";
      return;
    }

    for (GLsizei i = 0; i < count; i++) {
      SD().GetCurrentSharedStateData().ARBPrograms().Get(programId)->LocalParameterUsed(index + i);
    }
  }
}

inline void glProgramParameteri_SD(GLuint program,
                                   GLenum pname,
                                   GLint value,
                                   GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV)) {
    if (pname == GL_PROGRAM_SEPARABLE) {
      CGLSLProgramStateObj* programStateObj =
          SD().GetCurrentSharedStateData().GLSLPrograms().Get(program);
      if (programStateObj == 0) {
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
      programStateObj->Data().track.isSepShaderObj = value;
    }
  }
}

inline void glRenderbufferStorageMultisample_SD(GLenum target,
                                                GLsizei samples,
                                                GLenum internalformat,
                                                GLsizei width,
                                                GLsizei height,
                                                GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    GLint rbo;
    drv.gl.glGetIntegerv(GL_RENDERBUFFER_BINDING, &rbo);
    CRenderbufferStateObj* renderbufferStateObj =
        SD().GetCurrentSharedStateData().Renderbuffers().Get(rbo);
    if (renderbufferStateObj != nullptr) {
      renderbufferStateObj->Data().track.samples = samples;
    }
  }
}

inline void glRenderbufferStorageMultisampleEXT_SD(GLenum target,
                                                   GLsizei samples,
                                                   GLenum internalformat,
                                                   GLsizei width,
                                                   GLsizei height,
                                                   GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    GLint rbo;
    drv.gl.glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, &rbo);
    CRenderbufferStateObj* renderbufferStateObj =
        SD().GetCurrentSharedStateData().RenderbuffersEXT().Get(rbo);
    if (renderbufferStateObj == 0 && rbo != 0) {
      // WA for Oculus Samsung Galaxy S7 and Unity3D 2018.1 engine
      renderbufferStateObj = SD().GetCurrentSharedStateData().Renderbuffers().Get(rbo);
    }
    if (renderbufferStateObj != nullptr) {
      renderbufferStateObj->Data().track.samples = samples;
    }
  }
}

inline void glShaderSource_SD(GLuint shader,
                              GLsizei count,
                              const GLchar* const* string,
                              const GLint* length,
                              GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV) || Config::Get().common.shared.useEvents) {
    // build source string
    std::stringstream str;
    for (int i = 0; i < count; ++i) {
      if (length == 0 || length[i] < 0) {
        str << std::string(string[i]);
      } else {
        str << std::string(string[i], string[i] + length[i]);
      }
    }
    std::string sourceString = str.str();
    // drop '13' resulting from untranslated newlines
    sourceString.erase(std::remove(sourceString.begin(), sourceString.end(), 13),
                       sourceString.end());
    SD().GetCurrentSharedStateData().GLSLShaders().Get(shader)->Source(sourceString);
  }
}

inline void glTransformFeedbackVaryings_SD(GLuint program,
                                           GLsizei count,
                                           const GLchar* const* varyings,
                                           GLenum bufferMode,
                                           GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV)) {
    CGLSLProgramStateObj* programStateObj =
        SD().GetCurrentSharedStateData().GLSLPrograms().Get(program);
    if (programStateObj == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    } else {
      programStateObj->Data().track._tfBufferMode = bufferMode;
      programStateObj->Data().track.transformVaryings.clear();
      for (int i = 0; i < count; i++) {
        programStateObj->Data().track.transformVaryings.push_back(std::string(varyings[i]));
      }
    }
  }
}

inline void glUnmapBuffer_SD(GLboolean return_value,
                             GLenum target,
                             GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    GLint buffer = boundBuff(target);
    CBufferStateObj* bufferState = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferState != nullptr) {
      bufferState->RemoveMapping();
    }
  }
}

inline void glUnmapNamedBuffer_SD(GLboolean return_value,
                                  GLuint buffer,
                                  GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    CBufferStateObj* bufferState = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferState != nullptr) {
      bufferState->RemoveMapping();
    }
  }
}

inline void glVertexAttribPointerNV_SD(GLuint index,
                                       GLint fsize,
                                       GLenum type,
                                       GLsizei stride,
                                       const GLvoid* pointer,
                                       GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);
  }
}

inline void glVertexPointerEXT_SD(GLint size,
                                  GLenum type,
                                  GLsizei stride,
                                  GLsizei count,
                                  const GLvoid* pointer,
                                  GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentContextStateData().ClientArrays().InterleavedArray().Enabled(false);
  }
}

inline void glEGLImageTargetTexture2DOES_SD(GLenum target,
                                            EGLImageKHR image,
                                            GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    GLint texture;
    drv.gl.glGetIntegerv(GetBindingEnum(target), &texture);
    CTextureStateObj* texStateObj =
        SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(texture, target));
    if (texStateObj == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    } else {
      texStateObj->Data().track.eglImage = image;
    }
  }
}

inline void glEGLImageTargetRenderbufferStorageOES_SD(GLenum target,
                                                      EGLImageKHR image,
                                                      GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    GLint renderbuffer;
    drv.gl.glGetIntegerv(GetBindingEnum(target), &renderbuffer);
    CRenderbufferStateObj* rboStateObj = SD().GetCurrentSharedStateData().Renderbuffers().Get(
        CRenderbufferStateObj(renderbuffer, target));
    if (rboStateObj == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    } else {
      rboStateObj->Data().track.eglImage = image;
    }
  }
}

inline void glCreateShaderProgramv_SD(GLuint return_value,
                                      GLenum type,
                                      GLsizei count,
                                      const GLchar* const* strings,
                                      GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder() || ShouldLog(TRACEV) || Config::Get().common.shared.useEvents) {
    CGLSLShaderStateObj tmpShader(0, type);
    // This shader data will be attached to shader as linked but deleted though
    // unique name will be generated in state restore. This is why here we can
    // assign name 0;
    // build source string
    std::stringstream str;
    for (int i = 0; i < count; ++i) {
      str << std::string(strings[i]);
    }
    std::string sourceString = str.str();
    tmpShader.Source(sourceString);
    tmpShader.Compile();

    SD().GetCurrentSharedStateData().GLSLPrograms().Add(CGLSLProgramStateObj(return_value));
    SD().GetCurrentSharedStateData().GLSLPrograms().Get(return_value)->Data().track.isSepShaderObj =
        true;
    SD().GetCurrentSharedStateData()
        .GLSLPrograms()
        .Get(return_value)
        ->AttachShader(tmpShader.DataShared());
    SD().GetCurrentSharedStateData().GLSLPrograms().Get(return_value)->Link();
  }
}

inline void glDeleteProgramPipelines_SD(GLsizei n,
                                        const GLuint* pipelines,
                                        GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentContextStateData().GLSLPipelines().Remove(unsigned(n), pipelines);
  }
}

inline void glGenProgramPipelines_SD(GLsizei n,
                                     GLuint* pipelines,
                                     GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentContextStateData().GLSLPipelines().Generate((unsigned)n, pipelines);
  }
}

inline void glCreateProgramPipelines_SD(GLsizei n,
                                        GLuint* pipelines,
                                        GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    SD().GetCurrentContextStateData().GLSLPipelines().Generate((unsigned)n, pipelines);
  }
}

inline void glUseProgramStages_SD(GLuint pipeline,
                                  GLbitfield stages,
                                  GLuint program,
                                  GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    CGLSLPipelineStateObj* pipelineStateObj =
        SD().GetCurrentContextStateData().GLSLPipelines().Get(pipeline);
    if (pipelineStateObj == 0) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    pipelineStateObj->UseProgramStage(stages, program);
  }
}

inline void glMapTexture2DINTEL_SD(void* return_value,
                                   GLuint texture,
                                   GLint level,
                                   GLbitfield access,
                                   GLint* stride,
                                   GLenum* layout,
                                   GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsPlayer()) {
    int textno = 0;
    drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_2D, &textno);
    drv.gl.glBindTexture(GL_TEXTURE_2D, texture);
    drv.gl.glBindTexture(GL_TEXTURE_2D, textno);
  }

  GLint compressed = 0;
  GLint size = 0;
  GLint height = 0;

  drv.gl.glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED, &compressed);
  if (compressed == GL_FALSE) {
    drv.gl.glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &height);
    size = (*stride) * height;
  } else {
    drv.gl.glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
  }
  SD().GetCurrentSharedStateData().GetMappedTextures().MapTexture(
      texture, level, return_value, size, access, stride, (int*)layout);

  if (Config::Get().IsRecorder()) {
    SD().GetCurrentSharedStateData().GetMappedTextures().InitializeTexture(texture, level);
  }
}

inline void glFramebufferRenderbuffer_SD(GLenum target,
                                         GLenum attachment,
                                         GLenum renderbuffertarget,
                                         GLuint renderbuffer,
                                         GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (renderbuffer != 0 && drv.gl.glIsRenderbuffer(renderbuffer)) {
      stateObjAddFBOAttachent(target, attachment);
    } else {
      stateObjRemoveFBOAttachment(target, attachment);
    }
  }
}

inline void glFramebufferRenderbufferEXT_SD(GLenum target,
                                            GLenum attachment,
                                            GLenum renderbuffertarget,
                                            GLuint renderbuffer,
                                            GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (renderbuffer != 0 && drv.gl.glIsRenderbufferEXT(renderbuffer)) {
      stateObjAddFBOEXTAttachent(target, attachment);
    } else {
      stateObjRemoveFBOEXTAttachent(target, attachment);
    }
  }
}

inline void glFramebufferTexture_SD(GLenum target,
                                    GLenum attachment,
                                    GLuint texture,
                                    GLint level,
                                    GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOAttachent(target, attachment);
    } else {
      stateObjRemoveFBOAttachment(target, attachment);
    }
  }
}

inline void glNamedFramebufferTexture_SD(GLuint framebuffer,
                                         GLenum attachment,
                                         GLuint texture,
                                         GLint level,
                                         GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOAttachent(0, attachment, framebuffer);
    } else {
      stateObjRemoveFBOAttachment(0, attachment, framebuffer);
    }
  }
}

inline void glFramebufferTexture1D_SD(GLenum target,
                                      GLenum attachment,
                                      GLenum textarget,
                                      GLuint texture,
                                      GLint level,
                                      GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOAttachent(target, attachment);
    } else {
      stateObjRemoveFBOAttachment(target, attachment);
    }
  }
}

inline void glFramebufferTexture1DEXT_SD(GLenum target,
                                         GLenum attachment,
                                         GLenum textarget,
                                         GLuint texture,
                                         GLint level,
                                         GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOEXTAttachent(target, attachment);
    } else {
      stateObjRemoveFBOEXTAttachent(target, attachment);
    }
  }
}

inline void glFramebufferTexture2D_SD(GLenum target,
                                      GLenum attachment,
                                      GLenum textarget,
                                      GLuint texture,
                                      GLint level,
                                      GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOAttachent(target, attachment);
    } else {
      stateObjRemoveFBOAttachment(target, attachment);
    }
  }
}

inline void glFramebufferTexture2DEXT_SD(GLenum target,
                                         GLenum attachment,
                                         GLenum textarget,
                                         GLuint texture,
                                         GLint level,
                                         GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOEXTAttachent(target, attachment);
    } else {
      stateObjRemoveFBOEXTAttachent(target, attachment);
    }
  }
}

inline void glFramebufferTexture3D_SD(GLenum target,
                                      GLenum attachment,
                                      GLenum textarget,
                                      GLuint texture,
                                      GLint level,
                                      GLint zoffset,
                                      GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOAttachent(target, attachment);
    } else {
      stateObjRemoveFBOAttachment(target, attachment);
    }
  }
}

inline void glFramebufferTexture3DEXT_SD(GLenum target,
                                         GLenum attachment,
                                         GLenum textarget,
                                         GLuint texture,
                                         GLint level,
                                         GLint zoffset,
                                         GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOEXTAttachent(target, attachment);
    } else {
      stateObjRemoveFBOEXTAttachent(target, attachment);
    }
  }
}

inline void glFramebufferTextureFaceARB_SD(GLenum target,
                                           GLenum attachment,
                                           GLuint texture,
                                           GLint level,
                                           GLenum face,
                                           GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOEXTAttachent(target, attachment);
    } else {
      stateObjRemoveFBOEXTAttachent(target, attachment);
    }
  }
}

inline void glFramebufferTextureFaceEXT_SD(GLenum target,
                                           GLenum attachment,
                                           GLuint texture,
                                           GLint level,
                                           GLenum face,
                                           GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOEXTAttachent(target, attachment);
    } else {
      stateObjRemoveFBOEXTAttachent(target, attachment);
    }
  }
}

inline void glFramebufferTextureLayer_SD(GLenum target,
                                         GLenum attachment,
                                         GLuint texture,
                                         GLint level,
                                         GLint layer,
                                         GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOAttachent(target, attachment);
    } else {
      stateObjRemoveFBOAttachment(target, attachment);
    }
  }
}

inline void glNamedFramebufferTextureLayer_SD(GLuint framebuffer,
                                              GLenum attachment,
                                              GLuint texture,
                                              GLint level,
                                              GLint layer,
                                              GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOAttachent(0, attachment, framebuffer);
    } else {
      stateObjRemoveFBOAttachment(0, attachment, framebuffer);
    }
  }
}

inline void glFramebufferTextureLayerEXT_SD(GLenum target,
                                            GLenum attachment,
                                            GLuint texture,
                                            GLint level,
                                            GLint layer,
                                            GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOEXTAttachent(target, attachment);
    } else {
      stateObjRemoveFBOEXTAttachent(target, attachment);
    }
  }
}

inline void glFramebufferTextureEXT_SD(GLenum target,
                                       GLenum attachment,
                                       GLuint texture,
                                       GLint level,
                                       GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    if (texture != 0) {
      stateObjAddFBOEXTAttachent(target, attachment);
    } else {
      stateObjRemoveFBOEXTAttachent(target, attachment);
    }
  }
}

inline void glGenerateMipmap_SD(GLenum target, GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    GenMipMapStateData(target);
  }
}

inline void glGenerateTextureMipmapEXT_SD(GLuint texture,
                                          GLenum target,
                                          GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    GenMipMapStateData((GLint)texture, target);
  }
}

namespace {
void TrackDrawBuffers(CObjectsList<CFramebufferStateObj>& Framebuffers, GLint fbo = -1) {
  GLint maxDrawBuffers;
  drv.gl.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

  GLint currentFbo;
  drv.gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFbo);
  if (-1 == fbo) {
    fbo = currentFbo;
  } else if (fbo != currentFbo) {
    drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  }

  if (fbo != 0) {
    CFramebufferStateObj* fboStateObj = Framebuffers.Get(fbo);
    if (fboStateObj != 0) {
      GLint tmpDrawBuffer;
      fboStateObj->Data().track.drawBuffers.resize(maxDrawBuffers);
      for (int i = 0; i < maxDrawBuffers; ++i) {
        drv.gl.glGetIntegerv(GL_DRAW_BUFFER0 + i, &tmpDrawBuffer);
        fboStateObj->Data().track.drawBuffers[i] = tmpDrawBuffer;
      }
    }
  }
  if (fbo != currentFbo) {
    drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, currentFbo);
  }
}
} // namespace

inline void glDrawBuffers_SD(GLsizei n, const GLenum* bufs, GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    TrackDrawBuffers(SD().GetCurrentContextStateData().FramebuffersEXT());
    TrackDrawBuffers(SD().GetCurrentContextStateData().Framebuffers());
  }
}

inline void glNamedFramebufferDrawBuffers_SD(GLuint framebuffer,
                                             GLsizei n,
                                             const GLenum* bufs,
                                             GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    TrackDrawBuffers(SD().GetCurrentContextStateData().Framebuffers(), framebuffer);
  }
}

inline void glFlushMappedNamedBufferRange_SD(GLuint buffer,
                                             GLintptr offset,
                                             GLsizeiptr length,
                                             GLboolean recording = 0 /*= false*/) {
  CBufferStateObj* vboStateObj = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
  GLsizeiptr bufferLength;
  if (vboStateObj->Data().restore.mapLength == -1) {
    bufferLength = vboStateObj->Data().track.size;
  } else {
    bufferLength = vboStateObj->Data().restore.mapLength;
  }

  if ((vboStateObj->Data().restore.mapAccess & GL_MAP_FLUSH_EXPLICIT_BIT) &&
      ((offset + length <= bufferLength) && offset >= 0 && length >= 0)) {
    vboStateObj->FlushMappedBufferRange(offset, length);
  }
}

inline void glFlushMappedBufferRange_SD(GLenum target,
                                        GLintptr offset,
                                        GLsizeiptr length,
                                        GLboolean recording = 0 /*= false*/) {
  glFlushMappedNamedBufferRange_SD(boundBuff(target), offset, length);
}

inline void glBlendEquationSeparatei_SD(GLuint buf,
                                        GLenum modeRGB,
                                        GLenum modeAlpha,
                                        GLboolean recording = 0 /*= false*/) {
  CBlendEquationiData::TBlendEquationiInfo& blendEqInfo =
      SD().GetCurrentContextStateData().BlendEquationi().BlendEquationiNameMapInfo()[buf];
  blendEqInfo.modeAlpha = modeAlpha;
  blendEqInfo.modeRGB = modeRGB;
  blendEqInfo.separate = true;
}

inline void glBlendEquationi_SD(GLuint buf, GLenum mode, GLboolean recording = 0 /*= false*/) {
  CBlendEquationiData::TBlendEquationiInfo& blendEqInfo =
      SD().GetCurrentContextStateData().BlendEquationi().BlendEquationiNameMapInfo()[buf];
  blendEqInfo.mode = mode;
  blendEqInfo.separate = false;
}

inline void glBlendFuncSeparatei_SD(GLuint buf,
                                    GLenum srcRGB,
                                    GLenum dstRGB,
                                    GLenum srcAlpha,
                                    GLenum dstAlpha,
                                    GLboolean recording = 0 /*= false*/) {
  CBlendFunciData::TBlendFunciInfo& blendFuncInfo =
      SD().GetCurrentContextStateData().BlendFunci().BlendFunciNameMapInfo()[buf];
  blendFuncInfo.srcRGB = srcRGB;
  blendFuncInfo.dstRGB = dstRGB;
  blendFuncInfo.srcAlpha = srcAlpha;
  blendFuncInfo.dstAlpha = dstAlpha;
  blendFuncInfo.separate = true;
}

inline void glBlendFunci_SD(GLuint buf,
                            GLenum src,
                            GLenum dst,
                            GLboolean recording = 0 /*= false*/) {
  CBlendFunciData::TBlendFunciInfo& blendFuncInfo =
      SD().GetCurrentContextStateData().BlendFunci().BlendFunciNameMapInfo()[buf];
  blendFuncInfo.dst = dst;
  blendFuncInfo.src = src;
  blendFuncInfo.separate = false;
}

inline void glBlendEquationSeparate_SD(GLenum modeRGB,
                                       GLenum modeAlpha,
                                       GLboolean recording = 0 /*= false*/) {
  auto& data = SD().GetCurrentContextStateData().GeneralStateObjects().Data();
  CGeneralStateData::Track::CBlendEquationSeparate& blendEquationSeparate =
      data.tracked.blendEquationSeparate;
  CGeneralStateData::Track::CBlendEquation& blendEquation = data.tracked.blendEquation;
  blendEquation.used = false;
  blendEquationSeparate.used = true;
  blendEquationSeparate.modeAlpha = modeAlpha;
  blendEquationSeparate.modeRGB = modeRGB;
}

inline void glBlendEquation_SD(GLenum mode, GLboolean recording = 0 /*= false*/) {
  auto& data = SD().GetCurrentContextStateData().GeneralStateObjects().Data();
  CGeneralStateData::Track::CBlendEquation& blendEquation = data.tracked.blendEquation;
  CGeneralStateData::Track::CBlendEquationSeparate& blendEquationSeparate =
      data.tracked.blendEquationSeparate;
  blendEquationSeparate.used = false;
  blendEquation.used = true;
  blendEquation.mode = mode;
}

inline void glBlendFuncSeparate_SD(GLenum srcRGB,
                                   GLenum dstRGB,
                                   GLenum srcAlpha,
                                   GLenum dstAlpha,
                                   GLboolean recording = 0 /*= false*/) {
  auto& data = SD().GetCurrentContextStateData().GeneralStateObjects().Data();
  CGeneralStateData::Track::CBlendFuncSeparate& blendFuncSeparate = data.tracked.blendFuncSeparate;
  CGeneralStateData::Track::CBlendFunc& blendFunc = data.tracked.blendFunc;
  blendFunc.used = false;
  blendFuncSeparate.used = true;
  blendFuncSeparate.srcRGB = srcRGB;
  blendFuncSeparate.dstRGB = dstRGB;
  blendFuncSeparate.srcAlpha = srcAlpha;
  blendFuncSeparate.dstAlpha = dstAlpha;
}

inline void glBlendFunc_SD(GLenum src, GLenum dst, GLboolean recording = 0 /*= false*/) {
  auto& data = SD().GetCurrentContextStateData().GeneralStateObjects().Data();
  CGeneralStateData::Track::CBlendFunc& blendFunc = data.tracked.blendFunc;
  CGeneralStateData::Track::CBlendFuncSeparate& blendFuncSeparate = data.tracked.blendFuncSeparate;
  blendFuncSeparate.used = false;
  blendFunc.used = true;
  blendFunc.dst = dst;
  blendFunc.src = src;
}

inline void glTexBuffer_SD(GLenum target,
                           GLenum internalformat,
                           GLuint buffer,
                           GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    GLint texture;
    drv.gl.glGetIntegerv(GetBindingEnum(target), &texture);
    CTextureStateObj* texStateObj =
        SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(texture, target));
    if (texStateObj == nullptr) {
      throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown texture");
    }
    texStateObj->Data().track.texbuffer_internalformat = internalformat;
    texStateObj->Data().track.texbuffer_buffer = buffer;
  }
}

inline void glTextureBuffer_SD(GLuint texture,
                               GLenum internalformat,
                               GLuint buffer,
                               GLboolean recording = 0 /*= false*/) {
  if (Config::Get().IsRecorder()) {
    CTextureStateObj* textureState = SD().GetCurrentSharedStateData().Textures().Get(texture);
    if (textureState == nullptr) {
      throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown texture");
    }
    GLenum target = textureState->Target();
    CTextureStateObj* texStateObj =
        SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(texture, target));
    if (texStateObj == nullptr) {
      throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown texture");
    }
    texStateObj->Data().track.texbuffer_internalformat = internalformat;
    texStateObj->Data().track.texbuffer_buffer = buffer;
  }
}
} // namespace OpenGL
} // namespace gits
