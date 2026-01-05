// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "replayDescriptorHandleService.h"
#include "gits.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void ReplayDescriptorHandleService::createDescriptorHeap(unsigned descriptorHeapKey,
                                                         ID3D12DescriptorHeap* descriptorHeap,
                                                         const D3D12_DESCRIPTOR_HEAP_DESC* desc) {

  if (!initialized_) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT res = descriptorHeap->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(res == S_OK);

    for (unsigned i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
      descriptorHeapIncrements_[i] =
          device->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
    }
    initialized_ = true;
  }

  DescriptorHeapInfo heapInfo;
  heapInfo.cpuStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
  if (desc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
    heapInfo.gpuStart = descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
  }
  heapInfo.increment = descriptorHeapIncrements_[desc->Type];

  descriptorHeaps_[descriptorHeapKey] = heapInfo;
}

size_t ReplayDescriptorHandleService::getDescriptorHandle(unsigned descriptorHeapKey,
                                                          HandleType handleType,
                                                          unsigned index) {
  if (!descriptorHeapKey) {
    return 0;
  }

  auto it = descriptorHeaps_.find(descriptorHeapKey);
  GITS_ASSERT(it != descriptorHeaps_.end());
  DescriptorHeapInfo& info = it->second;
  size_t start = handleType == HandleType::CpuHandle ? info.cpuStart : info.gpuStart;
  return start + index * info.increment;
}

void ReplayDescriptorHandleService::destroyDescriptorHeap(unsigned descriptorHeapKey) {
  descriptorHeaps_.erase(descriptorHeapKey);
}

} // namespace DirectX
} // namespace gits
