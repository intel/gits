// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "recordingLayerAuto.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"

namespace gits {
namespace vulkan {

// Destructor defined here so this TU is the key function owner and the vtable
// is emitted only once (avoids MSVC LNK1163 COMDAT conflicts).
RecordingLayer::~RecordingLayer() = default;

// ---------------------------------------------------------------------------
// CreateWindowMetaCommand
// Window creation meta-commands are relevant both during state restore and
// when a new window/surface is created inside the capture range.
// ---------------------------------------------------------------------------

void RecordingLayer::Post(CreateWindowMetaCommand& command) {
  if (m_Range.InRange()) {
    m_Recorder.Record(CreateWindowMetaSerializer(command));
  }
}

// ---------------------------------------------------------------------------
// MappedDataMetaCommand
// Memory content snapshots may arrive inside the capture range when mapped
// memory is updated and flushed during the recorded frames.
// ---------------------------------------------------------------------------

void RecordingLayer::Post(MappedDataMetaCommand& command) {
  if (m_Range.InRange()) {
    m_Recorder.Record(MappedDataMetaSerializer(command));
  }
}

// ---------------------------------------------------------------------------
// vkQueuePresentKHR
// Special handling: emit a FrameEnd marker after the present so that the
// subcapture player can detect frame boundaries.  Also drives the range state
// machine and closes the recorder once the range ends.
// ---------------------------------------------------------------------------

void RecordingLayer::Post(vkQueuePresentKHRCommand& command) {
  // SubcaptureLayer is registered before RecordingLayer in the post-layer
  // order, so by the time this function runs SubcaptureLayer has already:
  //   1. Triggered state restore (if this is the restore-point present), and
  //   2. Called FrameEnd() on the shared SubcaptureRange.
  // Therefore m_Range.InRange() now reflects the NEXT frame's state, not the
  // frame that just ended.  m_WasInRange holds whether the frame that just
  // ended was inside the capture range -- that is the correct guard for
  // recording the present itself.
  if (m_WasInRange) {
    m_Recorder.Record(vkQueuePresentKHRSerializer(command));

    FrameEndCommand frameEnd;
    m_Recorder.Record(FrameEndSerializer(frameEnd));
  }

  // InRange() already reflects the post-FrameEnd (next-frame) state.
  bool isNowInRange = m_Range.InRange();

  if (m_WasInRange && !isNowInRange) {
    m_Recorder.FinishRecording();
  }

  m_WasInRange = isNowInRange;
}

} // namespace vulkan
} // namespace gits
