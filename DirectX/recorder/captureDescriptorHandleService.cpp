// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureDescriptorHandleService.h"
#include "gits.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void CaptureDescriptorHandleService::createDescriptorHeap(unsigned descriptorHeapKey,
                                                          ID3D12DescriptorHeap* descriptorHeap,
                                                          const D3D12_DESCRIPTOR_HEAP_DESC* desc) {
  tbb::spin_rw_mutex::scoped_lock lock(rwMutex_);

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

  DescriptorHeapInfo cpuInfo;
  cpuInfo.interfaceKey = descriptorHeapKey;
  cpuInfo.start = descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
  cpuInfo.end = cpuInfo.start + desc->NumDescriptors * descriptorHeapIncrements_[desc->Type];
  descriptorHeapsByCpuStartAddress_[desc->Type]
                                   [cpuInfo.start % descriptorHeapIncrements_[desc->Type]]
                                   [cpuInfo.start] = cpuInfo;

  if (desc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
    DescriptorHeapInfo gpuInfo;
    gpuInfo.interfaceKey = descriptorHeapKey;
    gpuInfo.start = descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
    gpuInfo.end = gpuInfo.start + desc->NumDescriptors * descriptorHeapIncrements_[desc->Type];
    descriptorHeapsByGpuStartAddress_[desc->Type][gpuInfo.start] = gpuInfo;
  }

  descriptorHeapKeys_.insert(descriptorHeapKey);
}

CaptureDescriptorHandleService::HandleInfo CaptureDescriptorHandleService::getDescriptorHandleInfo(
    D3D12_DESCRIPTOR_HEAP_TYPE heapType, HandleType handleType, size_t handle) const {

  tbb::spin_rw_mutex::scoped_lock lock(rwMutex_, false);

  const std::map<size_t, DescriptorHeapInfo>& descriptorHeaps =
      handleType == HandleType::CpuHandle ? descriptorHeapsByCpuStartAddress_[heapType].at(
                                                handle % descriptorHeapIncrements_[heapType])
                                          : descriptorHeapsByGpuStartAddress_[heapType];

  HandleInfo handleInfo{};

  auto it = descriptorHeaps.upper_bound(handle);
  if (it != descriptorHeaps.begin()) {
    --it;
    const DescriptorHeapInfo& info = it->second;
    if (handle >= info.start && handle < info.end) {
      handleInfo.interfaceKey = info.interfaceKey;
      handleInfo.index = (handle - info.start) / descriptorHeapIncrements_[heapType];
    }
  }

  return handleInfo;
}

void CaptureDescriptorHandleService::destroyDescriptorHeap(unsigned descriptorHeapKey) {

  tbb::spin_rw_mutex::scoped_lock lock(rwMutex_);

  auto it = descriptorHeapKeys_.find(descriptorHeapKey);
  if (it != descriptorHeapKeys_.end()) {

    bool found = false;
    for (auto& moduloMap : descriptorHeapsByCpuStartAddress_) {
      for (auto& startMap : moduloMap) {
        for (auto& it : startMap.second) {
          if (it.second.interfaceKey == descriptorHeapKey) {
            startMap.second.erase(it.second.start);
            found = true;
            break;
          }
        }
        if (found) {
          break;
        }
      }
      if (found) {
        break;
      }
    }

    found = false;
    for (auto& startMap : descriptorHeapsByGpuStartAddress_) {
      for (auto& it : startMap) {
        if (it.second.interfaceKey == descriptorHeapKey) {
          startMap.erase(it.second.start);
          found = true;
          break;
        }
      }
      if (found) {
        break;
      }
    }

    descriptorHeapKeys_.erase(descriptorHeapKey);
  }
}

} // namespace DirectX
} // namespace gits
