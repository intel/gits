// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "encoderLayerAuto.h"
#include "commandWritersAuto.h"

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
void EncoderLayer::Post(${command.name}Command& command) {
  % if command.name == 'vkQueuePresentKHR':
  m_Recorder.frameEnd(command.m_Key);
  % endif
  m_Recorder.record(command.m_Key, new ${command.name}Writer(command));
}
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
