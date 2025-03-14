// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresDeserializer.h"

#include <filesystem>
#include <cassert>
#include <fstream>

namespace gits {
namespace DirectX {

AccelerationStructuresDeserializer::~AccelerationStructuresDeserializer() {
  cleanup();
}

void AccelerationStructuresDeserializer::deserializeAccelerationStructure(
    ID3D12GraphicsCommandList4* commandList,
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc,
    const std::wstring& filePath) {

  cleanup();

  unsigned size = std::filesystem::file_size(filePath);

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
  assert(hr == S_OK);

  Microsoft::WRL::ComPtr<ID3D12Resource> serializedBuffer;

  {
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = size;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc = {1, 0};
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&serializedBuffer));
    assert(hr == S_OK);

    void* data{};
    hr = serializedBuffer->Map(0, nullptr, &data);

    std::ifstream stream(filePath, std::ios_base::binary);
    stream.read(static_cast<char*>(data), size);

    serializedBuffer->Unmap(0, nullptr);

    commandList->CopyRaytracingAccelerationStructure(
        desc.DestAccelerationStructureData, serializedBuffer->GetGPUVirtualAddress(),
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE);

    buffersByCommandList_[commandList].emplace_back(serializedBuffer);
  }
}

void AccelerationStructuresDeserializer::executeCommandLists(unsigned key,
                                                             unsigned commandQueueKey,
                                                             ID3D12CommandQueue* commandQueue,
                                                             ID3D12CommandList** commandLists,
                                                             unsigned commandListNum) {
  bool found = false;
  for (unsigned i = 0; i < commandListNum; ++i) {
    auto itCommandList = buffersByCommandList_.find(commandLists[i]);
    if (itCommandList != buffersByCommandList_.end()) {
      found = true;
      break;
    }
  }
  if (!found) {
    return;
  }

  cleanup();

  auto itCommandQueue = commandQueues_.find(commandQueue);
  if (itCommandQueue == commandQueues_.end()) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT hr = commandQueue->GetDevice(IID_PPV_ARGS(&device));
    assert(hr == S_OK);
    CommandQueueInfo info{};
    hr = device->CreateFence(info.fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&info.fence));
    assert(hr == S_OK);
    auto ret = commandQueues_.insert(std::make_pair(commandQueue, info));
    itCommandQueue = ret.first;
  }

  itCommandQueue->first->Signal(itCommandQueue->second.fence.Get(),
                                ++itCommandQueue->second.fenceValue);

  ExecuteInfo executeInfo{};
  executeInfo.fenceValue = itCommandQueue->second.fenceValue;

  for (unsigned i = 0; i < commandListNum; ++i) {
    auto itCommandList = buffersByCommandList_.find(commandLists[i]);
    if (itCommandList != buffersByCommandList_.end()) {
      for (auto& buffer : itCommandList->second) {
        executeInfo.buffers.emplace_back(buffer);
      }
    }
  }

  itCommandQueue->second.executes.push(executeInfo);
}

void AccelerationStructuresDeserializer::cleanup() {
  for (auto& it : commandQueues_) {
    UINT64 completedValue = it.second.fence->GetCompletedValue();
    while (!it.second.executes.empty()) {
      ExecuteInfo& executeInfo = it.second.executes.front();
      if (executeInfo.fenceValue > completedValue) {
        break;
      }
      it.second.executes.pop();
    }
  }
}

} // namespace DirectX
} // namespace gits
