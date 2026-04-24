// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "recordingLayerAuto.h"
#include "commandSerializersAuto.h"

namespace gits {
namespace DirectX {

<%
custom = [
    'IDXGISwapChainPresent',
    'IDXGISwapChain1Present1',
    'IDXGIAdapter3QueryVideoMemoryInfo',
    'ID3D12GraphicsCommandListReset',
    'ID3D12FenceGetCompletedValue'
]
%>\
%for function in functions:
%if not function.name in custom:
void RecordingLayer::Post(${function.name}Command& command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(${function.name}Serializer(command));
  }
}

%endif
%endfor
%for interface in interfaces:
%for function in interface.functions:
%if not interface.name + function.name in custom:
void RecordingLayer::Post(${interface.name}${function.name}Command& command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(${interface.name}${function.name}Serializer(command));
  }
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
