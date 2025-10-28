// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <vector>
#include <unordered_map>
#include <stack>
#include <wrl/client.h>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class BufferPool {
public:
  BufferPool() = default;
  ~BufferPool() = default;

  void initialize(ID3D12Device* device, size_t bufferSize, unsigned initialCount = 0);
  ID3D12Resource* acquireBuffer(unsigned key);
  void releaseBuffer(unsigned key);
  size_t size() const;

private:
  ID3D12Resource* createBuffer();

  ID3D12Device* device_{nullptr};
  size_t bufferSize_{0};

  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> buffers_;
  std::unordered_map<unsigned, unsigned> keyToIndex_;
  std::stack<unsigned> freeIndices_;
};

} // namespace DirectX
} // namespace gits
