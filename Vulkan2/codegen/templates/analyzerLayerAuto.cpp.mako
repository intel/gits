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
void AnalyzerLayer::Post(${command.name}Command& command) {
% for param in command.params:
% if param.is_handle or param.is_handle_output:
% if param.length:
  m_AnalyzerService.NotifyObjects(command.m_${param.name}.Keys);
% else:
  m_AnalyzerService.NotifyObject(command.m_${param.name}.Key);
% endif
% elif param.is_struct_with_handles and param.is_pointer and not param.is_pointer_to_pointer:
  m_AnalyzerService.NotifyObjects(command.m_${param.name}.HandleKeys);
% endif
% endfor
}
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
