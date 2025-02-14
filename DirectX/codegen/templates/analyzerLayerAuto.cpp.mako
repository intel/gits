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
  if (inRange_) {
    unsigned commandListKey = c.object_.key;
    auto it = commandListsReset_.find(commandListKey);
    if (it == commandListsReset_.end()) {
      commandListsForRestore_.insert(commandListKey);
    }
  }
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
