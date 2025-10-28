// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "BufferPool.h"

#include <cassert>

namespace gits {
namespace DirectX {

void BufferPool::initialize(ID3D12Device* device, size_t bufferSize, unsigned initialCount) {
  assert(device);
  assert(bufferSize > 0);

  device_ = device;
  bufferSize_ = bufferSize;

  // Pre-allocate buffers and mark all as free
  buffers_.reserve(initialCount);
  for (unsigned i = 0; i < initialCount; ++i) {
    buffers_.push_back(createBuffer());
    freeIndices_.push(i);
  }
}

ID3D12Resource* BufferPool::acquireBuffer(unsigned key) {
  unsigned index = 0;

  if (!freeIndices_.empty()) {
    // Use an existing free buffer
    index = freeIndices_.top();
    freeIndices_.pop();
  } else {
    // Create a new buffer
    index = static_cast<unsigned>(buffers_.size());
    buffers_.push_back(createBuffer());
  }

  keyToIndex_[key] = index;
  return buffers_[index].Get();
}

void BufferPool::releaseBuffer(unsigned key) {
  auto it = keyToIndex_.find(key);
  if (it != keyToIndex_.end()) {
    unsigned index = it->second;
    keyToIndex_.erase(it);
    freeIndices_.push(index);
  }
}

size_t BufferPool::size() const {
  return buffers_.size();
}

ID3D12Resource* BufferPool::createBuffer() {
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProperties.CreationNodeMask = 1;
  heapProperties.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Width = bufferSize_; // Always use the configured buffer size
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.SampleDesc = {1, 0};
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ID3D12Resource* buffer = nullptr;
  HRESULT hr =
      device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer));
  assert(hr == S_OK);
  return buffer;
}

} // namespace DirectX
} // namespace gits
