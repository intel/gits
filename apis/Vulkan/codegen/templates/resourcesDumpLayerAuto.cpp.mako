// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "resourcesDumpLayerAuto.h"

namespace gits {
namespace vulkan {

% for command in commands:
% if is_command_buffer_command(command) and command.name not in custom_commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
void ResourcesDumpLayer::Post(${command.name}Command& command) {
  m_Service.CommandBufferCall(command.m_Key, command.m_${command.params[0].name}.Value);
}
% if define:
#endif
% endif

% endif
% endfor
} // namespace vulkan
} // namespace gits
