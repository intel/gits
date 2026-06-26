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
%if function.api in [Api.D3D12, Api.DXGI, Api.DXGI_DEBUG, Api.XESS, Api.DSTORAGE, Api.XELL, Api.XEFG]:
${function_signature(function)} {
    ${function_call(function)}
}

%endif
%endfor

%for interface in interfaces:
%if interface.api in [Api.D3D12, Api.DXGI, Api.DXGI_DEBUG, Api.XESS, Api.DSTORAGE, Api.XELL, Api.XEFG]:
%for function in interface.functions:
${function_signature(function, interface.name)} {
    ${function_call(function, True)}
}

%endfor
%endif
%endfor