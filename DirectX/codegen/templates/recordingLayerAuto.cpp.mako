// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "recordingLayerAuto.h"
#include "commandWritersAuto.h"

namespace gits {
namespace DirectX {

<%
custom = [
    'IDXGISwapChainPresent',
    'IDXGISwapChain1Present1',
    'IDXGIAdapter3QueryVideoMemoryInfo',
    'ID3D12GraphicsCommandListReset',
    'ID3D12FenceGetCompletedValue',
    'xellAddMarkerData'
]
%>\
%for function in functions:
%if not function.name in custom:
void RecordingLayer::post(${function.name}Command& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new ${function.name}Writer(command));
  }
}

%endif
%endfor
%for interface in interfaces:
%for function in interface.functions:
%if not interface.name + function.name in custom:
void RecordingLayer::post(${interface.name}${function.name}Command& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new ${interface.name}${function.name}Writer(command));
  }
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
