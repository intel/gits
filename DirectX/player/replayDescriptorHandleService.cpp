// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "replayDescriptorHandleService.h"
#include "log.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void ReplayDescriptorHandleService::CreateDescriptorHeap(unsigned DescriptorHeapKey,
                                                         ID3D12DescriptorHeap* descriptorHeap,
                                                         const D3D12_DESCRIPTOR_HEAP_DESC* desc) {

  if (!m_Initialized) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT res = descriptorHeap->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(res == S_OK);

    for (unsigned i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
      m_DescriptorHeapIncrements[i] =
          device->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
    }
    m_Initialized = true;
  }

  DescriptorHeapInfo heapInfo;
  heapInfo.CpuStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
  if (desc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
    heapInfo.GpuStart = descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
  }
  heapInfo.Increment = m_DescriptorHeapIncrements[desc->Type];

  m_DescriptorHeaps[DescriptorHeapKey] = heapInfo;
}

size_t ReplayDescriptorHandleService::GetDescriptorHandle(unsigned DescriptorHeapKey,
                                                          HandleType handleType,
                                                          unsigned index) {
  if (!DescriptorHeapKey) {
    return 0;
  }

  auto it = m_DescriptorHeaps.find(DescriptorHeapKey);
  GITS_ASSERT(it != m_DescriptorHeaps.end());
  DescriptorHeapInfo& info = it->second;
  size_t start = handleType == HandleType::CpuHandle ? info.CpuStart : info.GpuStart;
  return start + index * info.Increment;
}

void ReplayDescriptorHandleService::DestroyDescriptorHeap(unsigned DescriptorHeapKey) {
  m_DescriptorHeaps.erase(DescriptorHeapKey);
}

} // namespace DirectX
} // namespace gits
