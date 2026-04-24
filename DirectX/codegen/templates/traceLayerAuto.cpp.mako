// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
    s = 'p.addArgument(command.m_' + param.name + ');'
    params.append(s)
is_result = False if function.ret.is_void else True
%>
void TraceLayer::Pre(${function.name}Command& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "${function.name}");
    %for param in params:
    ${param}
    %endfor
    %if is_result:
    p.addResult(command.m_Result);
    %endif
    p.print(m_Flush);
  }
}

void TraceLayer::Post(${function.name}Command& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "${function.name}");
    %for param in params:
    ${param}
    %endfor
    %if is_result:
    p.addResult(command.m_Result);
    %endif
    p.print(m_Flush);
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
    s = 'p.addArgument(command.m_' + param.name + ');'
    params.append(s)
is_result = False if function.ret.is_void else True
%>
void TraceLayer::Pre(${interface.name}${function.name}Command& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "${interface.name}::${function.name}", command.m_Object.Key);
    %for param in params:
    ${param}
    %endfor
    %if is_result:
    p.addResult(command.m_Result);
    %endif
    p.print(m_Flush);
  }
}

void TraceLayer::Post(${interface.name}${function.name}Command& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "${interface.name}::${function.name}", command.m_Object.Key);
    %for param in params:
    ${param}
    %endfor
    %if is_result:
    p.addResult(command.m_Result);
    %endif
    p.print(m_Flush);
  }
}

%endif
%endfor
%endfor

} // namespace DirectX
} // namespace gits
