// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "directx/directx.h"

%for function in functions:
%if function.api == Api.D3D12 or function.api == Api.DXGI:
<%
params = generate_params(function)
%>
${generate_return(function)} CC_${function.name}(${'' if params else ');'}
    %if params:
    %for param in params[:-1]:
    ${param},
    %endfor
    ${params[-1]});
    %endif
%endif
%endfor

%for interface in interfaces:
%if interface.api == Api.D3D12 or interface.api == Api.DXGI:
%for function in interface.functions:
<%
params = generate_params(function)
%>
${generate_return(function)} CC_${function.name}(
    ${interface.name}* object${', ' if params else ');'}
    %if params:
    %for param in params[:-1]:
    ${param},
    %endfor
    ${params[-1]});
    %endif
%endfor
%endif
%endfor