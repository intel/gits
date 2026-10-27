// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#pragma once

#include "layerAuto.h"
#include "stateTrackingService.h"
#include "subcaptureRange.h"
#include "subcaptureRecorder.h"

namespace gits::opengl {

// Records in Pre, before the runner overwrites outputs such as returned uniform locations with
// replay values, so the subcapture holds every command as the stream had it. Before the range,
// gives `stateTracking` the commands compiled into display lists.
class RecordingLayer : public Layer {
public:
  RecordingLayer(SubcaptureRecorder& recorder,
                 const SubcaptureRange& range,
                 StateTrackingService* stateTracking)
      : Layer("Recording"), m_Recorder(recorder), m_Range(range), m_StateTracking(stateTracking) {}

% for cmd in commands:
% if cmd.enabled:
  void Pre(${cmd.name}Command& command) override;
% endif
% endfor
  void Pre(MappedDataMetaCommand& command) override;
  void Pre(WindowGeometryMetaCommand& command) override;
  void Pre(ClientArraysMetaCommand& command) override;
  void Pre(UniformArrayMetaCommand& command) override;
  void Pre(MappedTextureDataMetaCommand& command) override;
  void Pre(PixelFormatMetaCommand& command) override;
  void Pre(UnsupportedCommand& command) override;

% for name in frame_end_commands:
  void Post(${name}Command& command) override;
% endfor

private:
  // Runs after SubcaptureLayer has advanced the frame.
  void EndFrame(uint32_t threadId);
  bool CompilingList(uint32_t threadId) const {
    return m_StateTracking != nullptr && m_StateTracking->CompilingList(threadId);
  }

  SubcaptureRecorder& m_Recorder;
  const SubcaptureRange& m_Range;
  StateTrackingService* m_StateTracking;
  bool m_RecordedFrameEndingSwap{false};
};

} // namespace gits::opengl
