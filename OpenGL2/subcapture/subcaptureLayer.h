// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "analyzerResults.h"
#include "layerAuto.h"
#include "stateTrackingService.h"
#include "subcaptureRange.h"
#include "subcaptureRecorder.h"

namespace gits::opengl2 {

// Owns the frame range and the subcapture stream, and advances the frame after a swap the
// recorder ended a frame at. A range starting after frame 1 begins with state restore.
class SubcaptureLayer : public Layer {
public:
  explicit SubcaptureLayer(const SubcaptureRange& range);

  SubcaptureRecorder& GetRecorder() {
    return m_Recorder;
  }
  const SubcaptureRange& GetRange() const {
    return m_Range;
  }
  StateTrackingService& GetStateTracking() {
    return m_StateTracking;
  }
  const AnalyzerResults& GetAnalysis() const {
    return m_Analysis;
  }

  void Pre(wglSwapBuffersCommand& command) override;
  void Post(wglSwapBuffersCommand& command) override;
  void Pre(wglSwapLayerBuffersCommand& command) override;
  void Post(wglSwapLayerBuffersCommand& command) override;
  void Pre(glXSwapBuffersCommand& command) override;
  void Post(glXSwapBuffersCommand& command) override;

private:
  void EndFrame();

  SubcaptureRange m_Range;
  SubcaptureRecorder m_Recorder;
  StateTrackingService m_StateTracking;
  AnalyzerResults m_Analysis;
  bool m_SwapEndsFrame{false};
};

} // namespace gits::opengl2
