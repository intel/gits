// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "descriptorHeapTracker.h"

#include <d3d12.h>
#include <fstream>
#include <unordered_map>
#include <array>

namespace gits {
namespace DirectX {

class PipelineBindingState {
public:
  PipelineBindingState(DescriptorHeapTracker& descriptorService)
      : m_DescriptorService(descriptorService) {}
  void Reset();
  void SetRootSignature(D3D12_ROOT_SIGNATURE_DESC2* desc);
  void SetDescriptorTable(unsigned parameterIndex,
                          unsigned descriptorHeapKey,
                          unsigned descriptorHeapIndex);
  void SetConstant(unsigned parameterIndex, unsigned data, unsigned offset);
  void SetConstants(unsigned parameterIndex, unsigned* data, unsigned offset, unsigned size);
  void SetConstantBufferView(unsigned parameterIndex,
                             unsigned resourceKey,
                             unsigned resourceOffset);
  void SetUnorderedAccessView(unsigned parameterIndex,
                              unsigned resourceKey,
                              unsigned resourceOffset);
  void SetShaderResourceView(unsigned parameterIndex,
                             unsigned resourceKey,
                             unsigned resourceOffset);
  void DumpState(std::ofstream& stream);

private:
  void DumpShaderResourceView(std::ofstream& stream, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
  void DumpUnorderedAccessView(std::ofstream& stream, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);
  void DumpSampler(std::ofstream& stream, const D3D12_SAMPLER_DESC& desc);

private:
  DescriptorHeapTracker& m_DescriptorService;
  D3D12_ROOT_SIGNATURE_DESC2* m_RootSignatureDesc{};

  struct Binding {
    D3D12_ROOT_PARAMETER_TYPE Type;
    union {
      struct {
        unsigned DescriptorHeapKey;
        unsigned DescriptorHeapIndex;
      } DescriptorTable;
      struct {
        unsigned ResourceKey;
        unsigned ResourceOffset;
      } Descriptor;
      struct {
      } Constant;
    };
  };
  std::unordered_map<unsigned, Binding> m_Bindings;
};

} // namespace DirectX
} // namespace gits
