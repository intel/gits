// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "capturePlayerDescriptorHandleService.h"
#include "log.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void CapturePlayerDescriptorHandleService::AddCaptureHandle(
    ID3D12DescriptorHeap* heap, unsigned heapKey, D3D12_GPU_DESCRIPTOR_HANDLE captureHandle) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto itView = m_ViewHeapsByKey.find(heapKey);
  if (itView != m_ViewHeapsByKey.end()) {
    return;
  }
  auto itSampler = m_SamplerHeapsByKey.find(heapKey);
  if (itSampler != m_SamplerHeapsByKey.end()) {
    return;
  }

  if (!m_ViewHeapIncrement) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT hr = heap->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(hr == S_OK);
    m_ViewHeapIncrement =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_SamplerHeapIncrement =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  }

  D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
  if (desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
    DescriptorHeapInfo* info = new DescriptorHeapInfo();
    info->CaptureStart = captureHandle.ptr;
    info->Size = desc.NumDescriptors * m_ViewHeapIncrement;
    info->Key = heapKey;
    m_ViewHeapsByCaptureHandle[captureHandle.ptr] = info;
    m_ViewHeapsByKey[heapKey].reset(info);
    m_ViewHeapsChanged = true;
  } else if (desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
    DescriptorHeapInfo* info = new DescriptorHeapInfo();
    info->CaptureStart = captureHandle.ptr;
    info->Size = desc.NumDescriptors * m_SamplerHeapIncrement;
    info->Key = heapKey;
    m_SamplerHeapsByCaptureHandle[captureHandle.ptr] = info;
    m_SamplerHeapsByKey[heapKey].reset(info);
    m_SamplerHeapsChanged = true;
  }
}

void CapturePlayerDescriptorHandleService::AddPlayerHandle(
    unsigned heapKey, D3D12_GPU_DESCRIPTOR_HANDLE playerHandle) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto it = m_ViewHeapsByKey.find(heapKey);
  if (it != m_ViewHeapsByKey.end()) {
    it->second->PlayerStart = playerHandle.ptr;
    if (m_DumpLookup) {
      m_ViewHeapsByPlayerHandle[playerHandle.ptr] = it->second.get();
    }
    return;
  }
  it = m_SamplerHeapsByKey.find(heapKey);
  if (it != m_SamplerHeapsByKey.end()) {
    it->second->PlayerStart = playerHandle.ptr;
    if (m_DumpLookup) {
      m_SamplerHeapsByPlayerHandle[playerHandle.ptr] = it->second.get();
    }
  }
}

void CapturePlayerDescriptorHandleService::DestroyHeap(unsigned heapKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto itView = m_ViewHeapsByKey.find(heapKey);
  if (itView != m_ViewHeapsByKey.end()) {
    m_ViewHeapsByCaptureHandle.erase(itView->second->CaptureStart);
    if (m_DumpLookup) {
      m_ViewHeapsByPlayerHandle.erase(itView->second->PlayerStart);
    }
    m_ViewHeapsByKey.erase(itView);
    return;
  }
  auto itSampler = m_SamplerHeapsByKey.find(heapKey);
  if (itSampler != m_SamplerHeapsByKey.end()) {
    m_SamplerHeapsByCaptureHandle.erase(itSampler->second->CaptureStart);
    if (m_DumpLookup) {
      m_SamplerHeapsByPlayerHandle.erase(itSampler->second->PlayerStart);
    }
    m_SamplerHeapsByKey.erase(itSampler);
  }
}

bool CapturePlayerDescriptorHandleService::GetViewMappings(
    std::vector<DescriptorMapping>& mappings) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  mappings.resize(m_ViewHeapsByCaptureHandle.size());
  unsigned i = 0;
  for (auto& it : m_ViewHeapsByCaptureHandle) {
    mappings[i].CaptureStart = it.second->CaptureStart;
    mappings[i].PlayerStart = it.second->PlayerStart;
    mappings[i].Size = it.second->Size;
    ++i;
  }
  bool changed = m_ViewHeapsChanged;
  m_ViewHeapsChanged = false;
  return changed;
}

bool CapturePlayerDescriptorHandleService::GetSamplerMappings(
    std::vector<DescriptorMapping>& mappings) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  mappings.resize(m_SamplerHeapsByCaptureHandle.size());
  unsigned i = 0;
  for (auto& it : m_SamplerHeapsByCaptureHandle) {
    mappings[i].CaptureStart = it.second->CaptureStart;
    mappings[i].PlayerStart = it.second->PlayerStart;
    mappings[i].Size = it.second->Size;
    ++i;
  }
  bool changed = m_SamplerHeapsChanged;
  m_SamplerHeapsChanged = false;
  return changed;
}

CapturePlayerDescriptorHandleService::DescriptorHeapInfo* CapturePlayerDescriptorHandleService::
    GetViewDescriptorHeapInfo(DescriptorHandleMap& descriptorHandleMap,
                              UINT64 handle,
                              bool fromCapture) {
  DescriptorHeapInfo* descriptorHeapInfo{};
  if (handle) {
    auto itResource = descriptorHandleMap.upper_bound(handle);
    if (itResource != descriptorHandleMap.begin() && !descriptorHandleMap.empty()) {
      --itResource;
      DescriptorHeapInfo* info = itResource->second;
      UINT64 start = fromCapture ? info->CaptureStart : info->PlayerStart;
      if (handle >= start && handle < start + info->Size) {
        descriptorHeapInfo = info;
      }
    }
  }
  return descriptorHeapInfo;
}

} // namespace DirectX
} // namespace gits
