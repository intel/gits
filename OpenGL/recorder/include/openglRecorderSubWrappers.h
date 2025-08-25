// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openglRecorderSubWrappers.h
*
* @brief Automatically generated declarations of OpenGL library simple function call wrappers.
*
*/

#pragma once

#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "gits.h"
#include "log.h"
#include "exception.h"
#include "streams.h"
#include "recorder.h"
#include "openglFunction.h"
#include "clientArrays.h"
#include "eglArguments.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "stateObjects.h"
#include "stateObjects.h"
#include "openglLibrary.h"
#include "glFunctions.h"
#include "openglState.h"

#include "gitsFunctions.h"
#include "eglFunctions.h"
#include "wglFunctions.h"
#include "glxFunctions.h"
#include "openglRecorderConditions.h"

namespace gits {
namespace OpenGL {
void RestoreDisplayListState(CRecorder& recorder);

inline void glDeleteObjectARB_RECWRAP(GLhandleARB obj, CRecorder& recorder) {
  if (SD().GetCurrentContext() == 0) {
    LOG_WARNING << "glDeleteObjectARB called while Current context is 0 - call ommited (workaround "
                   "for X-plane 10).";
    return;
  } else if (Recording(recorder)) {
    recorder.Schedule(new CglDeleteObjectARB(obj));
  }
  glDeleteObjectARB_SD(obj, Recording(recorder));
}

inline void glNewList_RECWRAP(GLuint list, GLenum mode, CRecorder& recorder) {
  if (!Recording(recorder)) {
    RestoreDisplayListState(CRecorder::Instance());
  }

  CRecorder::Instance().RecordingOverride(true);

  if (Recording(recorder)) {
    recorder.Schedule(new CglNewList(list, mode));
  }
}

inline void glEndList_RECWRAP(CRecorder& recorder) {
  if (Recording(recorder)) {
    recorder.Schedule(new CglEndList);
  }

  CRecorder::Instance().RecordingOverride(false);
}

inline void glEnableVertexAttribArray_RECWRAP(GLuint index, CRecorder& recorder) {
  // Input index value validation
  GLint maxAttribs;
  drv.gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
  if (index <= (GLuint)maxAttribs && Recording(recorder)) {
    recorder.Schedule(new CglEnableVertexAttribArray(index));
  }
  glEnableVertexAttribArray_SD(index, Recording(recorder));
}

inline void glEnableVertexAttribArrayARB_RECWRAP(GLuint index, CRecorder& recorder) {
  // Input index value validation
  GLint maxAttribs;
  drv.gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
  if (index <= (GLuint)maxAttribs && Recording(recorder)) {
    recorder.Schedule(new CglEnableVertexAttribArrayARB(index));
  }
  glEnableVertexAttribArray_SD(index, Recording(recorder));
}

inline void glDisableVertexAttribArray_RECWRAP(GLuint index, CRecorder& recorder) {
  // Input index value validation
  GLint maxAttribs;
  drv.gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
  if (index <= (GLuint)maxAttribs && Recording(recorder)) {
    recorder.Schedule(new CglDisableVertexAttribArray(index));
  }

  glDisableVertexAttribArray_SD(index, Recording(recorder));
}

inline void glDisableVertexAttribArrayARB_RECWRAP(GLuint index, CRecorder& recorder) {
  // Input index value validation
  GLint maxAttribs;
  drv.gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
  if (index <= (GLuint)maxAttribs && Recording(recorder)) {
    recorder.Schedule(new CglDisableVertexAttribArrayARB(index));
  }

  glDisableVertexAttribArray_SD(index, Recording(recorder));
}

inline void glDeleteShader_RECWRAP(GLuint shader, CRecorder& recorder) {
  CGLSLShaderStateObj* shaderStateObjPtr =
      SD().GetCurrentSharedStateData().GLSLShaders().Get(shader);
  if (shaderStateObjPtr == 0) {
    LOG_WARNING << "Deleted shader object: " << shader << " was not found. Ignoring function";
    return;
  }

  if (Recording(recorder)) {
    recorder.Schedule(new CglDeleteShader(shader));
  }

  if (shaderStateObjPtr->IsAttached()) {
    shaderStateObjPtr->MarkToDelete();
  } else {
    SD().GetCurrentSharedStateData().GLSLShaders().Remove(1, &shader);
  }
}
inline void glBindFramebuffer_RECWRAP(GLenum target, GLuint framebuffer, CRecorder& recorder) {
  if (Recording(recorder)) {
    recorder.Schedule(new CglBindFramebuffer(target, framebuffer));
  }

  glBindFramebuffer_SD(target, framebuffer, Recording(recorder));
  if (!SD().GetCurrentContextStateData().FramebuffersEXT().List().empty()) {
    CALL_ONCE[] {
      LOG_WARNING << "Mixed Framebuffers calls (EXT and Core). Consider enabling "
                     "ScheduleFboEXTAsCoreWA in recorder config.";
    };
  }
}

inline void glBindFramebufferEXT_RECWRAP(GLenum target, GLuint framebuffer, CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    glBindFramebuffer_RECWRAP(target, framebuffer, recorder);
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(new CglBindFramebufferEXT(target, framebuffer));
    }

