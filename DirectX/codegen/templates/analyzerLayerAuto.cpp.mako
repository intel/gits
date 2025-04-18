// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "analyzerLayerAuto.h"

namespace gits {
namespace DirectX {

%for interface in interfaces:
%for function in interface.functions:
%if interface.name.startswith('ID3D12GraphicsCommandList') and not function.name.startswith('Reset') and not function.name.startswith('SetName'):
void AnalyzerLayer::post(${interface.name}${function.name}Command& c) {
  analyzerService_.commandListCommand(c.object_.key);
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
