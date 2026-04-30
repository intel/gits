// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureDescriptorHandleService.h"
#include "log.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void CaptureDescriptorHandleService::CreateDescriptorHeap(unsigned descriptorHeapKey,
                                                          ID3D12DescriptorHeap* descriptorHeap,
                                                          const D3D12_DESCRIPTOR_HEAP_DESC* desc) {
  tbb::spin_rw_mutex::scoped_lock lock(m_RwMutex);

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

  DescriptorHeapInfo cpuInfo;
  cpuInfo.InterfaceKey = descriptorHeapKey;
  cpuInfo.Start = descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
  cpuInfo.End = cpuInfo.Start + desc->NumDescriptors * m_DescriptorHeapIncrements[desc->Type];
  m_DescriptorHeapsByCpuStartAddress[desc->Type]
                                    [cpuInfo.Start % m_DescriptorHeapIncrements[desc->Type]]
                                    [cpuInfo.Start] = cpuInfo;

  if (desc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
    DescriptorHeapInfo gpuInfo;
    gpuInfo.InterfaceKey = descriptorHeapKey;
    gpuInfo.Start = descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
    gpuInfo.End = gpuInfo.Start + desc->NumDescriptors * m_DescriptorHeapIncrements[desc->Type];
    m_DescriptorHeapsByGpuStartAddress[desc->Type][gpuInfo.Start] = gpuInfo;
  }

  m_DescriptorHeapKeys.insert(descriptorHeapKey);
}

CaptureDescriptorHandleService::HandleInfo CaptureDescriptorHandleService::GetDescriptorHandleInfo(
    D3D12_DESCRIPTOR_HEAP_TYPE heapType, HandleType handleType, size_t handle) const {

  tbb::spin_rw_mutex::scoped_lock lock(m_RwMutex, false);

  const std::map<size_t, DescriptorHeapInfo>& descriptorHeaps =
      handleType == HandleType::CpuHandle ? m_DescriptorHeapsByCpuStartAddress[heapType].at(
                                                handle % m_DescriptorHeapIncrements[heapType])
                                          : m_DescriptorHeapsByGpuStartAddress[heapType];

  HandleInfo handleInfo{};

  auto it = descriptorHeaps.upper_bound(handle);
  if (it != descriptorHeaps.begin()) {
    --it;
    const DescriptorHeapInfo& info = it->second;
    if (handle >= info.Start && handle < info.End) {
      handleInfo.InterfaceKey = info.InterfaceKey;
      handleInfo.Index = (handle - info.Start) / m_DescriptorHeapIncrements[heapType];
    }
  }

  return handleInfo;
}

void CaptureDescriptorHandleService::DestroyDescriptorHeap(unsigned descriptorHeapKey) {

  tbb::spin_rw_mutex::scoped_lock lock(m_RwMutex);

  auto it = m_DescriptorHeapKeys.find(descriptorHeapKey);
  if (it != m_DescriptorHeapKeys.end()) {

    bool found = false;
    for (auto& moduloMap : m_DescriptorHeapsByCpuStartAddress) {
      for (auto& startMap : moduloMap) {
        for (auto& it : startMap.second) {
          if (it.second.InterfaceKey == descriptorHeapKey) {
            startMap.second.erase(it.second.Start);
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
    for (auto& startMap : m_DescriptorHeapsByGpuStartAddress) {
      for (auto& it : startMap) {
        if (it.second.InterfaceKey == descriptorHeapKey) {
          startMap.erase(it.second.Start);
          found = true;
          break;
        }
      }
      if (found) {
        break;
      }
    }

    m_DescriptorHeapKeys.erase(descriptorHeapKey);
  }
}

} // namespace DirectX
} // namespace gits
