// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "encoderLayerAuto.h"
#include "commandSerializersAuto.h"

namespace gits {
namespace DirectX {

<%
custom = [
    'IDXGISwapChainPresent',
    'IDXGISwapChain1Present1'
]
%>\
%for function in functions:
%if not function.name in custom:
void EncoderLayer::post(${function.name}Command& command) {
  recorder_.Record(command.key, new ${function.name}Serializer(command));
}

%endif
%endfor
%for interface in interfaces:
%for function in interface.functions:
%if not interface.name + function.name in custom:
void EncoderLayer::post(${interface.name}${function.name}Command& command) {
  recorder_.Record(command.key, new ${interface.name}${function.name}Serializer(command));
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
