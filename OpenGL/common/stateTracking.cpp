// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   stateTracking.cpp
*
* @brief Automatically generated definitions of OpenGL library simple function call wrappers.
*
*/

#include "stateTracking.h"
#include "stateObjects.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "gits.h"
#include "exception.h"
#include "log.h"
#include "streams.h"
#include "platform.h"

#include <memory>

GLboolean gits::OpenGL::checkBuff() {
  GLint buffer = SD().GetCurrentContextStateData().Bindings().BoundBuffers()[GL_ARRAY_BUFFER];
  return buffer != 0;
}

void gits::OpenGL::setTargetForBuff(GLenum target, GLuint buffer) {
  CBufferStateObj* bufferStateObj = SD().GetCurrentSharedStateData().Buffers().Get(buffer);

  if (bufferStateObj != nullptr) {
    bufferStateObj->SetTarget(target);
  } else {
    // In compatibility context, this may be the case
    SD().GetCurrentSharedStateData().Buffers().Add(CBufferStateObj(buffer, target));
  }
}

GLint gits::OpenGL::boundBuff(GLenum target) {
  GLint boundBuffer = -1;
  if (!curctx::IsOgl11()) {
    boundBuffer = SD().GetCurrentContextStateData().Bindings().BoundBuffers()[target];
  } else {
    boundBuffer = 0;
  }

  if (boundBuffer == -1) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return boundBuffer;
}

void gits::OpenGL::updateIndexedTargetInfo(
    GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
  // Track generic binding point
  setTargetForBuff(target, buffer);

  // Track indexed binding point
  CIndexedBindingRangeStateData::TIndexInfo& indexInfo =
      SD().GetCurrentSharedStateData().IndexedBoundBuffers().TargetsInfo()[target][index];
  indexInfo.buffer = buffer;
  indexInfo.offset = offset;
  indexInfo.size = size;
}

void gits::OpenGL::ESCompressedTexDataTrack(GLenum target,
                                            GLint level,
                                            GLsizei imagesize,
                                            const GLvoid* data) {
  using namespace gits::OpenGL;
  //Track compressed textures data on ES if textures restoration option enabled
  //This data is restored in openglState for OpenGL.
  if (Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
    if (GL_TEXTURE_CUBE_MAP_POSITIVE_X <= target && GL_TEXTURE_CUBE_MAP_NEGATIVE_Z >= target) {
      auto textureRestoreData = std::static_pointer_cast<CTextureStateData::CTextureCubeData>(
          TextureStateObject(target).Data().restore.ptr);
      //Extend vector to proper size
      while ((GLint)textureRestoreData->pixels[target - GL_TEXTURE_CUBE_MAP_POSITIVE_X].size() <=
             level) {
        textureRestoreData->pixels[target - GL_TEXTURE_CUBE_MAP_POSITIVE_X].push_back(0);
      }
      //Save data and hash
      textureRestoreData->pixels[target - GL_TEXTURE_CUBE_MAP_POSITIVE_X].at(level) =
          CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, data, imagesize);
    } else {
      auto textureRestoreData = std::static_pointer_cast<CTextureStateData::CTextureNDData>(
          TextureStateObject(target).Data().restore.ptr);
      //Extend vector to proper size
      while ((GLint)textureRestoreData->pixels.size() <= level) {
        textureRestoreData->pixels.push_back(0);
      }
      //Save data and hash
      textureRestoreData->pixels.at(level) =
          CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, data, imagesize);
    }
  }
}

void gits::OpenGL::stateObjAddFBOAttachent(GLenum target, GLenum attachment, GLint fbo) {
  if (-1 == fbo) {
    drv.gl.glGetIntegerv(GetBindingEnum(target), &fbo);
  }
  CFramebufferStateObj* fboStateObj = SD().GetCurrentContextStateData().Framebuffers().Get(fbo);
  if (fbo != 0 && fboStateObj != nullptr) {
    fboStateObj->AddAttachment(attachment);
  }
}

