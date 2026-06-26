// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "bufferPool.h"

#include <cassert>

namespace gits {
namespace DirectX {

void BufferPool::Initialize(ID3D12Device* device, size_t bufferSize, unsigned initialCount) {
  assert(device);
  assert(bufferSize > 0);

  m_Device = device;
  m_BufferSize = bufferSize;

  // Clear existing data
  m_Buffers.clear();
  m_KeyToIndex.clear();
  m_FreeIndices = std::stack<unsigned>();

  // Pre-allocate buffers and mark all as free
  for (unsigned i = 0; i < initialCount; ++i) {
    m_Buffers.push_back(CreateBuffer(m_Device, m_BufferSize));
    m_FreeIndices.push(i);
  }
}

ID3D12Resource* BufferPool::AcquireBuffer(unsigned key) {
  unsigned index = 0;
  if (!m_FreeIndices.empty()) {
    // Use an existing free buffer
    index = m_FreeIndices.top();
    m_FreeIndices.pop();
  } else {
    // Create a new buffer
    index = static_cast<unsigned>(m_Buffers.size());
    m_Buffers.push_back(CreateBuffer(m_Device, m_BufferSize));
  }

  m_KeyToIndex[key] = index;
  return m_Buffers[index].Get();
}

void BufferPool::ReleaseBuffer(unsigned key) {
  auto it = m_KeyToIndex.find(key);
  if (it != m_KeyToIndex.end()) {
    unsigned index = it->second;
    m_KeyToIndex.erase(it);
    m_FreeIndices.push(index);
  }
}

size_t BufferPool::Size() const {
  return m_Buffers.size();
}

ID3D12Resource* CreateBuffer(ID3D12Device* device, size_t bufferSize) {
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProperties.CreationNodeMask = 1;
  heapProperties.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Width = bufferSize;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.SampleDesc = {1, 0};
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ID3D12Resource* buffer = nullptr;
  auto hr =
      device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                      D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer));
  assert(hr == S_OK);
  return buffer;
}

} // namespace DirectX
} // namespace gits
