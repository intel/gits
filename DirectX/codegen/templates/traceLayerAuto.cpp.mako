// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "traceLayerAuto.h"

namespace gits {
namespace DirectX {

<%
custom = [
    'ID3D12ResourceGetGPUVirtualAddress',
    'ID3D12DescriptorHeapGetCPUDescriptorHandleForHeapStart',
    'ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStart',
    'ID3D12GraphicsCommandListBeginEvent'
]
%>\
%for function in functions:
%if not function.name in custom:
<%
params = []
for param in function.params:
    s = 'p.addArgument(command.' + param.name + '_);'
    params.append(s)
is_result = False if function.ret.is_void else True
%>
void TraceLayer::pre(${function.name}Command& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "${function.name}");
    %for param in params:
    ${param}
    %endfor
    %if is_result:
    p.addResult(command.result_);
    %endif
    p.print(flush_);
  }
}

void TraceLayer::post(${function.name}Command& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "${function.name}");
    %for param in params:
    ${param}
    %endfor
    %if is_result:
    p.addResult(command.result_);
    %endif
    p.print(flush_);
  }
}

%endif
%endfor

%for interface in interfaces:
%for function in interface.functions:
%if not interface.name + function.name in custom:
<%
params = []
for param in function.params:
    s = 'p.addArgument(command.' + param.name + '_);'
    params.append(s)
is_result = False if function.ret.is_void else True
%>
void TraceLayer::pre(${interface.name}${function.name}Command& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "${interface.name}::${function.name}", command.object_.key);
    %for param in params:
    ${param}
    %endfor
    %if is_result:
    p.addResult(command.result_);
    %endif
    p.print(flush_);
  }
}

void TraceLayer::post(${interface.name}${function.name}Command& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "${interface.name}::${function.name}", command.object_.key);
    %for param in params:
    ${param}
    %endfor
    %if is_result:
    p.addResult(command.result_);
    %endif
    p.print(flush_);
  }
}

%endif
%endfor
%endfor

} // namespace DirectX
} // namespace gits
