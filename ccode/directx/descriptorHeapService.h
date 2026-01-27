// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"
#include <unordered_map>
#include <array>

namespace directx {

class DescriptorHeapService {
public:
  enum class HandleType {
    CpuHandle,
    GpuHandle
  };

  static DescriptorHeapService& Get();

  void Create(unsigned key,
              ID3D12DescriptorHeap* descriptorHeap,
              const D3D12_DESCRIPTOR_HEAP_DESC* desc);
  size_t GetHandle(unsigned key, HandleType handleType, unsigned index);
  void Destroy(unsigned key);

private:
  DescriptorHeapService() = default;
  ~DescriptorHeapService() = default;

  // Prevent copying and assignment
  DescriptorHeapService(const DescriptorHeapService&) = delete;
  DescriptorHeapService& operator=(const DescriptorHeapService&) = delete;

  struct DescriptorHeapInfo {
    size_t CPUStart{};
    size_t GPUStart{};
    unsigned Increment{};
  };
  std::unordered_map<unsigned, DescriptorHeapInfo> m_DescriptorHeaps;

  std::array<unsigned, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_DescriptorHeapIncrements{};
  bool m_Initialized{false};
};

} // namespace directx
