// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openglRecorderConditions.h
*
* @brief Automatically generated declarations of OpenGL library simple function call wrappers.
*
*/

#pragma once

#include "gits.h"
#include "log.h"
#include "exception.h"
#include "streams.h"
#include "platform.h"
#include "recorder.h"
#include "openglFunction.h"
#include "clientArrays.h"
#include "eglArguments.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "stateObjects.h"
#include "stateObjects.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "openglState.h"

#include "gitsFunctions.h"
#include "eglFunctions.h"
#include "wglFunctions.h"
#include "glxFunctions.h"

namespace gits {
inline bool Recording(gits::CRecorder& recorder) {
  return recorder.Running() && (gits::OpenGL::SD().GetCurrentContext() != 0);
}

namespace OpenGL {
inline bool ConditionTextureES(CRecorder& recorder) {
  bool force = !curctx::IsOgl() && !IsGlGetTexAndCompressedTexImagePresentOnGLES();
  if (Config::Get().recorder.openGL.utilities.texturesState ==
      TTexturesState::TEXTURES_STATE_RESTORE) {
    force = false;
  }

  return (force || Recording(recorder));
}

inline bool ConditionBufferES(CRecorder& recorder) {
  bool force = (!curctx::IsOgl() && !IsGlGetTexAndCompressedTexImagePresentOnGLES()) ||
               Config::Get().recorder.openGL.utilities.forceBuffersStateCaptureAlwaysWA;
  if (ESBufferState() == TBuffersState::BUFFERS_STATE_RESTORE) {
    force = false;
  }

  return (force || Recording(recorder));
}

inline bool ConditionBufferData(
    CRecorder& recorder, GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) {
  bool force = (!curctx::IsOgl() && !IsGlGetTexAndCompressedTexImagePresentOnGLES()) ||
               Config::Get().recorder.openGL.utilities.forceBuffersStateCaptureAlwaysWA;
  bool skip = false;
  if (size < 0) {
    Log(WARN) << "Skipping glBufferData - size < 0";
    skip = true;
  }

  if (target == 0) {
    Log(WARN) << "Skipping glBufferData - target = 0";
    skip = true;
  }

  if (ESBufferState() == TBuffersState::BUFFERS_STATE_RESTORE) {
    force = false;
  }

  return ((force || Recording(recorder)) && !skip);
}

inline bool ConditionBufferStorage(
    CRecorder& recorder, GLenum target, GLsizeiptr size, const GLvoid* data, GLbitfield flags) {
  bool force = (!curctx::IsOgl() && !IsGlGetTexAndCompressedTexImagePresentOnGLES()) ||
               Config::Get().recorder.openGL.utilities.forceBuffersStateCaptureAlwaysWA;
  bool skip = false;
  if (size < 0) {
    Log(WARN) << "Skipping glBufferStorage - size < 0";
    skip = true;
  }
  if (target == 0) {
    Log(WARN) << "Skipping glBufferStorage - target = 0";
    skip = true;
  }

  if (ESBufferState() == TBuffersState::BUFFERS_STATE_RESTORE) {
    force = false;
  }

  return ((force || Recording(recorder)) && !skip);
}

inline bool ConditionTexImageES(CRecorder& recorder, GLenum format, GLenum type) {
  bool force = !curctx::IsOgl() && !IsGlGetTexImagePresentOnGLES();
  bool skip = false;

  if (Config::Get().recorder.openGL.utilities.texturesState ==
      TTexturesState::TEXTURES_STATE_RESTORE) {
    force = false;
  }

  if (format <= 0 || type <= 0) { //WA for Antutu Android Benchmark invalid enums;
    Log(WARN) << "glTexImage2D call skipped due to invalid enum values.";
    skip = true;
  }

  return ((force || Recording(recorder)) && !skip);
}

inline bool ConditionTexParamES(CRecorder& recorder, GLenum pname) {
  bool force = !curctx::IsOgl() && pname == GL_GENERATE_MIPMAP &&
               !IsGlGetTexAndCompressedTexImagePresentOnGLES();
  return (force || Recording(recorder));
}
} // namespace OpenGL
} // namespace gits