    glBindFramebufferEXT_SD(target, framebuffer, Recording(recorder));
    if (!SD().GetCurrentContextStateData().Framebuffers().List().empty()) {
      CALL_ONCE[] {
        LOG_WARNING << "Mixed Framebuffers calls (EXT and Core). Consider enabling "
                       "ScheduleFboEXTAsCoreWA in recorder config.";
      };
    }
  }
}

inline void glBindFramebufferOES_RECWRAP(GLenum target, GLuint framebuffer, CRecorder& recorder) {
  if (Recording(recorder)) {
    recorder.Schedule(new CglBindFramebufferOES(target, framebuffer));
  }

  glBindFramebufferEXT_SD(target, framebuffer, Recording(recorder));
  if (!SD().GetCurrentContextStateData().Framebuffers().List().empty()) {
    CALL_ONCE[] {
      LOG_WARNING << "Mixed Framebuffers calls (EXT and Core). Consider enabling "
                     "ScheduleFboEXTAsCoreWA in recorder config.";
    };
  }
}

inline void glBindRenderbufferEXT_RECWRAP(GLenum target, GLuint renderbuffer, CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglBindRenderbuffer(target, renderbuffer));
    }

    glBindRenderbuffer_SD(target, renderbuffer, Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(new CglBindRenderbufferEXT(target, renderbuffer));
    }

    glBindRenderbufferEXT_SD(target, renderbuffer, Recording(recorder));
  }
}

inline void glCheckFramebufferStatusEXT_RECWRAP(GLenum return_value,
                                                GLenum target,
                                                CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglCheckFramebufferStatus(return_value, target));
    }
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(new CglCheckFramebufferStatusEXT(return_value, target));
    }
  }
}

inline void glDeleteFramebuffersEXT_RECWRAP(GLsizei n,
                                            const GLuint* framebuffers,
                                            CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglDeleteFramebuffers(n, framebuffers));
    }

    glDeleteFramebuffers_SD(n, framebuffers, Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(new CglDeleteFramebuffersEXT(n, framebuffers));
    }

    glDeleteFramebuffersEXT_SD(n, framebuffers, Recording(recorder));
  }
}

inline void glDeleteRenderbuffersEXT_RECWRAP(GLsizei n,
                                             const GLuint* renderbuffers,
                                             CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglDeleteRenderbuffers(n, renderbuffers));
    }

    glDeleteRenderbuffers_SD(n, renderbuffers, Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(new CglDeleteRenderbuffersEXT(n, renderbuffers));
    }

    glDeleteRenderbuffersEXT_SD(n, renderbuffers, Recording(recorder));
  }
}

inline void glFinish_RECWRAP(CRecorder& recorder) {
  if (Recording(recorder)) {
    recorder.Schedule(new CglFinish());
  }

  if (Configurator::Get().opengl.recorder.frames.frameSeparators.glFinishSep) {
    recorder.FrameEnd();
  }
}

inline void glFlush_RECWRAP(CRecorder& recorder) {
  if (Recording(recorder)) {
    recorder.Schedule(new CglFlush());
  }

  if (Configurator::Get().opengl.recorder.frames.frameSeparators.glFlushSep) {
    recorder.FrameEnd();
  }
}

inline void glFramebufferRenderbufferEXT_RECWRAP(GLenum target,
                                                 GLenum attachment,
                                                 GLenum renderbuffertarget,
                                                 GLuint renderbuffer,
                                                 CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(
          new CglFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer));
    }

    glFramebufferRenderbuffer_SD(target, attachment, renderbuffertarget, renderbuffer,
                                 Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(
          new CglFramebufferRenderbufferEXT(target, attachment, renderbuffertarget, renderbuffer));
    }

    glFramebufferRenderbufferEXT_SD(target, attachment, renderbuffertarget, renderbuffer,
                                    Recording(recorder));
  }
}

