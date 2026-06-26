// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

// Used internally by BufferPool but can also be used externally (for larger buffers)
ID3D12Resource* CreateBuffer(ID3D12Device* device, size_t bufferSize);

class BufferPool {
public:
  BufferPool() = default;
  ~BufferPool() = default;

  void Initialize(ID3D12Device* device, size_t bufferSize, unsigned initialCount = 0);
  ID3D12Resource* AcquireBuffer(unsigned key);
  void ReleaseBuffer(unsigned key);
  size_t Size() const;

private:
  ID3D12Device* m_Device{nullptr};
  size_t m_BufferSize{0};

  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_Buffers;
  std::unordered_map<unsigned, unsigned> m_KeyToIndex;
  std::stack<unsigned> m_FreeIndices;
};

} // namespace DirectX
} // namespace gits
