// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "analyzerLayerAuto.h"

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
% if command.name not in analyzer_layer_custom_commands:
void AnalyzerLayer::Post(${command.name}Command& command) {
% for param in command.params:
% if param.is_handle or param.is_handle_output:
% if param.length:
  m_AnalyzerService.AddObjectsForRestore(command.m_${param.name}.Keys);
% else:
  m_AnalyzerService.AddObjectForRestore(command.m_${param.name}.Key);
% endif
% elif param.is_struct_with_handles and param.is_pointer and not param.is_pointer_to_pointer:
  m_AnalyzerService.AddObjectsForRestore(command.m_${param.name}.HandleKeys);
% endif
% endfor
}
% endif
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
