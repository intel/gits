// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gpuExecutionTracker.h"

#include <wrl/client.h>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class GpuExecutionFlusher : public GpuExecutionTracker {
public:
  void createCommandQueue(unsigned commandQueueKey, ID3D12CommandQueue* commandQueue);
  void destroyCommandQueue(unsigned commandQueueKey);
  void flushCommandQueues();

private:
  struct CommandQueueInfo {
    ID3D12CommandQueue* commandQueue{};
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    UINT64 fenceValue{};
  };
  std::unordered_map<unsigned, CommandQueueInfo> commandQueues_;
};

} // namespace DirectX
} // namespace gits
