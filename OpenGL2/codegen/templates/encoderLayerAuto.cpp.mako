// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

#include "encoderLayerAuto.h"

#include "captureManager.h"
#include "commandSerializersAuto.h"
#include "unsupportedCommandTypes.h"

namespace gits::opengl2 {

% for cmd in commands:
% if cmd.enabled:
void EncoderLayer::Post(${cmd.name}Command& command) {
  CaptureManager::Get().RecordCommand(command, new ${cmd.name}CommandSerializer(command));
% if cmd.name in frame_end_commands:
<%
    ret = cmd.return_value.type.strip()
    is_void = ret.lower() == 'void'  # WGL commands use Win32 'VOID', GL commands use 'void'.
%>\
  // See codegen/handle_lifecycle.py's FRAME_END_COMMANDS and
  // common/layer_interface/unsupportedCommandTypes.h's FrameEndCommand - only emit the marker
  // on a successful swap (this used to be gated on the wrapper's own local `result`, before the
  // call this replaces moved here from recorderWrappersAuto.cpp.mako's Wrapper_${cmd.name} - see
  // command.m_Return.Value, set from that same `result` before this Post hook ever ran), so an
  // unsuccessful swap does not advance the shared player infrastructure's frame count. Reuses
  // command.m_ThreadId (identical to the recording thread's GetCurrentThreadId() at the point
  // the wrapper constructed `command`) instead of calling GetCurrentThreadId() again here.
% if is_void:
  FrameEndCommand frameEndCommand(command.m_ThreadId);
  frameEndCommand.m_Key = CaptureManager::Get().CreateCommandKey();
  CaptureManager::Get().RecordCommand(frameEndCommand, new FrameEndSerializer(frameEndCommand));
% else:
  if (command.m_Return.Value) {
    FrameEndCommand frameEndCommand(command.m_ThreadId);
    frameEndCommand.m_Key = CaptureManager::Get().CreateCommandKey();
    CaptureManager::Get().RecordCommand(frameEndCommand, new FrameEndSerializer(frameEndCommand));
  }
% endif
% endif
}

% endif
% endfor
} // namespace gits::opengl2
