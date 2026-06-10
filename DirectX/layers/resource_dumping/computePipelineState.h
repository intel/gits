// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "pipelineBindingState.h"
#include "descriptorHeapTracker.h"
#include "descriptorRootSignatureService.h"

#include <filesystem>
#include <fstream>

namespace gits {
namespace DirectX {

class ComputePipelineState {
public:
  ComputePipelineState(DescriptorHeapTracker& descriptorService)
      : m_DescriptorService(descriptorService), m_BindingState(descriptorService) {}
  void Reset();
  void SetStateDesc(const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument* stateDesc) {
    m_StateDesc = stateDesc;
    m_StateStreamDesc = nullptr;
  }
  void SetStateDesc(const D3D12_PIPELINE_STATE_STREAM_DESC_Argument* stateDesc) {
    m_StateStreamDesc = stateDesc;
    m_StateDesc = nullptr;
  }
  void DumpState(const std::wstring& dumpDir, ID3D12GraphicsCommandListDispatchCommand& c);
  void SetRootSignature(unsigned rootSignatureKey, D3D12_ROOT_SIGNATURE_DESC2* desc);
  void SetComputeRoot32BitConstant(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c);
  void SetComputeRoot32BitConstants(
      ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c);
  void SetComputeRootConstantBufferView(
      ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c);
  void SetComputeRootUnorderedAccessView(
      ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c);
  void SetComputeRootShaderResourceView(
      ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c);
  void SetComputeRootDescriptorTable(
      ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c);

private:
  void DumpState(std::ofstream& stream);
  void DumpStateDesc(std::ofstream& stream);

private:
  PipelineBindingState m_BindingState;
  DescriptorHeapTracker& m_DescriptorService;
  const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument* m_StateDesc{};
  const D3D12_PIPELINE_STATE_STREAM_DESC_Argument* m_StateStreamDesc{};
  unsigned m_RootSignatureKey{};
};

} // namespace DirectX
} // namespace gits
