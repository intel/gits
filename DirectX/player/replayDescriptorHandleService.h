// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <d3d12.h>
#include <array>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ReplayDescriptorHandleService {
public:
  enum class HandleType {
    CpuHandle,
    GpuHandle
  };

  void CreateDescriptorHeap(unsigned DescriptorHeapKey,
                            ID3D12DescriptorHeap* descriptorHeap,
                            const D3D12_DESCRIPTOR_HEAP_DESC* desc);
  size_t GetDescriptorHandle(unsigned DescriptorHeapKey, HandleType handleType, unsigned index);
  void DestroyDescriptorHeap(unsigned DescriptorHeapKey);

private:
  struct DescriptorHeapInfo {
    size_t CpuStart{};
    size_t GpuStart{};
    unsigned Increment{};
  };
  std::unordered_map<unsigned, DescriptorHeapInfo> m_DescriptorHeaps;

  std::array<unsigned, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_DescriptorHeapIncrements{};
  bool m_Initialized{false};
};

} // namespace DirectX
} // namespace gits
