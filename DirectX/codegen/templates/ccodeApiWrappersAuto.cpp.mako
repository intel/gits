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
${function_signature(function)} {
    ${function_call(function)}
}

%endif
%endfor

%for interface in interfaces:
%if interface.api == Api.D3D12 or interface.api == Api.DXGI:
%for function in interface.functions:
${function_signature(function, interface.name)} {
    ${function_call(function, True)}
}

%endfor
%endif
%endfor