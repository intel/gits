// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"

#include <unordered_map>
#include <memory>

namespace gits {
namespace DirectX {

class DescriptorHeapTracker {
public:
  struct Descriptor {
    unsigned HeapKey{};
    unsigned DescriptorIndex{};
    unsigned ResourceKey{};
    unsigned UavCounterResourceKey{};
    enum class DescriptorType {
      Unknown,
      RTV,
      DSV,
      SRV,
      UAV,
      CBV,
      Sampler
    };
    DescriptorType Type{};
    union {
      D3D12_RENDER_TARGET_VIEW_DESC RenderTargetViewDesc;
      D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc;
      D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc;
      D3D12_UNORDERED_ACCESS_VIEW_DESC UnorderedAccessViewDesc;
      D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantBufferViewDesc;
      D3D12_SAMPLER_DESC SamplerDesc;
    };
    bool IsDesc{};
  };

  void CreateDescriptor(Descriptor* descriptor);
  void DestroyObject(unsigned key);
  void CopyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c);
  void CopyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c);
  Descriptor* GetDescriptor(unsigned heapKey, unsigned descriptorIndex) {
    return m_DescriptorByHeapByIndex[heapKey][descriptorIndex].get();
  }

private:
  Descriptor* CopyDescriptor(Descriptor* descriptor,
                             unsigned destHeapKey,
                             unsigned destDescriptorIndex);

private:
  std::unordered_map<unsigned, std::unordered_map<unsigned, std::unique_ptr<Descriptor>>>
      m_DescriptorByHeapByIndex;
};

} // namespace DirectX
} // namespace gits
