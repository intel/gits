// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   opengl_apis_iface.h
 *
 * @brief Declaration  of class that passes generic opengl data to the lower, generic layers.
 *
 */

#pragma once
#include "apis_iface.h"

namespace gits {
namespace OpenGL {
class OpenGLApi : public ApisIface::Api3d {
public:
  OpenGLApi() : Api3d(ApisIface::OpenGL) {}
  virtual ~OpenGLApi() {}
  virtual bool CfgRec_IsAllMode() const {
    return Configurator::Get().opengl.recorder.mode == TOpenGLRecorderMode::ALL;
  }
  virtual unsigned CfgRec_ExitFrame() const {
    return Configurator::Get().opengl.recorder.all.exitFrame;
  }
  virtual bool CfgRec_IsFramesMode() const {
    return Configurator::Get().opengl.recorder.mode == TOpenGLRecorderMode::FRAMES;
  }
  virtual int CfgRec_StartFrame() const {
    return Configurator::Get().opengl.recorder.frames.startFrame;
  }
  virtual int CfgRec_StopFrame() const {
    return Configurator::Get().opengl.recorder.frames.stopFrame;
  }
  virtual const std::vector<unsigned>& CfgRec_StartKeys() const {
    return Configurator::Get().opengl.recorder.frames.startKeys;
  }
  virtual bool CfgRec_IsSingleDrawMode() const {
    return Configurator::Get().opengl.recorder.mode == TOpenGLRecorderMode::SINGLE_DRAW;
  }
  virtual int CfgRec_SingleDraw() const {
    return Configurator::Get().opengl.recorder.oglSingleDraw.number;
  }
  virtual bool CfgRec_IsDrawsRangeMode() const {
    return Configurator::Get().opengl.recorder.mode == TOpenGLRecorderMode::DRAWS_RANGE;
  }
  virtual int CfgRec_StartDraw() const {
    return Configurator::Get().opengl.recorder.oglDrawsRange.startDraw;
  }
  virtual int CfgRec_StopDraw() const {
    return Configurator::Get().opengl.recorder.oglDrawsRange.stopDraw;
  }
  virtual int CfgRec_Frame() const {
    return Configurator::Get().opengl.recorder.oglDrawsRange.frame;
  }
  virtual unsigned CfgRec_EndFrameSleep() const {
    return Configurator::Get().opengl.recorder.endFrameSleep;
  }
  virtual void Play_SwapAfterPrepare() const;
};
} // namespace OpenGL
} // namespace gits
