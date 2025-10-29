// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "bufferPool.h"

#include <unordered_map>
#include <string>
#include <queue>
#include <fstream>

namespace gits {
class CGits;
namespace DirectX {

class RtasDeserializer {
public:
  RtasDeserializer(CGits&, const std::string& cacheFile);
  ~RtasDeserializer();

  // Disallow copying (gits::noncopyable is not available here).
  RtasDeserializer(const RtasDeserializer&) = delete;
  RtasDeserializer& operator=(const RtasDeserializer&) = delete;

  bool preloadCache(ID3D12Device5* device);

  bool deserialize(unsigned buildKey,
                   ID3D12GraphicsCommandList4* commandList,
                   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc);
  void executeCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);

private:
  bool isCompatible(std::ifstream& cacheFile, ID3D12Device5* device);
  void cleanup();

private:
  struct ExecuteInfo {
    UINT64 fenceValue{};
    std::vector<unsigned> buildKeys;
  };
  struct CommandQueueInfo {
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    UINT64 fenceValue{};
    std::queue<ExecuteInfo> executes;
  };

  CGits& gits_;
  std::string cacheFilePath_;
  std::unordered_map<unsigned, std::vector<uint8_t>> cacheData_;

  // Track command execution to release buffers after GPU is done with them
  std::unordered_map<ID3D12CommandQueue*, CommandQueueInfo> commandQueues_;
  std::unordered_map<ID3D12CommandList*, std::vector<unsigned>> buildKeysByCommandList_;

  // Buffer pool for deserialization (max 2 MB buffers) to avoid repeated allocations
  unsigned maxBufferSize_{2 * 1024 * 1024};
  BufferPool bufferPool_;
  // Temporary buffers (for buffers larger than maxBufferSize_)
  std::unordered_map<unsigned, Microsoft::WRL::ComPtr<ID3D12Resource>> tmpBuffers_;
};

} // namespace DirectX
} // namespace gits
