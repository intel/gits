// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtasSerializer.h"
#include "log.h"

#include <fstream>
#include <iomanip>

namespace gits {
namespace DirectX {

RtasSerializer::RtasSerializer(const std::string& cacheFile, bool dumpCacheInfo)
    : ResourceDump(),
      m_CacheFile(cacheFile),
      m_DumpCacheInfo(dumpCacheInfo),
      m_Initialized(false) {}

RtasSerializer::~RtasSerializer() {
  WriteCache();
}

void RtasSerializer::Serialize(unsigned buildKey,
                               ID3D12GraphicsCommandList4* commandList,
                               D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& desc) {
  Initialize();

  m_BuildKeys.push_back(buildKey);
  m_CacheInfoByBuildKey[buildKey].DestVA = desc.DestAccelerationStructureData;

  Microsoft::WRL::ComPtr<ID3D12Device5> device;
  HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
  assert(hr == S_OK);

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
  device->GetRaytracingAccelerationStructurePrebuildInfo(&desc.Inputs, &prebuildInfo);

  UINT64 size = prebuildInfo.ResultDataMaxSizeInBytes * 2;

  RtasDumpInfo* dumpInfo = new RtasDumpInfo();
  dumpInfo->DumpName = m_TmpCacheDir + L"/" + std::to_wstring(buildKey);
  dumpInfo->Size = size;
  dumpInfo->PostbuildInfo.Size =
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
                                         IID_PPV_ARGS(&dumpInfo->SerializedBuffer));
    assert(hr == S_OK);

    heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&dumpInfo->StagingBuffer));
    assert(hr == S_OK);
  }

  commandList->CopyRaytracingAccelerationStructure(
      dumpInfo->SerializedBuffer->GetGPUVirtualAddress(), desc.DestAccelerationStructureData,
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE);

  {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.pResource = dumpInfo->SerializedBuffer.Get();
    commandList->ResourceBarrier(1, &barrier);
  }

  commandList->CopyResource(dumpInfo->StagingBuffer.Get(), dumpInfo->SerializedBuffer.Get());

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
                                         IID_PPV_ARGS(&dumpInfo->PostbuildInfoBuffer));
    assert(hr == S_OK);

    heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                         IID_PPV_ARGS(&dumpInfo->PostbuildInfo.StagingBuffer));
    assert(hr == S_OK);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postbuildInfoDesc{};
    postbuildInfoDesc.DestBuffer = dumpInfo->PostbuildInfoBuffer->GetGPUVirtualAddress();
    postbuildInfoDesc.InfoType =
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION;
    commandList->EmitRaytracingAccelerationStructurePostbuildInfo(
        &postbuildInfoDesc, 1, &desc.DestAccelerationStructureData);

    D3D12_RESOURCE_BARRIER barrierDesc{};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Transition.pResource = dumpInfo->PostbuildInfoBuffer.Get();
    barrierDesc.Transition.Subresource = 0;
    barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    commandList->ResourceBarrier(1, &barrierDesc);

    commandList->CopyResource(dumpInfo->PostbuildInfo.StagingBuffer.Get(),
                              dumpInfo->PostbuildInfoBuffer.Get());

    barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    commandList->ResourceBarrier(1, &barrierDesc);
  }

  m_StagedResources[commandList].push_back(dumpInfo);
}

void RtasSerializer::WriteCache() {
  static bool written = false;
  if (!m_Initialized || written) {
    return;
  }

  try {
    WaitUntilDumped();

    LOG_INFO << "RtasCache - Writing " << m_CacheFile;

    std::unordered_map<unsigned, unsigned> blases;
    for (std::filesystem::directory_entry file :
         std::filesystem::directory_iterator(m_TmpCacheDir)) {
      std::string name = file.path().filename().string();
      unsigned size = file.file_size();
      unsigned buildKey = std::stoi(name);
      blases[buildKey] = size;
    }

    // Serialize the RTASes based on build key serialization order
    std::ofstream cache(m_CacheFile, std::ios_base::binary);
    for (unsigned buildKey : m_BuildKeys) {
      std::wstring name = std::to_wstring(buildKey);
      unsigned size = blases[buildKey];
      std::ifstream file(m_TmpCacheDir + L"/" + name, std::ios_base::binary);
      std::vector<char> data(size);
      file.read(data.data(), size);
      if (file.fail()) {
        LOG_ERROR << "RtasCache - Error reading BLAS " << buildKey;
        break;
      }
      file.close();
      cache.write(reinterpret_cast<char*>(&buildKey), sizeof(buildKey));
      cache.write(reinterpret_cast<char*>(&size), sizeof(size));
      cache.write(data.data(), size);
      if (cache.bad()) {
        LOG_ERROR << "RtasCache - Error writing BLAS " << buildKey;
        break;
      }
    }
    cache.flush();

    LOG_INFO << "RtasCache - Writing done";

    std::filesystem::remove_all(m_TmpCacheDir);

    if (m_DumpCacheInfo) {
      DumpCacheInfo();
    }

    written = true;
  } catch (...) {
    std::cerr << "Unhandled exception caught in RtasSerializer::WriteCache()";
  }
}

void RtasSerializer::DumpStagedResource(DumpInfo& dumpInfo) {
  RtasDumpInfo& info = static_cast<RtasDumpInfo&>(dumpInfo);
  UINT64 size = 0;
  {
    void* data{};
    HRESULT hr = info.PostbuildInfo.StagingBuffer->Map(0, nullptr, &data);
    assert(hr == S_OK);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION_DESC*
        infoSerializationDesc =
            static_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION_DESC*>(
                data);
    size = infoSerializationDesc->SerializedSizeInBytes;

    info.PostbuildInfo.StagingBuffer->Unmap(0, nullptr);
  }

  assert(size <= info.Size);

  {
    void* data{};
    HRESULT hr = info.StagingBuffer->Map(0, nullptr, &data);

    std::ofstream stream(info.DumpName, std::ios_base::binary);
    stream.write(static_cast<char*>(data), size);

    info.StagingBuffer->Unmap(0, nullptr);
  }
}

void RtasSerializer::Initialize() {
  if (m_Initialized) {
    return;
  }

  // Create the temporary RTAS cache directory
  m_TmpCacheDir = L"rtas_cache";
  if (!m_TmpCacheDir.empty() && !std::filesystem::exists(m_TmpCacheDir)) {
    std::filesystem::create_directory(m_TmpCacheDir);
  }
  m_Initialized = true;
}

void RtasSerializer::DumpCacheInfo() {
  std::filesystem::path cacheInfoFile =
      m_CacheFile.parent_path() / (m_CacheFile.stem().string() + "_info.csv");
  std::ofstream cacheInfo(cacheInfoFile);
  cacheInfo << "BuildKey,DestVA\n";
  for (const auto& [buildKey, info] : m_CacheInfoByBuildKey) {
    cacheInfo << buildKey << "," << info.DestVA << '\n';
  }
}

} // namespace DirectX
} // namespace gits
