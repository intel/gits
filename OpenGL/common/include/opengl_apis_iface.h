// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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
#include "config.h"

namespace gits {
namespace OpenGL {
class OpenGLApi : public ApisIface::Api3d {
public:
  OpenGLApi() : Api3d(ApisIface::OpenGL) {}
  virtual ~OpenGLApi() {}
  virtual bool CfgRec_IsAllMode() const {
    return Config::Get().recorder.openGL.capture.mode.find("All") != std::string::npos;
  }
  virtual unsigned CfgRec_ExitFrame() const {
    return Config::Get().recorder.openGL.capture.all.exitFrame;
  }
  virtual bool CfgRec_IsFramesMode() const {
    return Config::Get().recorder.openGL.capture.mode.find("Frames") != std::string::npos;
  }
  virtual int CfgRec_StartFrame() const {
    return Config::Get().recorder.openGL.capture.frames.startFrame;
  }
  virtual int CfgRec_StopFrame() const {
    return Config::Get().recorder.openGL.capture.frames.stopFrame;
  }
  virtual const std::vector<unsigned>& CfgRec_StartKeys() const {
    return Config::Get().recorder.openGL.capture.frames.startKeys;
  }
  virtual bool CfgRec_IsSingleDrawMode() const {
    return Config::Get().recorder.openGL.capture.mode.find("OglSingleDraw") != std::string::npos;
  }
  virtual int CfgRec_SingleDraw() const {
    return Config::Get().recorder.openGL.capture.oglSingleDraw.number;
  }
  virtual bool CfgRec_IsDrawsRangeMode() const {
    return Config::Get().recorder.openGL.capture.mode.find("OglDrawsRange") != std::string::npos;
  }
  virtual int CfgRec_StartDraw() const {
    return Config::Get().recorder.openGL.capture.oglDrawsRange.startDraw;
  }
  virtual int CfgRec_StopDraw() const {
    return Config::Get().recorder.openGL.capture.oglDrawsRange.stopDraw;
  }
  virtual int CfgRec_Frame() const {
    return Config::Get().recorder.openGL.capture.oglDrawsRange.frame;
  }
  virtual bool CfgRec_IsBenchmark() const {
    return Config::Get().recorder.openGL.performance.benchmark;
  }
  virtual unsigned CfgRec_EndFrameSleep() const {
    return Config::Get().recorder.openGL.utilities.endFrameSleep;
  }
  virtual void Play_SwapAfterPrepare() const;
};
} // namespace OpenGL
} // namespace gits
