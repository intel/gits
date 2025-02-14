// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"

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

  void createDescriptorHeap(unsigned descriptorHeapKey,
                            ID3D12DescriptorHeap* descriptorHeap,
                            const D3D12_DESCRIPTOR_HEAP_DESC* desc);
  size_t getDescriptorHandle(unsigned descriptorHeapKey, HandleType handleType, unsigned index);
  void destroyDescriptorHeap(unsigned descriptorHeapKey);

private:
  struct DescriptorHeapInfo {
    size_t cpuStart{};
    size_t gpuStart{};
    unsigned increment{};
  };
  std::unordered_map<unsigned, DescriptorHeapInfo> descriptorHeaps_;

  std::array<unsigned, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> descriptorHeapIncrements_{};
  bool initialized_{false};
};

} // namespace DirectX
} // namespace gits
