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
void ResourceDumpLayer::Post(${interface.name}${function.name}Command& c) {
  m_ResourceDumpService.CommandListCall(c.Key, c.m_Object.Value);
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
