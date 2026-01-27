// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "descriptorHeapService.h"
#include <cassert>

namespace directx {

DescriptorHeapService& DescriptorHeapService::Get() {
  static DescriptorHeapService s_Instance;
  return s_Instance;
}

void DescriptorHeapService::Create(unsigned key,
                                   ID3D12DescriptorHeap* descriptorHeap,
                                   const D3D12_DESCRIPTOR_HEAP_DESC* desc) {
  if (!m_Initialized) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT res = descriptorHeap->GetDevice(IID_PPV_ARGS(&device));
    assert(res == S_OK);

    for (unsigned i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
      m_DescriptorHeapIncrements[i] =
          device->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
    }
    m_Initialized = true;
  }

  DescriptorHeapInfo heapInfo;
  heapInfo.CPUStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
  if (desc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
    heapInfo.GPUStart = descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
  }
  heapInfo.Increment = m_DescriptorHeapIncrements[desc->Type];

  m_DescriptorHeaps[key] = heapInfo;
}

size_t DescriptorHeapService::GetHandle(unsigned key, HandleType handleType, unsigned index) {
  if (!key) {
    return 0;
  }

  auto it = m_DescriptorHeaps.find(key);
  assert(it != m_DescriptorHeaps.end());
  DescriptorHeapInfo& info = it->second;
  size_t start = handleType == HandleType::CpuHandle ? info.CPUStart : info.GPUStart;
  return start + index * info.Increment;
}

void DescriptorHeapService::Destroy(unsigned key) {
  m_DescriptorHeaps.erase(key);
}

} // namespace directx
