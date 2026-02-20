// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "directx/wrappers/ccodeApiWrappersAuto.h"

%for function in functions:
%if function.api == Api.D3D12 or function.api == Api.DXGI:
<%
params = generate_params(function)
args_simple = []
for param in function.params:
    args_simple.append(param.name)
return_statement = ""
if not function.ret.is_void:
    return_statement = "return "
%>
${generate_return(function)} CC_${function.name}(${'' if params else ') {'}
    %if params:
    %for param in params[:-1]:
    ${param},
    %endfor
    ${params[-1]}) {
    %endif
    ${return_statement}${function.name}(${'' if args_simple else ');'}
        %if args_simple:
        %for arg in args_simple[:-1]:
        ${arg},
        %endfor
        ${args_simple[-1]});
        %endif
}
%endif
%endfor

%for interface in interfaces:
%if interface.api == Api.D3D12 or interface.api == Api.DXGI:
%for function in interface.functions:
<%
params = generate_params(function)
args_simple = []
for param in function.params:
    args_simple.append(param.name)
return_statement = ""
if not function.ret.is_void:
    return_statement = "return "
%>
${generate_return(function)} CC_${function.name}(
    ${interface.name}* object${', ' if params else ') {'}
    %if params:
    %for param in params[:-1]:
    ${param},
    %endfor
    ${params[-1]}) {
    %endif
    ${return_statement}object->${function.name}(${'' if args_simple else ');'}
        %if args_simple:
        %for arg in args_simple[:-1]:
        ${arg},
        %endfor
        ${args_simple[-1]});
        %endif
}
%endfor
%endif
%endfor