// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "statisticsLayerAuto.h"

namespace gits {
namespace DirectX {

%for function in functions:
void StatisticsLayer::post(${function.name}Command& command) {
  m_StatisticsService.Command("${function.name}");
}

%endfor
<%
custom = [
    'IDXGISwapChainPresent',
    'IDXGISwapChain1Present1'
]
%>\
%for interface in interfaces:
%for function in interface.functions:
%if not interface.name + function.name in custom:
void StatisticsLayer::post(${interface.name}${function.name}Command& command) {
  m_StatisticsService.Command("${interface.name}::${function.name}");
}

%endif
%endfor
%endfor

} // namespace DirectX
} // namespace gits
