// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "recordingLayerAuto.h"
#include "commandSerializersAuto.h"

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
% if command.name not in recording_layer_custom_commands:
% if command.name.startswith('vkCmd'):
void RecordingLayer::Post(${command.name}Command& command) {
  if (m_Range.InRange()) {
    m_Recorder.Record(${command.name}Serializer(command));
  } else {
    TrackCmdBuffer(command);
  }
}

% else:
void RecordingLayer::Post(${command.name}Command& command) {
  if (m_Range.InRange()) {
    m_Recorder.Record(${command.name}Serializer(command));
  }
}

% endif
% endif
% if define:
#endif
% endif
% endfor
} // namespace vulkan
} // namespace gits
