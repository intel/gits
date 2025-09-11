// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtasDeserializer.h"
#include "pluginUtils.h"
#include "to_string/toStr.h"

#include <filesystem>
#include <cassert>
#include <fstream>

namespace gits {
namespace DirectX {

static std::string IdentifierToStr(
    const D3D12_SERIALIZED_DATA_DRIVER_MATCHING_IDENTIFIER& identifier) {
  const auto* pBlob = reinterpret_cast<const uint8_t*>(&identifier);
  static_assert(sizeof(D3D12_SERIALIZED_DATA_DRIVER_MATCHING_IDENTIFIER) == 32);

  auto printBytes = [](std::ostringstream& oss, const uint8_t* bytes, size_t count) {
    for (size_t i = 0; i < count; ++i) {
      oss << "0x" << std::hex << std::setw(2) << std::setfill('0')
          << static_cast<unsigned>(bytes[i]);
      if (i < (count - 1)) {
        oss << ", ";
      }
    }
  };

  std::ostringstream oss;
  oss << "GUID = {";
  printBytes(oss, pBlob, 16);
  oss << "}, VersioningData = {";
  printBytes(oss, pBlob + 16, 16);
  oss << "}";
  return oss.str();
}

RtasDeserializer::RtasDeserializer(CGits& gits, const std::string& cacheFile)
    : gits_(gits), cacheFilePath_(cacheFile) {}

RtasDeserializer::~RtasDeserializer() {
  cleanup();
}

bool RtasDeserializer::preloadCache(ID3D12Device5* device) {
  std::ifstream cacheFile(cacheFilePath_, std::ios_base::binary);
  if (!cacheFile) {
    logE(gits_, "RtasCache - Failed to open ", cacheFilePath_);
  }

  // Check if the cache file is compatible with the current device
  if (!isCompatible(cacheFile, device)) {
    logE(gits_, "RtasCache - Cache is not compatible with the current device");
    return false;
  }

  // Read the cache file
  while (cacheFile) {
    unsigned key, size = {};
    cacheFile.read(reinterpret_cast<char*>(&key), sizeof(key));
    cacheFile.read(reinterpret_cast<char*>(&size), sizeof(size));

    std::vector<uint8_t> data(size);
    cacheFile.read(reinterpret_cast<char*>(data.data()), size);
    if (size > 0 && !cacheFile) {
      logE(gits_, "RtasCache - Error reading BLAS ", key);
      cacheData_.clear();
      return false;
    }

    cacheData_[key] = std::move(data);
  }
  logI(gits_, "RtasCache - Cache file read successfully, total BLASes: ", cacheData_.size());

  return true;
}

bool RtasDeserializer::isCompatible(std::ifstream& cacheFile, ID3D12Device5* device) {
  if (!device || !cacheFile) {
    return false;
  }

  auto initialPos = cacheFile.tellg();
  // Read the key and size
  unsigned key, size = {};
  cacheFile.read(reinterpret_cast<char*>(&key), sizeof(key));
  cacheFile.read(reinterpret_cast<char*>(&size), sizeof(size));
  // Read the RTAS header
  D3D12_SERIALIZED_RAYTRACING_ACCELERATION_STRUCTURE_HEADER header;
  cacheFile.read(reinterpret_cast<char*>(&header), sizeof(header));
  // Rewind to file
  cacheFile.seekg(initialPos);

  // Check the driver identifier
  auto status = device->CheckDriverMatchingIdentifier(
      D3D12_SERIALIZED_DATA_RAYTRACING_ACCELERATION_STRUCTURE, &header.DriverMatchingIdentifier);
  logI(gits_, "RtasCache - Driver identifier check: ", toStr(status));
  if (status != D3D12_DRIVER_MATCHING_IDENTIFIER_COMPATIBLE_WITH_DEVICE) {
    logW(gits_, "RtasCache - Driver identifier mismatch for cache!");
    logW(gits_, "RtasCache - Cache identifier: ", IdentifierToStr(header.DriverMatchingIdentifier));
    return false;
  }
  return true;
}

bool RtasDeserializer::deserialize(unsigned buildKey,
                                   ID3D12GraphicsCommandList4* commandList,
                                   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc) {

  cleanup();

  auto& it = cacheData_.find(buildKey);
  if (it == cacheData_.end()) {
    logW(gits_, "RtasCache - BLAS ", buildKey, " not found in cache");
    return false;
  }

  auto key = it->first;
  auto size = static_cast<unsigned>(it->second.size());
  auto& data = it->second;

  if (size == 0 || data.empty()) {
    logW(gits_, "RtasCache - BLAS ", buildKey, " has no data in cache");
    return false;
  }

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

    void* bufferData{};
    hr = serializedBuffer->Map(0, nullptr, &bufferData);
    memcpy(bufferData, data.data(), size);
    serializedBuffer->Unmap(0, nullptr);

    commandList->CopyRaytracingAccelerationStructure(
        desc.DestAccelerationStructureData, serializedBuffer->GetGPUVirtualAddress(),
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE);

    buffersByCommandList_[commandList].emplace_back(serializedBuffer);
  }

  data.clear();

  return true;
}

void RtasDeserializer::executeCommandLists(unsigned key,
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

void RtasDeserializer::cleanup() {
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