inline void glFramebufferTexture1DEXT_RECWRAP(GLenum target,
                                              GLenum attachment,
                                              GLenum textarget,
                                              GLuint texture,
                                              GLint level,
                                              CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglFramebufferTexture1D(target, attachment, textarget, texture, level));
    }

    glFramebufferTexture1D_SD(target, attachment, textarget, texture, level, Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(
          new CglFramebufferTexture1DEXT(target, attachment, textarget, texture, level));
    }

    glFramebufferTexture1DEXT_SD(target, attachment, textarget, texture, level,
                                 Recording(recorder));
  }
}

inline void glFramebufferTexture2DEXT_RECWRAP(GLenum target,
                                              GLenum attachment,
                                              GLenum textarget,
                                              GLuint texture,
                                              GLint level,
                                              CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglFramebufferTexture2D(target, attachment, textarget, texture, level));
    }

    glFramebufferTexture2D_SD(target, attachment, textarget, texture, level, Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(
          new CglFramebufferTexture2DEXT(target, attachment, textarget, texture, level));
    }

    glFramebufferTexture2DEXT_SD(target, attachment, textarget, texture, level,
                                 Recording(recorder));
  }
}

inline void glFramebufferTexture3DEXT_RECWRAP(GLenum target,
                                              GLenum attachment,
                                              GLenum textarget,
                                              GLuint texture,
                                              GLint level,
                                              GLint zoffset,
                                              CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(
          new CglFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset));
    }

    glFramebufferTexture3D_SD(target, attachment, textarget, texture, level, zoffset,
                              Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(
          new CglFramebufferTexture3DEXT(target, attachment, textarget, texture, level, zoffset));
    }

    glFramebufferTexture3DEXT_SD(target, attachment, textarget, texture, level, zoffset,
                                 Recording(recorder));
  }
}

inline void glFramebufferTextureEXT_RECWRAP(
    GLenum target, GLenum attachment, GLuint texture, GLint level, CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglFramebufferTexture(target, attachment, texture, level));
    }

    glFramebufferTexture_SD(target, attachment, texture, level, Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(new CglFramebufferTextureEXT(target, attachment, texture, level));
    }

    glFramebufferTextureEXT_SD(target, attachment, texture, level, Recording(recorder));
  }
}

inline void glFramebufferTextureFaceEXT_RECWRAP(GLenum target,
                                                GLenum attachment,
                                                GLuint texture,
                                                GLint level,
                                                GLenum face,
                                                CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglFramebufferTextureFaceARB(target, attachment, texture, level, face));
    }

    glFramebufferTextureFaceARB_SD(target, attachment, texture, level, face, Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(new CglFramebufferTextureFaceEXT(target, attachment, texture, level, face));
    }

    glFramebufferTextureFaceEXT_SD(target, attachment, texture, level, face, Recording(recorder));
  }
}

inline void glFramebufferTextureLayerEXT_RECWRAP(GLenum target,
                                                 GLenum attachment,
                                                 GLuint texture,
                                                 GLint level,
                                                 GLint layer,
                                                 CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglFramebufferTextureLayer(target, attachment, texture, level, layer));
    }

    glFramebufferTextureLayer_SD(target, attachment, texture, level, layer, Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(
          new CglFramebufferTextureLayerEXT(target, attachment, texture, level, layer));
    }

    glFramebufferTextureLayerEXT_SD(target, attachment, texture, level, layer, Recording(recorder));
  }
}

inline void glGenFramebuffersEXT_RECWRAP(GLsizei n, GLuint* framebuffers, CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglGenFramebuffers(n, framebuffers));
    }

    glGenFramebuffers_SD(n, framebuffers, Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(new CglGenFramebuffersEXT(n, framebuffers));
    }
  }
}

inline void glGenRenderbuffersEXT_RECWRAP(GLsizei n, GLuint* renderbuffers, CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglGenRenderbuffers(n, renderbuffers));
    }

    glGenRenderbuffers_SD(n, renderbuffers, Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(new CglGenRenderbuffersEXT(n, renderbuffers));
    }

    glGenRenderbuffersEXT_SD(n, renderbuffers, Recording(recorder));
  }
}

inline void glGenerateMipmapEXT_RECWRAP(GLenum target, CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (ConditionTextureES(recorder)) {
      recorder.Schedule(new CglGenerateMipmap(target));
    }

    glGenerateMipmap_SD(target, Recording(recorder));
  } else {
    if (ConditionTextureES(recorder)) {
      recorder.Schedule(new CglGenerateMipmapEXT(target));
    }

    glGenerateMipmap_SD(target, Recording(recorder));
  }
}

