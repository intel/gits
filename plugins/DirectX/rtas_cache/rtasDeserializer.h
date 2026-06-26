// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
namespace DirectX {

class RtasDeserializer {
public:
  RtasDeserializer(const std::string& cacheFile);
  ~RtasDeserializer();

  // Disallow copying (gits::noncopyable is not available here).
  RtasDeserializer(const RtasDeserializer&) = delete;
  RtasDeserializer& operator=(const RtasDeserializer&) = delete;

  bool PreloadCache(ID3D12Device5* device);

  bool Deserialize(unsigned buildKey,
                   ID3D12GraphicsCommandList4* commandList,
                   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc);
  void ExecuteCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);

private:
  bool IsCompatible(std::ifstream& cacheFile, ID3D12Device5* device);
  void Cleanup();

  struct ExecuteInfo {
    UINT64 FenceValue{};
    std::vector<unsigned> BuildKeys;
  };
  struct CommandQueueInfo {
    Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
    UINT64 FenceValue{};
    std::queue<ExecuteInfo> Executes;
  };

  std::string m_CacheFilePath;
  std::unordered_map<unsigned, std::vector<uint8_t>> m_CacheData;

  // Track command execution to release buffers after GPU is done with them
  std::unordered_map<ID3D12CommandQueue*, CommandQueueInfo> m_CommandQueues;
  std::unordered_map<ID3D12CommandList*, std::vector<unsigned>> m_BuildKeysByCommandList;

  // Buffer pool for deserialization (max 2 MB buffers) to avoid repeated allocations
  unsigned m_MaxBufferSize{2 * 1024 * 1024};
  BufferPool m_BufferPool;
  // Temporary buffers (for buffers larger than m_MaxBufferSize)
  std::unordered_map<unsigned, Microsoft::WRL::ComPtr<ID3D12Resource>> m_TmpBuffers;
};

} // namespace DirectX
} // namespace gits
