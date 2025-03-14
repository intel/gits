// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <d3d12.h>
#include <wrl/client.h>
#include <unordered_map>
#include <vector>
#include <queue>
#include <fstream>

namespace gits {
class CGits;
namespace DirectX {

class AccelerationStructuresDeserializer {
public:
  AccelerationStructuresDeserializer(CGits& gits);
  ~AccelerationStructuresDeserializer();
  void deserializeAccelerationStructure(unsigned buildKey,
                                        ID3D12GraphicsCommandList4* commandList,
                                        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc);
  void executeCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);

private:
  void cleanup();

private:
  CGits& gits_;
  std::unordered_map<ID3D12CommandList*, std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>>
      buffersByCommandList_;

  struct ExecuteInfo {
    UINT64 fenceValue{};
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> buffers;
  };
  struct CommandQueueInfo {
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    UINT64 fenceValue{};
    std::queue<ExecuteInfo> executes;
  };
  std::unordered_map<ID3D12CommandQueue*, CommandQueueInfo> commandQueues_;

  std::ifstream cacheFile_;
};

} // namespace DirectX
} // namespace gits