void gits::OpenGL::stateObjRemoveFBOAttachment(GLenum target, GLenum attachment, GLint fbo) {
  if (-1 == fbo) {
    drv.gl.glGetIntegerv(GetBindingEnum(target), &fbo);
  }
  CFramebufferStateObj* fboStateObj = SD().GetCurrentContextStateData().Framebuffers().Get(fbo);
  if (fbo != 0 && fboStateObj != nullptr) {
    fboStateObj->RemoveAttachment(attachment);
  }
}

void gits::OpenGL::stateObjAddFBOEXTAttachent(GLenum target, GLenum attachment) {
  GLint fbo;
  drv.gl.glGetIntegerv(GetBindingEnum(target), &fbo);
  CFramebufferStateObj* fboStateObj = SD().GetCurrentContextStateData().FramebuffersEXT().Get(fbo);
  if (fbo != 0 && fboStateObj != nullptr) {
    fboStateObj->AddAttachment(attachment);
  }
}

void gits::OpenGL::stateObjRemoveFBOEXTAttachent(GLenum target, GLenum attachment) {
  GLint fbo;
  drv.gl.glGetIntegerv(GetBindingEnum(target), &fbo);
  CFramebufferStateObj* fboStateObj = SD().GetCurrentContextStateData().FramebuffersEXT().Get(fbo);
  if (fbo != 0 && fboStateObj != nullptr) {
    fboStateObj->RemoveAttachment(attachment);
  }
}

void gits::OpenGL::OptionalBufferDataTrack(
    GLint buffer, GLintptr offset, GLsizeiptr size, const GLvoid* data, bool recording) {
  if (buffer == 0) {
    return;
  }
  //For OpenGL ES we need to track buffer data by default.
  //In case of optimizeBuffersz option we need to track buffer changes during recording for mapped memory changes detection performance.
  if ((!curctx::IsOgl() && ESBufferState() != TBuffersState::RESTORE &&
       !IsGlGetTexAndCompressedTexImagePresentOnGLES()) ||
      (recording && Config::Get().opengl.recorder.optimizeBufferSize)) {
    auto* bufferStateData = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferStateData != nullptr) {
      bufferStateData->TrackBufferData(offset, size, data);
    }
  }
}

void gits::OpenGL::attributeIndexTrack(GLuint index) {
  bool versionAtLeast3 = curctx::IsEs3Plus() || (curctx::IsOgl() && curctx::Version() >= 300);
  if (Config::Get().IsRecorder() && versionAtLeast3) {
    GLuint vaobj = 0;
    drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&vaobj);
    if (vaobj || !SD().IsCurrentContextCore()) {
      checkVertexArrayTrackingHelper(vaobj);
      auto& currCtxStateData = SD().GetCurrentContextStateData();
      auto& vaobjData = currCtxStateData.VertexArrays().Get(vaobj)->Data();
      vaobjData.track.attribs.insert(index);
    }
  }
}

std::vector<uint8_t> gits::OpenGL::GetBufferData(GLenum target, GLintptr offset, GLsizeiptr size) {
  std::vector<uint8_t> tmpData(size);
  uint8_t* pSrc = (uint8_t*)drv.gl.glMapBufferRange(target, offset, size, GL_MAP_READ_BIT);
  if (pSrc == nullptr) {
    Log(ERR) << "glMapBufferRange failed, cannot get data from GPU-side buffer.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  memcpy(tmpData.data(), pSrc, size);
  drv.gl.glUnmapBuffer(target);

  return tmpData;
}

std::vector<uint8_t> gits::OpenGL::GetNamedBufferData(GLuint buffer,
                                                      GLintptr offset,
                                                      GLsizeiptr size) {
  std::vector<uint8_t> tmpData(size);
  uint8_t* pSrc = (uint8_t*)drv.gl.glMapNamedBufferRange(buffer, offset, size, GL_MAP_READ_BIT);
  if (pSrc == nullptr) {
    Log(ERR) << "glMapNamedBufferRange failed, cannot get data from GPU-side buffer.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  memcpy(tmpData.data(), pSrc, size);
  drv.gl.glUnmapNamedBuffer(buffer);

  return tmpData;
}
