// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "commandsCustom.h"
#include "pipelineBindingState.h"
#include "descriptorHeapTracker.h"
#include "descriptorRootSignatureService.h"

#include <filesystem>
#include <fstream>
#include <array>

namespace gits {
namespace DirectX {

class GraphicsPipelineState {
public:
  GraphicsPipelineState(DescriptorHeapTracker& descriptorService)
      : m_DescriptorService(descriptorService), m_BindingState(descriptorService) {}
  void Reset();
  void SetStateDesc(const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument* stateDesc) {
    m_StateDesc = stateDesc;
  }
  void DumpState(const std::wstring& dumpDir, ID3D12GraphicsCommandListDrawInstancedCommand& c);
  void DumpState(const std::wstring& dumpDir,
                 ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c);
  void SetRootSignature(unsigned rootSignatureKey, D3D12_ROOT_SIGNATURE_DESC2* desc);
  void IASetIndexBuffer(ID3D12GraphicsCommandListIASetIndexBufferCommand& c);
  void IASetVertexBuffers(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c);
  void IASetPrimitiveTopology(ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand& c);
  void OMSetBlendFactor(ID3D12GraphicsCommandListOMSetBlendFactorCommand& c);
  void OMSetRenderTargets(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c);
  void OMSetStencilRef(ID3D12GraphicsCommandListOMSetStencilRefCommand& c);
  void RSSetScissorRects(ID3D12GraphicsCommandListRSSetScissorRectsCommand& c);
  void RSSetViewports(ID3D12GraphicsCommandListRSSetViewportsCommand& c);
  void SOSetTargets(ID3D12GraphicsCommandListSOSetTargetsCommand& c);
  void SetGraphicsRoot32BitConstant(
      ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantCommand& c);
  void SetGraphicsRoot32BitConstants(
      ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsCommand& c);
  void SetGraphicsRootConstantBufferView(
      ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c);
  void SetGraphicsRootUnorderedAccessView(
      ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c);
  void SetGraphicsRootShaderResourceView(
      ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c);
  void SetGraphicsRootDescriptorTable(
      ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c);

private:
  void DumpState(std::ofstream& stream);
  void DumpStateDesc(std::ofstream& stream);

private:
  PipelineBindingState m_BindingState;
  DescriptorHeapTracker& m_DescriptorService;
  const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument* m_StateDesc{};
  unsigned m_RootSignatureKey{};
  unsigned m_IndexBufferKey{};
  unsigned m_IndexBufferOffset{};

  struct VertexBuffer {
    unsigned Key{};
    unsigned Offset{};
    unsigned Size{};
    unsigned Stride{};
  };
  std::array<VertexBuffer, 16> m_VertexBuffers{};
  std::array<std::optional<DescriptorHeapTracker::Descriptor>, 8> m_RenderTargets{};
  std::optional<DescriptorHeapTracker::Descriptor> m_DepthStencil{};
};

} // namespace DirectX
} // namespace gits
