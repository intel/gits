// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#include "recordingLayerAuto.h"
#include "commandSerializersAuto.h"

namespace gits::opengl {

% for cmd in commands:
% if cmd.enabled:
void RecordingLayer::Pre(${cmd.name}Command& command) {
  if (m_Range.InRange()) {
    m_Recorder.Record(${cmd.name}CommandSerializer(command));
% if cmd.name in frame_end_commands:
% if has_return_member(cmd):
    m_RecordedFrameEndingSwap = command.m_Return.Value != 0;
% else:
    // A swap without a result always ends a frame.
    m_RecordedFrameEndingSwap = true;
% endif
% endif
% if cmd.name in list_commands:
  } else if (CompilingList(command.m_ThreadId)) {
    m_StateTracking->AddListCommand(command.m_ThreadId,
                                    Store<${cmd.name}CommandSerializer>(command));
% endif
  }
}

% endif
% endfor
% for name in frame_end_commands:
void RecordingLayer::Post(${name}Command& command) {
  EndFrame(command.m_ThreadId);
}

% endfor
} // namespace gits::opengl
