// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "graphicsPipelineState.h"
#include "computePipelineState.h"
#include "keyUtils.h"
#include "descriptorHeapTracker.h"
#include "descriptorRootSignatureService.h"

#include <unordered_map>

namespace gits {
namespace DirectX {

class PipelineStateDumpLayer : public Layer {
public:
  PipelineStateDumpLayer();
  ~PipelineStateDumpLayer();
  PipelineStateDumpLayer(const PipelineStateDumpLayer&) = delete;
  PipelineStateDumpLayer& operator=(const PipelineStateDumpLayer&) = delete;

  void Post(IUnknownReleaseCommand& c) override;
  void Post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) override;
  void Post(ID3D12DeviceCreateComputePipelineStateCommand& c) override;
  void Post(ID3D12Device2CreatePipelineStateCommand& c) override;
  void Post(ID3D12DeviceCreateRootSignatureCommand& c) override;
  void Post(ID3D12DeviceCreateDescriptorHeapCommand& c) override;
  void Post(ID3D12DeviceCreateRenderTargetViewCommand& c) override;
  void Post(ID3D12DeviceCreateDepthStencilViewCommand& c) override;
  void Post(ID3D12Device15TryCreateRenderTargetViewCommand& c) override;
  void Post(ID3D12Device15TryCreateDepthStencilViewCommand& c) override;
  void Post(ID3D12DeviceCreateShaderResourceViewCommand& c) override;
  void Post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) override;
  void Post(ID3D12DeviceCreateConstantBufferViewCommand& c) override;
  void Post(ID3D12Device15TryCreateShaderResourceViewCommand& c) override;
  void Post(ID3D12Device15TryCreateUnorderedAccessViewCommand& c) override;
  void Post(ID3D12Device15TryCreateConstantBufferViewCommand& c) override;
  void Post(ID3D12DeviceCreateSamplerCommand& c) override;
  void Post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) override;
  void Post(ID3D12DeviceCopyDescriptorsCommand& c) override;
  void Post(ID3D12GraphicsCommandListResetCommand& c) override;
  void Post(ID3D12GraphicsCommandListClearStateCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) override;
  void Post(ID3D12GraphicsCommandListDrawInstancedCommand& c) override;
  void Post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) override;
  void Post(ID3D12GraphicsCommandListDispatchCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetPredicationCommand& c) override;
  void Post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) override;
  void Post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) override;
  void Post(ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand& c) override;
  void Post(ID3D12GraphicsCommandListOMSetBlendFactorCommand& c) override;
  void Post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) override;
  void Post(ID3D12GraphicsCommandListOMSetStencilRefCommand& c) override;
  void Post(ID3D12GraphicsCommandListRSSetScissorRectsCommand& c) override;
  void Post(ID3D12GraphicsCommandListRSSetViewportsCommand& c) override;
  void Post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) override;

private:
  GraphicsPipelineState* GetGraphicsState(unsigned commandListKey) {
    return m_GraphicsPipelineStates
        .try_emplace(commandListKey, std::make_unique<GraphicsPipelineState>(m_DescriptorService))
        .first->second.get();
  }
  ComputePipelineState* GetComputeState(unsigned commandListKey) {
    return m_ComputePipelineStates
        .try_emplace(commandListKey, std::make_unique<ComputePipelineState>(m_DescriptorService))
        .first->second.get();
  }

private:
  DescriptorHeapTracker m_DescriptorService;
  DescriptorRootSignatureService m_RootSignatureService;
  std::unordered_map<unsigned, std::unique_ptr<GraphicsPipelineState>> m_GraphicsPipelineStates;
  std::unordered_map<unsigned, std::unique_ptr<ComputePipelineState>> m_ComputePipelineStates;
  std::wstring m_DumpDir;
  ConfigKeySet m_CommandKeys;

  std::unordered_map<unsigned, std::unique_ptr<D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument>>
      m_GraphicsPipelineStateDescs;
  std::unordered_map<unsigned, std::unique_ptr<D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument>>
      m_ComputePipelineStateDescs;
  std::unordered_map<unsigned, std::unique_ptr<D3D12_PIPELINE_STATE_STREAM_DESC_Argument>>
      m_PipelineStateDescs;
  std::unordered_map<unsigned, D3D12_DESCRIPTOR_HEAP_DESC> m_DescriptorHeaps;
  std::unordered_map<unsigned, D3D12_ROOT_SIGNATURE_DESC2*> m_RootSignatures;
};

} // namespace DirectX
} // namespace gits
