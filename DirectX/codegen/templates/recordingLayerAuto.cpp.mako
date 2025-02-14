// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
    'IDXGIAdapter3QueryVideoMemoryInfo'
]
%>\
%for function in functions:
void RecordingLayer::post(${function.name}Command& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new ${function.name}Writer(command));
  }
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
%if not interface.name + function.name in custom:
void RecordingLayer::post(${interface.name}${function.name}Command& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new ${interface.name}${function.name}Writer(command));
  }
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
