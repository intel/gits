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
#include "subcaptureSession.h"

namespace gits::opengl {

// Owns the frame range and the subcapture stream, and advances the frame after a swap the
// recorder ended a frame at. A range starting after frame 1 begins with state restore. In a
// stream recorded with several APIs, the frames end at the frame ends of any of them, and the
// state of all of them is restored together. See SubcaptureSession.
class SubcaptureLayer : public Layer, public stream::SubcaptureSession::Participant {
public:
  explicit SubcaptureLayer(const SubcaptureRange& range);
  ~SubcaptureLayer() override;

  // Throws for a memory object or semaphore that imports an export the other API does not
  // restore, as the subcapture could not import it, and for a buffer or texture with storage in
  // memory whose contents the other API cannot restore.
  void PrepareRestoreState() override;
  void RestoreState() override;
  void EndFrame() override;

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
  void SwapEnded();

  SubcaptureRange m_Range;
  SubcaptureRecorder m_Recorder;
  StateTrackingService m_StateTracking;
  AnalyzerResults m_Analysis;
  bool m_SwapEndsFrame{false};
};

} // namespace gits::opengl
