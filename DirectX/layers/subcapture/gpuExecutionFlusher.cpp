// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuExecutionFlusher.h"
#include "log.h"

namespace gits {
namespace DirectX {

void GpuExecutionFlusher::CreateCommandQueue(unsigned commandQueueKey,
                                             ID3D12CommandQueue* commandQueue) {
  CommandQueueInfo info{};
  info.CommandQueue = commandQueue;
  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = commandQueue->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  hr = device->CreateFence(info.FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&info.Fence));
  GITS_ASSERT(hr == S_OK);
  m_CommandQueues[commandQueueKey] = info;
}

void GpuExecutionFlusher::DestroyCommandQueue(unsigned commandQueueKey) {
  m_CommandQueues.erase(commandQueueKey);
}

void GpuExecutionFlusher::FlushCommandQueues() {
  if (m_CommandQueues.empty()) {
    return;
  }
  for (auto& it : m_CommandQueues) {
    if (!IsCommandQueueWaiting(it.first)) {
      HRESULT hr = it.second.CommandQueue->Signal(it.second.Fence.Get(), ++it.second.FenceValue);
      GITS_ASSERT(hr == S_OK);
      hr = it.second.Fence->SetEventOnCompletion(it.second.FenceValue, NULL);
      GITS_ASSERT(hr == S_OK);
    }
  }
}

} // namespace DirectX
} // namespace gits
