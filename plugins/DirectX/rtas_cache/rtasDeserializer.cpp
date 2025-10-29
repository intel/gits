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
    // Save current format state
    std::ios formatState(nullptr);
    formatState.copyfmt(oss);

    for (size_t i = 0; i < count; ++i) {
      oss << "0x" << std::hex << std::setw(2) << std::setfill('0')
          << static_cast<unsigned>(bytes[i]) << std::dec;
      if (i < (count - 1)) {
        oss << ", ";
      }
    }

    // Restore format state
    oss.copyfmt(formatState);
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
  logI(gits_, "RtasCache - Cleaning up RTAS deserializer...");
  logI(gits_, "RtasCache - Buffer pool contains ", bufferPool_.size(), " buffers");
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
  logI(gits_, "RtasCache - Buffer pool size: ", FormatMemorySize(maxBufferSize_));

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

  // Create buffer pool if not initialized
  if (bufferPool_.size() == 0) {
    bufferPool_.initialize(device.Get(), maxBufferSize_, 1);
  }

  ID3D12Resource* buffer = nullptr;
  if (size <= maxBufferSize_) {
    buffer = bufferPool_.acquireBuffer(buildKey);
  } else {
    logW(gits_, "RtasCache - BLAS ", key, " with size ", FormatMemorySize(size),
         " does not fit in the buffer pool");
    buffer = createBuffer(device.Get(), size);
    tmpBuffers_[buildKey] = buffer;
  }
  assert(buffer);

  void* bufferData{};
  hr = buffer->Map(0, nullptr, &bufferData);
  memcpy(bufferData, data.data(), size);
  buffer->Unmap(0, nullptr);

  commandList->CopyRaytracingAccelerationStructure(
      desc.DestAccelerationStructureData, buffer->GetGPUVirtualAddress(),
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE);

  buildKeysByCommandList_[commandList].emplace_back(buildKey);

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
    auto itCommandList = buildKeysByCommandList_.find(commandLists[i]);
    if (itCommandList != buildKeysByCommandList_.end()) {
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
    auto itCommandList = buildKeysByCommandList_.find(commandLists[i]);
    if (itCommandList != buildKeysByCommandList_.end()) {
      for (auto buildKey : itCommandList->second) {
        executeInfo.buildKeys.push_back(buildKey);
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
      for (auto buildKey : executeInfo.buildKeys) {
        bufferPool_.releaseBuffer(buildKey);
        tmpBuffers_.erase(buildKey);
      }
      it.second.executes.pop();
    }
  }
}

} // namespace DirectX
} // namespace gits
