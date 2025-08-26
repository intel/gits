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
<%
custom = [
    'ID3D12GraphicsCommandListReset',
    'ID3D12GraphicsCommandListSetComputeRootSignature',
    'ID3D12GraphicsCommandListSetGraphicsRootSignature',
    'ID3D12GraphicsCommandListSetComputeRootDescriptorTable',
    'ID3D12GraphicsCommandListSetGraphicsRootDescriptorTable',
    'ID3D12GraphicsCommandListSetComputeRootConstantBufferView',
    'ID3D12GraphicsCommandListSetGraphicsRootConstantBufferView',
    'ID3D12GraphicsCommandListSetComputeRootShaderResourceView',
    'ID3D12GraphicsCommandListSetGraphicsRootShaderResourceView',
    'ID3D12GraphicsCommandListSetComputeRootUnorderedAccessView',
    'ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessView',
    'ID3D12GraphicsCommandListIASetIndexBuffer',
    'ID3D12GraphicsCommandListIASetVertexBuffers',
    'ID3D12GraphicsCommandListSOSetTargets',
    'ID3D12GraphicsCommandListOMSetRenderTargets',
    'ID3D12GraphicsCommandListClearDepthStencilView',
    'ID3D12GraphicsCommandListClearRenderTargetView',
    'ID3D12GraphicsCommandListClearUnorderedAccessViewUint',
    'ID3D12GraphicsCommandListClearUnorderedAccessViewFloat',
    'ID3D12GraphicsCommandList4SetPipelineState1',
    'ID3D12GraphicsCommandList2WriteBufferImmediate',
    'ID3D12GraphicsCommandListCopyBufferRegion',
    'ID3D12GraphicsCommandListCopyResource',
    'ID3D12GraphicsCommandListCopyTextureRegion',
    'ID3D12GraphicsCommandListCopyTiles',
    'ID3D12GraphicsCommandListDiscardResource',
    'ID3D12GraphicsCommandListResolveSubresource',
    'ID3D12GraphicsCommandListResourceBarrier',
    'ID3D12GraphicsCommandListSetPipelineState',
    'ID3D12GraphicsCommandList1ResolveSubresourceRegion',
    'ID3D12GraphicsCommandList3SetProtectedResourceSession',
    'ID3D12GraphicsCommandList4InitializeMetaCommand',
]
%>\
%for interface in interfaces:
%for function in interface.functions:
%if interface.name.startswith('ID3D12GraphicsCommandList') and not function.name.startswith('SetName') \
	and not interface.name + function.name in custom:
void AnalyzerLayer::post(${interface.name}${function.name}Command& c) {
  analyzerService_.commandListCommand(c.object_.key);
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
