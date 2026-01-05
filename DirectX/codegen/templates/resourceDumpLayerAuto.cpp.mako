// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "resourceDumpLayerAuto.h"

namespace gits {
namespace DirectX {

%for interface in interfaces:
%for function in interface.functions:
%if interface.name.startswith('ID3D12GraphicsCommandList') and not function.name.startswith('ResourceBarrier'):
void ResourceDumpLayer::post(${interface.name}${function.name}Command& c) {
  resourceDumpService_.commandListCall(c.key, c.object_.value);
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
