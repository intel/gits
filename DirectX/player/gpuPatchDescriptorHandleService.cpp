// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuPatchDescriptorHandleService.h"
#include "gits.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void GpuPatchDescriptorHandleService::addCaptureHandle(ID3D12DescriptorHeap* heap,
                                                       unsigned heapKey,
                                                       D3D12_GPU_DESCRIPTOR_HANDLE captureHandle) {

  auto itView = viewHeapsByKey_.find(heapKey);
  if (itView != viewHeapsByKey_.end()) {
    return;
  }
  auto itSampler = samplerHeapsByKey_.find(heapKey);
  if (itSampler != samplerHeapsByKey_.end()) {
    return;
  }

  if (!viewHeapIncrement_) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT hr = heap->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(hr == S_OK);
    viewHeapIncrement_ =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    samplerHeapIncrement_ =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  }

  D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
  if (desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
    DescriptorHeapInfo* info = new DescriptorHeapInfo();
    info->captureStart = captureHandle.ptr;
    info->size = desc.NumDescriptors * viewHeapIncrement_;
    info->key = heapKey;
    viewHeapsByCaptureHandle_[captureHandle.ptr] = info;
    viewHeapsByKey_[heapKey].reset(info);
    viewHeapsChanged_ = true;
  } else if (desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
    DescriptorHeapInfo* info = new DescriptorHeapInfo();
    info->captureStart = captureHandle.ptr;
    info->size = desc.NumDescriptors * samplerHeapIncrement_;
    info->key = heapKey;
    samplerHeapsByCaptureHandle_[captureHandle.ptr] = info;
    samplerHeapsByKey_[heapKey].reset(info);
    samplerHeapsChanged_ = true;
  }
}

void GpuPatchDescriptorHandleService::addPlayerHandle(unsigned heapKey,
                                                      D3D12_GPU_DESCRIPTOR_HANDLE playerHandle) {
  auto it = viewHeapsByKey_.find(heapKey);
  if (it != viewHeapsByKey_.end()) {
    it->second->playerStart = playerHandle.ptr;
    if (dumpLookup_) {
      viewHeapsByPlayerHandle_[playerHandle.ptr] = it->second.get();
    }
    return;
  }
  it = samplerHeapsByKey_.find(heapKey);
  if (it != samplerHeapsByKey_.end()) {
    it->second->playerStart = playerHandle.ptr;
    if (dumpLookup_) {
      samplerHeapsByPlayerHandle_[playerHandle.ptr] = it->second.get();
    }
  }
}

void GpuPatchDescriptorHandleService::destroyHeap(unsigned heapKey) {
  auto itView = viewHeapsByKey_.find(heapKey);
  if (itView != viewHeapsByKey_.end()) {
    viewHeapsByCaptureHandle_.erase(itView->second->captureStart);
    if (dumpLookup_) {
      viewHeapsByPlayerHandle_.erase(itView->second->playerStart);
    }
    viewHeapsByKey_.erase(itView);
    return;
  }
  auto itSampler = samplerHeapsByKey_.find(heapKey);
  if (itSampler != samplerHeapsByKey_.end()) {
    samplerHeapsByCaptureHandle_.erase(itSampler->second->captureStart);
    if (dumpLookup_) {
      samplerHeapsByPlayerHandle_.erase(itSampler->second->playerStart);
    }
    samplerHeapsByKey_.erase(itSampler);
  }
}

bool GpuPatchDescriptorHandleService::getViewMappings(std::vector<DescriptorMapping>& mappings) {
  mappings.resize(viewHeapsByCaptureHandle_.size());
  unsigned i = 0;
  for (auto& it : viewHeapsByCaptureHandle_) {
    mappings[i].captureStart = it.second->captureStart;
    mappings[i].playerStart = it.second->playerStart;
    mappings[i].size = it.second->size;
    ++i;
  }
  bool changed = viewHeapsChanged_;
  viewHeapsChanged_ = false;
  return changed;
}

bool GpuPatchDescriptorHandleService::getSamplerMappings(std::vector<DescriptorMapping>& mappings) {
  mappings.resize(samplerHeapsByCaptureHandle_.size());
  unsigned i = 0;
  for (auto& it : samplerHeapsByCaptureHandle_) {
    mappings[i].captureStart = it.second->captureStart;
    mappings[i].playerStart = it.second->playerStart;
    mappings[i].size = it.second->size;
    ++i;
  }
  bool changed = samplerHeapsChanged_;
  samplerHeapsChanged_ = false;
  return changed;
}

GpuPatchDescriptorHandleService::DescriptorHeapInfo* GpuPatchDescriptorHandleService::
    getViewDescriptorHeapInfo(DescriptorHandleMap& descriptorHandleMap,
                              UINT64 handle,
                              bool fromCapture) {
  DescriptorHeapInfo* descriptorHeapInfo{};
  if (handle) {
    auto itResource = descriptorHandleMap.upper_bound(handle);
    if (itResource != descriptorHandleMap.begin() && !descriptorHandleMap.empty()) {
      --itResource;
      DescriptorHeapInfo* info = itResource->second;
      UINT64 start = fromCapture ? info->captureStart : info->playerStart;
      if (handle >= start && handle < start + info->size) {
        descriptorHeapInfo = info;
      }
    }
  }
  return descriptorHeapInfo;
}

} // namespace DirectX
} // namespace gits
