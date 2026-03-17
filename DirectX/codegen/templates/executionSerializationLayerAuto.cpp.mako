// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "executionSerializationLayerAuto.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
#include "intelExtensions.h"

namespace gits {
namespace DirectX {

<%
custom = [
    'IDXGISwapChainPresent',
    'IDXGISwapChain1Present1',
    'IDXGIAdapter3QueryVideoMemoryInfo',
    'ID3D12CommandQueueExecuteCommandLists',
    'ID3D12CommandQueueWait',
    'ID3D12CommandQueueSignal',
    'ID3D12FenceSignal',
    'ID3D12DeviceCreateFence',
    'ID3D12Device3EnqueueMakeResident',
    'ID3D12FenceGetCompletedValue',
    'ID3D12FenceSetEventOnCompletion',
    'ID3D12DeviceCreateCommandQueue',
    'ID3D12Device9CreateCommandQueue1',
    'ID3D12DeviceCreateCommandList',
    'ID3D12Device4CreateCommandList1',
    'ID3D12GraphicsCommandListReset',
    'ID3D12GraphicsCommandListClose',
    'ID3D12GraphicsCommandListClearDepthStencilView',
    'ID3D12GraphicsCommandListClearRenderTargetView',
    'ID3D12GraphicsCommandListClearUnorderedAccessViewFloat',
    'ID3D12GraphicsCommandListClearUnorderedAccessViewUint',
    'ID3D12GraphicsCommandListOMSetRenderTargets',
    'xessD3D12Execute'
]
%>\
%for interface in interfaces:
%for function in interface.functions:
%if not interface.name + function.name in custom and interface.name.startswith('ID3D12GraphicsCommandList'):
void ExecutionSerializationLayer::pre(${interface.name}${function.name}Command& c) {
  if (inRange()) {
    executionService_.commandListCommand(c.object_.key, new ${interface.name}${function.name}Serializer(c));
  }
}

%endif
%endfor
%endfor

%for function in functions:
%if not function.name in custom:
void ExecutionSerializationLayer::pre(${function.name}Command& c) {
  if (inRange()) {
    recorder_.record(new ${function.name}Serializer(c));
  }
}

%endif
%endfor
%for interface in interfaces:
%for function in interface.functions:
%if not interface.name + function.name in custom and not interface.name.startswith('ID3D12GraphicsCommandList'):
void ExecutionSerializationLayer::pre(${interface.name}${function.name}Command& c) {
  if (inRange()) {
    recorder_.record(new ${interface.name}${function.name}Serializer(c));
  }
}

%endif
%endfor
%endfor

} // namespace DirectX
} // namespace gits
