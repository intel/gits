// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"
#include "tbb/spin_rw_mutex.h"

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <array>

namespace gits {
namespace DirectX {

class CaptureDescriptorHandleService {
public:
  struct HandleInfo {
    unsigned InterfaceKey;
    unsigned Index;
  };
  enum class HandleType {
    CpuHandle,
    GpuHandle
  };

  void CreateDescriptorHeap(unsigned descriptorHeapKey,
                            ID3D12DescriptorHeap* descriptorHeap,
                            const D3D12_DESCRIPTOR_HEAP_DESC* desc);
  HandleInfo GetDescriptorHandleInfo(D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                     HandleType handleType,
                                     size_t handle) const;
  void DestroyDescriptorHeap(unsigned descriptorHeapKey);

private:
  struct DescriptorHeapInfo {
    unsigned InterfaceKey;
    size_t Start{};
    size_t End{};
  };

  std::array<std::unordered_map<unsigned, std::map<size_t, DescriptorHeapInfo>>,
             D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES>
      m_DescriptorHeapsByCpuStartAddress{};
  std::array<std::map<size_t, DescriptorHeapInfo>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES>
      m_DescriptorHeapsByGpuStartAddress{};

  std::array<unsigned, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_DescriptorHeapIncrements{};
  bool m_Initialized{false};

  std::unordered_set<unsigned> m_DescriptorHeapKeys;

  mutable tbb::spin_rw_mutex m_RwMutex;
};

} // namespace DirectX
} // namespace gits
