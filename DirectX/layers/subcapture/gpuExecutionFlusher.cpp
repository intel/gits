// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuExecutionFlusher.h"
#include "gits.h"

namespace gits {
namespace DirectX {

void GpuExecutionFlusher::createCommandQueue(unsigned commandQueueKey,
                                             ID3D12CommandQueue* commandQueue) {
  CommandQueueInfo info{};
  info.commandQueue = commandQueue;
  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = commandQueue->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  hr = device->CreateFence(info.fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&info.fence));
  GITS_ASSERT(hr == S_OK);
  commandQueues_[commandQueueKey] = info;
}

void GpuExecutionFlusher::destroyCommandQueue(unsigned commandQueueKey) {
  commandQueues_.erase(commandQueueKey);
}

void GpuExecutionFlusher::flushCommandQueues() {
  if (commandQueues_.empty()) {
    return;
  }
  for (auto& it : commandQueues_) {
    if (!isCommandQueueWaiting(it.first)) {
      HRESULT hr = it.second.commandQueue->Signal(it.second.fence.Get(), ++it.second.fenceValue);
      GITS_ASSERT(hr == S_OK);
      hr = it.second.fence->SetEventOnCompletion(it.second.fenceValue, NULL);
      GITS_ASSERT(hr == S_OK);
    }
  }
}

} // namespace DirectX
} // namespace gits
