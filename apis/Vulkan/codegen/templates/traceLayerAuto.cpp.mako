// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "traceLayerAuto.h"
#include "log.h"

#include <iomanip>
#include <sstream>

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif

void TraceLayer::Pre(${command.name}Command& command) {
  if(printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "${command.name}");
    % for i, param in enumerate(command.params):
    p.addArgument(command.m_${param.name});
    % endfor
    % if command.return_type != 'void':
    p.addResult(command.m_Return);
    % endif
    p.print(flush_);
  }
}

void TraceLayer::Post(${command.name}Command& command) {
  if(printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "${command.name}");
    % for i, param in enumerate(command.params):
    p.addArgument(command.m_${param.name});
    % endfor
    % if command.return_type != 'void':
    p.addResult(command.m_Return);
    % endif
    p.print(flush_);
  }
}

% if define:
#endif
% endif

% endfor

} // namespace vulkan
} // namespace gits