inline void glGetActiveUniformBlockName_RECWRAP(GLuint program,
                                                GLuint uniformBlockIndex,
                                                GLsizei bufSize,
                                                GLsizei* length,
                                                GLchar* uniformBlockName,
                                                CRecorder& recorder) {
  // WA for applications which do not query uniform block index via
  // glGetUniformBlockIndex call but use values returned by
  // glGetActiveUniformsiv without block index mapping
  auto return_value = drv.gl.glGetUniformBlockIndex(program, uniformBlockName);
  if (Recording(recorder)) {
    recorder.Schedule(new CglGetUniformBlockIndex(return_value, program, uniformBlockName));
    recorder.Schedule(new CglGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length,
                                                       uniformBlockName));
  }
}

inline void glIsFramebufferEXT_RECWRAP(GLboolean return_value,
                                       GLuint framebuffer,
                                       CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglIsFramebuffer(return_value, framebuffer));
    }
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(new CglIsFramebufferEXT(return_value, framebuffer));
    }
  }
}

inline void glIsRenderbufferEXT_RECWRAP(GLboolean return_value,
                                        GLuint renderbuffer,
                                        CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglIsRenderbuffer(return_value, renderbuffer));
    }
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(new CglIsRenderbufferEXT(return_value, renderbuffer));
    }
  }
}

inline void glRenderbufferStorageEXT_RECWRAP(
    GLenum target, GLenum internalformat, GLsizei width, GLsizei height, CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(new CglRenderbufferStorage(target, internalformat, width, height));
    }
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(new CglRenderbufferStorageEXT(target, internalformat, width, height));
    }
  }
}

inline void glRenderbufferStorageMultisampleEXT_RECWRAP(GLenum target,
                                                        GLsizei samples,
                                                        GLenum internalformat,
                                                        GLsizei width,
                                                        GLsizei height,
                                                        CRecorder& recorder) {
  if (isSchedulefboEXTAsCoreWA()) {
    if (Recording(recorder)) {
      recorder.Schedule(
          new CglRenderbufferStorageMultisample(target, samples, internalformat, width, height));
    }

    glRenderbufferStorageMultisample_SD(target, samples, internalformat, width, height,
                                        Recording(recorder));
  } else {
    if (Recording(recorder)) {
      recorder.Schedule(
          new CglRenderbufferStorageMultisampleEXT(target, samples, internalformat, width, height));
    }

    glRenderbufferStorageMultisampleEXT_SD(target, samples, internalformat, width, height,
                                           Recording(recorder));
  }
}

inline void glViewport_RECWRAP(
    GLint x, GLint y, GLsizei width, GLsizei height, CRecorder& recorder) {
  if (Recording(recorder)) {
#ifdef GITS_PLATFORM_WINDOWS
    HWND hwnd = WindowFromDC(drv.wgl.wglGetCurrentDC());
    int windowThread = CStateDynamicNative::Get().GetWindowThread(hwnd);
    int currentThread = CGits::Instance().CurrentThreadId();
    recorder.Schedule(new CTokenMakeCurrentThreadNoCtxSwitch(windowThread));
#endif
    recorder.Schedule(new CgitsViewportSettings(x, y, width, height));
#ifdef GITS_PLATFORM_WINDOWS
    recorder.Schedule(new CTokenMakeCurrentThreadNoCtxSwitch(currentThread));
#endif
    recorder.Schedule(new CglViewport(x, y, width, height));
  }
}
inline void glLinkProgram_RECWRAP(GLuint program, CRecorder& recorder) {
  if (Recording(recorder)) {
    recorder.Schedule(new CgitsLinkProgramAttribsSetting(program));
    recorder.Schedule(new CglLinkProgram(program));
    if ((curctx::IsOgl() && curctx::Version() >= 430) || (curctx::IsEs31Plus()) ||
        (drv.gl.HasExtension("GL_ARB_program_interface_query") &&
         drv.gl.HasExtension("GL_ARB_shader_storage_buffer_object"))) {
      recorder.Schedule(new CgitsLinkProgramBuffersSetting(program));
    }
  }
  glLinkProgram_SD(program, Recording(recorder));
}
inline void glLinkProgramARB_RECWRAP(GLuint program, CRecorder& recorder) {
  if (Recording(recorder)) {
    recorder.Schedule(new CgitsLinkProgramAttribsSetting(program));
    recorder.Schedule(new CglLinkProgramARB(program));
    if ((curctx::IsOgl() && curctx::Version() >= 430) || (curctx::IsEs31Plus()) ||
        (drv.gl.HasExtension("GL_ARB_program_interface_query") &&
         drv.gl.HasExtension("GL_ARB_shader_storage_buffer_object"))) {
      recorder.Schedule(new CgitsLinkProgramBuffersSetting(program));
    }
  }
  glLinkProgram_SD(program, Recording(recorder));
}
} // namespace OpenGL
} // namespace gits
