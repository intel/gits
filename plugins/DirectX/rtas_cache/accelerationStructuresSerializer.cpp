// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresSerializer.h"
#include "gits.h"
#include "pluginUtils.h"

#include <fstream>
#include <iomanip>

namespace gits {
namespace DirectX {

AccelerationStructuresSerializer::AccelerationStructuresSerializer(CGits& gits, bool enabled)
    : ResourceDump(), gits_(gits), enabled_(enabled) {
  if (enabled_) {
    cachePath_ = L"rtas_cache";
    if (!cachePath_.empty() && !std::filesystem::exists(cachePath_)) {
      std::filesystem::create_directory(cachePath_);
    }
  }
}

AccelerationStructuresSerializer::~AccelerationStructuresSerializer() {
  if (!enabled_) {
    return;
  }
  try {
    waitUntilDumped();

    logE(gits_, "RtasCache: writing rtas_cache.dat");

    std::map<unsigned, unsigned> blases;
    for (std::filesystem::directory_entry file : std::filesystem::directory_iterator(cachePath_)) {
      std::string name = file.path().filename().string();
      unsigned size = file.file_size();
      unsigned buildKey = std::stoi(name);
      blases[buildKey] = size;
    }

    std::ofstream cache("rtas_cache.dat", std::ios_base::binary);
    for (auto& it : blases) {
      unsigned buildKey = it.first;
      std::string name = std::to_string(buildKey);
      unsigned size = it.second;
      std::ifstream file("rtas_cache/" + name, std::ios_base::binary);
      std::vector<char> data(size);
      file.read(data.data(), size);
      if (file.fail()) {
        logE(gits_, "RtasCache: error reading BLAS ", buildKey);
        break;
      }
      file.close();
      cache.write(reinterpret_cast<char*>(&buildKey), sizeof(buildKey));
      cache.write(reinterpret_cast<char*>(&size), sizeof(size));
      cache.write(data.data(), size);
      if (cache.bad()) {
        logE(gits_, "RtasCache: error writing BLAS ", buildKey);
        break;
      }
    }

    cache.flush();

    std::filesystem::remove_all(cachePath_);
  } catch (...) {
    std::cerr << "Unhandled exception caught in "
                 "AccelerationStructuresSerializer::~AccelerationStructuresSerializer";
  }
}

void AccelerationStructuresSerializer::serializeAccelerationStructure(
    unsigned buildKey,
    ID3D12GraphicsCommandList4* commandList,
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc) {

  Microsoft::WRL::ComPtr<ID3D12Device5> device;
  HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
  assert(hr == S_OK);

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
  device->GetRaytracingAccelerationStructurePrebuildInfo(&desc.Inputs, &prebuildInfo);

  UINT64 size = prebuildInfo.ResultDataMaxSizeInBytes * 2;

  AccelerationStructuresDumpInfo* dumpInfo = new AccelerationStructuresDumpInfo();
  dumpInfo->dumpName = cachePath_ + L"/" + std::to_wstring(buildKey);
  dumpInfo->size = size;
  dumpInfo->postbuildInfo.size =
      sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION_DESC);

  {
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
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
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&dumpInfo->serializedBuffer));
    assert(hr == S_OK);

    heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&dumpInfo->stagingBuffer));
    assert(hr == S_OK);
  }

  commandList->CopyRaytracingAccelerationStructure(
      dumpInfo->serializedBuffer->GetGPUVirtualAddress(), desc.DestAccelerationStructureData,
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE);

  {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.pResource = dumpInfo->serializedBuffer.Get();
    commandList->ResourceBarrier(1, &barrier);
  }

  commandList->CopyResource(dumpInfo->stagingBuffer.Get(), dumpInfo->serializedBuffer.Get());

  {
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width =
        sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION_DESC);
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc = {1, 0};
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&dumpInfo->postbuildInfoBuffer));
    assert(hr == S_OK);

    heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                         IID_PPV_ARGS(&dumpInfo->postbuildInfo.stagingBuffer));
    assert(hr == S_OK);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postbuildInfoDesc{};
    postbuildInfoDesc.DestBuffer = dumpInfo->postbuildInfoBuffer->GetGPUVirtualAddress();
    postbuildInfoDesc.InfoType =
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION;
    commandList->EmitRaytracingAccelerationStructurePostbuildInfo(
        &postbuildInfoDesc, 1, &desc.DestAccelerationStructureData);

    D3D12_RESOURCE_BARRIER barrierDesc{};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Transition.pResource = dumpInfo->postbuildInfoBuffer.Get();
    barrierDesc.Transition.Subresource = 0;
    barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    commandList->ResourceBarrier(1, &barrierDesc);

    commandList->CopyResource(dumpInfo->postbuildInfo.stagingBuffer.Get(),
                              dumpInfo->postbuildInfoBuffer.Get());

    barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    commandList->ResourceBarrier(1, &barrierDesc);
  }

  stagedResources_[commandList].push_back(dumpInfo);
}

void AccelerationStructuresSerializer::dumpStagedResource(DumpInfo& dumpInfo) {
  AccelerationStructuresDumpInfo& info = static_cast<AccelerationStructuresDumpInfo&>(dumpInfo);
  UINT64 size = 0;
  {
    void* data{};
    HRESULT hr = info.postbuildInfo.stagingBuffer->Map(0, nullptr, &data);
    assert(hr == S_OK);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION_DESC*
        infoSerializationDesc =
            static_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION_DESC*>(
                data);
    size = infoSerializationDesc->SerializedSizeInBytes;

    info.postbuildInfo.stagingBuffer->Unmap(0, nullptr);
  }

  assert(size <= info.size);

  {
    void* data{};
    HRESULT hr = info.stagingBuffer->Map(0, nullptr, &data);

    std::ofstream stream(info.dumpName, std::ios_base::binary);
    stream.write(static_cast<char*>(data), size);

    info.stagingBuffer->Unmap(0, nullptr);
  }
}

} // namespace DirectX
} // namespace gits
