// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "gpuExecutionTracker.h"

#include <wrl/client.h>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class GpuExecutionFlusher : public GpuExecutionTracker {
public:
  void ExecuteCommandLists(GITSKey commandQueueKey, ID3D12CommandQueue* commandQueue);
  void DestroyCommandQueue(GITSKey commandQueueKey);
  void FlushCommandQueues();

private:
  struct CommandQueueInfo {
    ID3D12CommandQueue* CommandQueue{};
    Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
    UINT64 FenceValue{};
  };
  std::unordered_map<GITSKey, CommandQueueInfo> m_CommandQueues;
};

} // namespace DirectX
} // namespace gits
