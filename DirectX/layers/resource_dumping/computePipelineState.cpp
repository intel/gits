// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "computePipelineState.h"
#include "pipelineStateStreamDescDump.h"
#include "to_string/toStr.h"

#include <fstream>

namespace gits {
namespace DirectX {

void ComputePipelineState::Reset() {
  m_StateDesc = nullptr;
  m_StateStreamDesc = nullptr;
  m_RootSignatureKey = 0;
  m_BindingState.Reset();
}

void ComputePipelineState::SetRootSignature(unsigned rootSignatureKey,
                                            D3D12_ROOT_SIGNATURE_DESC2* desc) {
  m_RootSignatureKey = rootSignatureKey;
  m_BindingState.SetRootSignature(desc);
}

void ComputePipelineState::SetComputeRoot32BitConstant(
    ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) {
  m_BindingState.SetConstant(c.m_RootParameterIndex.Value, c.m_SrcData.Value,
                             c.m_DestOffsetIn32BitValues.Value);
}

void ComputePipelineState::SetComputeRoot32BitConstants(
    ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c) {
  m_BindingState.SetConstants(c.m_RootParameterIndex.Value,
                              static_cast<unsigned*>(c.m_pSrcData.Value),
                              c.m_DestOffsetIn32BitValues.Value, c.m_Num32BitValuesToSet.Value);
}

void ComputePipelineState::SetComputeRootConstantBufferView(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  m_BindingState.SetConstantBufferView(c.m_RootParameterIndex.Value,
                                       c.m_BufferLocation.InterfaceKey, c.m_BufferLocation.Offset);
}

void ComputePipelineState::SetComputeRootUnorderedAccessView(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  m_BindingState.SetUnorderedAccessView(c.m_RootParameterIndex.Value,
                                        c.m_BufferLocation.InterfaceKey, c.m_BufferLocation.Offset);
}

void ComputePipelineState::SetComputeRootShaderResourceView(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  m_BindingState.SetShaderResourceView(c.m_RootParameterIndex.Value,
                                       c.m_BufferLocation.InterfaceKey, c.m_BufferLocation.Offset);
}

void ComputePipelineState::SetComputeRootDescriptorTable(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  m_BindingState.SetDescriptorTable(c.m_RootParameterIndex.Value, c.m_BaseDescriptor.InterfaceKey,
                                    c.m_BaseDescriptor.Index);
}

void ComputePipelineState::DumpState(const std::wstring& dumpDir,
                                     ID3D12GraphicsCommandListDispatchCommand& c) {
  std::wstring dumpName = dumpDir + L"/dispatch_" + keyToWStr(c.Key) + L".txt";
  std::ofstream stream(dumpName);
  stream << c.Key << " Dispatch(" << c.m_ThreadGroupCountX.Value << ", "
         << c.m_ThreadGroupCountY.Value << ", " << c.m_ThreadGroupCountZ.Value << ")\n";
  DumpState(stream);
}

void ComputePipelineState::DumpState(std::ofstream& stream) {
  stream << "\n";
  if (m_StateDesc) {
    DumpStateDesc(stream);
  } else if (m_StateStreamDesc) {
    DumpPipelineStateStreamDesc(*m_StateStreamDesc, stream);
  }
  stream << "\n";
  stream << "Root signature O" << m_RootSignatureKey << "\n";
  stream << "\n";
  m_BindingState.DumpState(stream);
}

void ComputePipelineState::DumpStateDesc(std::ofstream& stream) {
  if (!m_StateDesc || !m_StateDesc->Value) {
    return;
  }
  const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc = *m_StateDesc->Value;

  stream << "D3D12_COMPUTE_PIPELINE_STATE_DESC\n";
  stream << "\tRootSignature O" << m_StateDesc->RootSignatureKey << "\n";

  stream << "\tCS BytecodeLength " << desc.CS.BytecodeLength << "\n";

  stream << "\tNodeMask 0x" << std::hex << desc.NodeMask << std::dec << "\n";
  stream << "\tCachedPSO.CachedBlobSizeInBytes " << desc.CachedPSO.CachedBlobSizeInBytes << "\n";

  stream << "\tFlags " << toStr(desc.Flags) << "\n";
}

} // namespace DirectX
} // namespace gits
