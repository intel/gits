// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "commandCodersAuto.h"
#include "subcaptureRecorder.h"
#include "subcaptureRange.h"
#include "stateTrackingService.h"
#include "objectState.h"

#include <vector>

namespace gits {
namespace vulkan {

class RecordingLayer : public Layer {
public:
  RecordingLayer(SubcaptureRecorder& recorder,
                 SubcaptureRange& range,
                 StateTrackingService* stateTracking = nullptr)
      : Layer("Recording"), m_Recorder(recorder), m_Range(range),
        m_StateTracking(stateTracking) {}
  ~RecordingLayer();

  RecordingLayer(const RecordingLayer&) = delete;
  RecordingLayer& operator=(const RecordingLayer&) = delete;

  // Custom-handled commands (implemented in recordingLayerCustom.cpp).
  void Post(CreateWindowMetaCommand& command) override;
  void Post(MappedDataMetaCommand& command) override;
  void Post(vkQueuePresentKHRCommand& command) override;

  // Auto-generated Post overrides for every other Vulkan command.
  % for command in commands:
  <% define = get_define(command.platform) %>\
  % if define:
#ifdef ${define}
  % endif
  % if command.name not in recording_layer_custom_commands:
  void Post(${command.name}Command& command) override;
  % endif
  % if define:
#endif
  % endif
  % endfor

private:
  // Encode a vkCmd* command into the owning command buffer's recorded-commands
  // log so it can be re-emitted verbatim during state restore.  Called when
  // !m_Range.InRange() and the command buffer is still in recording state.
  template <typename TCommand>
  void TrackCmdBuffer(const TCommand& cmd) {
    if (!m_StateTracking) {
      return;
    }
    auto* state = m_StateTracking->GetState<CommandBufferState>(cmd.m_commandBuffer.Key);
    if (!state || !state->IsRecording) {
      return;
    }
    uint32_t sz = GetSize(cmd);
    std::vector<char> buf(sz);
    Encode(cmd, buf.data());
    state->RecordedCommandIds.push_back(static_cast<CommandId>(cmd.GetId()));
    state->RecordedCommands.push_back(std::move(buf));
  }

  SubcaptureRecorder& m_Recorder;
  SubcaptureRange& m_Range;
  StateTrackingService* m_StateTracking;
  // Tracks whether the frame that just ended was inside the capture range.
  // Needed because SubcaptureLayer fires before RecordingLayer in the post-
  // layer order and has already advanced m_CurrentFrame via FrameEnd() by the
  // time RecordingLayer::Post(vkQueuePresentKHR) runs.
  bool m_WasInRange{false};
};

} // namespace vulkan
} // namespace gits
