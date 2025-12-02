// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <array>
#include <shared_mutex>

namespace gits {
namespace DirectX {

class CaptureDescriptorHandleService {
public:
  struct HandleInfo {
    unsigned interfaceKey;
    unsigned index;
  };
  enum class HandleType {
    CpuHandle,
    GpuHandle
  };

  void createDescriptorHeap(unsigned descriptorHeapKey,
                            ID3D12DescriptorHeap* descriptorHeap,
                            const D3D12_DESCRIPTOR_HEAP_DESC* desc);
  HandleInfo getDescriptorHandleInfo(D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                     HandleType handleType,
                                     size_t handle) const;
  void destroyDescriptorHeap(unsigned descriptorHeapKey);

private:
  struct DescriptorHeapInfo {
    unsigned interfaceKey;
    size_t start{};
    size_t end{};
  };

  std::array<std::unordered_map<unsigned, std::map<size_t, DescriptorHeapInfo>>,
             D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES>
      descriptorHeapsByCpuStartAddress_{};
  std::array<std::map<size_t, DescriptorHeapInfo>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES>
      descriptorHeapsByGpuStartAddress_{};

  std::array<unsigned, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> descriptorHeapIncrements_{};
  bool initialized_{false};

  std::unordered_set<unsigned> descriptorHeapKeys_;

  mutable std::shared_mutex rwMutex_;
};

} // namespace DirectX
} // namespace gits
