// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "patchService.h"

#include "dataService.h"
#include <plog/Log.h>

#include <wrl/client.h>
#include <algorithm>
#include <cassert>
#include <fstream>

namespace directx {

PatchService::PatchService(std::filesystem::path dataDir, CpuPatchServices services)
    : m_DataDir(std::move(dataDir)), m_CpuPatchDir(m_DataDir / "cpu_patch"), m_Services(services) {
  if (std::filesystem::exists(m_CpuPatchDir)) {
    LOG_INFO << "CCode PatchService - data location: " << m_CpuPatchDir.string();
  }

  size_t instancesMaxSize{};
  if (std::filesystem::exists(m_CpuPatchDir / "instances_dump")) {
    for (const auto& entry :
         std::filesystem::directory_iterator(m_CpuPatchDir / "instances_dump")) {
      if (std::filesystem::is_regular_file(entry.path())) {
        if (entry.file_size() > instancesMaxSize) {
          instancesMaxSize = entry.file_size();
        }
      }
    }
  }

  std::unordered_map<unsigned, size_t> bindingTablesSizes;
  if (std::filesystem::exists(m_CpuPatchDir / "binding_table_dump")) {
    for (const auto& entry :
         std::filesystem::directory_iterator(m_CpuPatchDir / "binding_table_dump")) {
      if (std::filesystem::is_regular_file(entry.path())) {
        size_t pos = entry.path().filename().string().find("-");
        unsigned key = std::stoul(entry.path().filename().string().substr(0, pos));
        bindingTablesSizes[key] += entry.file_size();
      }
    }
  }
  if (std::filesystem::exists(m_CpuPatchDir / "execute_indirect_dump" / "binding_tables")) {
    for (const auto& entry : std::filesystem::directory_iterator(
             m_CpuPatchDir / "execute_indirect_dump" / "binding_tables")) {
      if (std::filesystem::is_regular_file(entry.path())) {
        size_t pos = entry.path().filename().string().find("-");
        unsigned key = std::stoul(entry.path().filename().string().substr(0, pos));
        bindingTablesSizes[key] += entry.file_size();
      }
    }
  }
  auto bindingTablesMaxSizeIt =
      std::max_element(bindingTablesSizes.begin(), bindingTablesSizes.end(),
                       [](const auto& a, const auto& b) { return a.second < b.second; });
  size_t bindingTablesMaxSize{};
  if (bindingTablesMaxSizeIt != bindingTablesSizes.end()) {
    bindingTablesMaxSize = bindingTablesMaxSizeIt->second;
  }

  size_t argumentBufferMaxSize{};
  if (std::filesystem::exists(m_CpuPatchDir / "execute_indirect_dump")) {
    for (const auto& entry :
         std::filesystem::directory_iterator(m_CpuPatchDir / "execute_indirect_dump")) {
      if (std::filesystem::is_regular_file(entry.path())) {
        if (entry.file_size() > argumentBufferMaxSize) {
          argumentBufferMaxSize = entry.file_size();
        }
      }
    }
  }

  m_PatchBufferSize = static_cast<unsigned>(
      (std::max)({instancesMaxSize, bindingTablesMaxSize, argumentBufferMaxSize}));
  constexpr unsigned maxNumOffsetsPerPatchBuffer = 4;
  m_PatchBufferSize = static_cast<unsigned>(
      Align(m_PatchBufferSize +
                maxNumOffsetsPerPatchBuffer * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
            D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
  LOG_INFO << "CCode PatchService - patch buffer size: " << m_PatchBufferSize;

  LoadExecuteIndirectDispatchRays();
}

PatchService& PatchService::Get() {
  static std::unique_ptr<PatchService> s_Instance;
  if (!s_Instance) {
    const auto dataDir = DataService::Get().GetPath().parent_path();
    CpuPatchServices services{CapturePlayerGpuAddressService::Get(),
                              CapturePlayerShaderIdentifierService::Get(),
                              CapturePlayerDescriptorHandleService::Get()};
    s_Instance = std::make_unique<PatchService>(dataDir, services);
  }
  return *s_Instance;
}

void PatchService::RegisterCommandSignature(unsigned commandSignatureKey,
                                            const D3D12_COMMAND_SIGNATURE_DESC& desc) {
  OwnedCommandSignatureDesc owned;
  owned.desc = desc;
  if (desc.NumArgumentDescs && desc.pArgumentDescs) {
    owned.argumentDescs.assign(desc.pArgumentDescs, desc.pArgumentDescs + desc.NumArgumentDescs);
    owned.desc.pArgumentDescs = owned.argumentDescs.data();
  } else {
    owned.desc.pArgumentDescs = nullptr;
  }
  m_CommandSignatures[commandSignatureKey] = std::move(owned);
}

void PatchService::PreBuildRaytracingAccelerationStructure(
    unsigned commandListKey,
    unsigned commandKey,
    ID3D12GraphicsCommandList* commandList,
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc) {
  if (!desc || desc->Inputs.Type != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL ||
      desc->Inputs.NumDescs == 0) {
    return;
  }

  if (!m_Initialized) {
    Initialize(commandList);
  }

  const unsigned patchBufferIndex = GetPatchBufferIndex(commandListKey, commandList);
  desc->Inputs.InstanceDescs = m_PatchBuffers[patchBufferIndex]->GetGPUVirtualAddress();

  PendingPatch patchInfo;
  patchInfo.patchBufferIndex = patchBufferIndex;
  patchInfo.type = PendingPatch::Type::Build;
  patchInfo.commandKey = commandKey;
  patchInfo.commandListKey = commandListKey;
  patchInfo.numInstanceDescs = desc->Inputs.NumDescs;
  m_PendingPatchesByCommandList[commandListKey].push_back(patchInfo);
}

void PatchService::PreDispatchRays(unsigned commandListKey,
                                   unsigned commandKey,
                                   ID3D12GraphicsCommandList* commandList,
                                   D3D12_DISPATCH_RAYS_DESC* desc) {
  if (!desc || !commandList) {
    return;
  }

  if (!m_Initialized) {
    Initialize(commandList);
  }

  const unsigned patchBufferIndex = GetPatchBufferIndex(commandListKey, commandList);
  const D3D12_GPU_VIRTUAL_ADDRESS patchBufferAddress =
      m_PatchBuffers[patchBufferIndex]->GetGPUVirtualAddress();

  unsigned patchBufferOffset = 0;
  auto replaceBindingTable = [&](D3D12_GPU_VIRTUAL_ADDRESS& startAddress, UINT64 sizeInBytes,
                                 UINT64 strideInBytes) {
    if (!startAddress || !sizeInBytes) {
      return;
    }
    if (strideInBytes == 0) {
      strideInBytes = sizeInBytes;
    }
    const unsigned stride = static_cast<unsigned>(strideInBytes);
    const unsigned count = static_cast<unsigned>(sizeInBytes / strideInBytes);
    startAddress = patchBufferAddress + patchBufferOffset;
    patchBufferOffset += stride * count;
    patchBufferOffset = static_cast<unsigned>(
        Align(patchBufferOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
  };

  replaceBindingTable(desc->RayGenerationShaderRecord.StartAddress,
                      desc->RayGenerationShaderRecord.SizeInBytes,
                      desc->RayGenerationShaderRecord.SizeInBytes);
  replaceBindingTable(desc->MissShaderTable.StartAddress, desc->MissShaderTable.SizeInBytes,
                      desc->MissShaderTable.StrideInBytes);
  replaceBindingTable(desc->HitGroupTable.StartAddress, desc->HitGroupTable.SizeInBytes,
                      desc->HitGroupTable.StrideInBytes);
  replaceBindingTable(desc->CallableShaderTable.StartAddress, desc->CallableShaderTable.SizeInBytes,
                      desc->CallableShaderTable.StrideInBytes);

  PendingPatch patchInfo;
  patchInfo.patchBufferIndex = patchBufferIndex;
  patchInfo.type = PendingPatch::Type::DispatchRays;
  patchInfo.commandKey = commandKey;
  patchInfo.commandListKey = commandListKey;
  patchInfo.dispatchRaysDesc = *desc;
  m_PendingPatchesByCommandList[commandListKey].push_back(patchInfo);
}

void PatchService::PreExecuteIndirect(unsigned commandListKey,
                                      unsigned commandKey,
                                      ID3D12GraphicsCommandList* commandList,
                                      unsigned commandSignatureKey,
                                      ID3D12Resource** argumentBuffer,
                                      UINT64* argumentBufferOffset,
                                      UINT maxCommandCount) {
  if (!argumentBuffer || !argumentBufferOffset || !commandList) {
    return;
  }

  m_ExecuteIndirectLastArgumentBufferOffset = *argumentBufferOffset;

  const auto commandSignaturesIt = m_CommandSignatures.find(commandSignatureKey);
  assert(commandSignaturesIt != m_CommandSignatures.end());
  const D3D12_COMMAND_SIGNATURE_DESC& commandSignature = commandSignaturesIt->second.desc;

  bool view = false;
  bool raytracing = false;
  for (unsigned i = 0; i < commandSignature.NumArgumentDescs; ++i) {
    const D3D12_INDIRECT_ARGUMENT_TYPE type = commandSignature.pArgumentDescs[i].Type;
    if (type == D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW) {
      view = true;
    } else if (type == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS) {
      raytracing = true;
    }
  }

  if (!view && !raytracing) {
    return;
  }

  if (!m_Initialized) {
    Initialize(commandList);
  }

  const unsigned patchBufferIndex = GetPatchBufferIndex(commandListKey, commandList);
  *argumentBuffer = m_PatchBuffers[patchBufferIndex];
  *argumentBufferOffset = 0;

  PendingPatch patchInfo;
  patchInfo.patchBufferIndex = patchBufferIndex;
  patchInfo.type = PendingPatch::Type::ExecuteIndirect;
  patchInfo.commandKey = commandKey;
  patchInfo.commandListKey = commandListKey;
  patchInfo.commandSignatureKey = commandSignatureKey;
  patchInfo.argumentBufferOffset = m_ExecuteIndirectLastArgumentBufferOffset;
  patchInfo.maxCommandCount = maxCommandCount;
  patchInfo.commandList = commandList;
  m_PendingPatchesByCommandList[commandListKey].push_back(patchInfo);
}

void PatchService::PostExecuteIndirect(UINT64* argumentBufferOffset, UINT64 restoreOffset) {
  if (argumentBufferOffset) {
    *argumentBufferOffset = restoreOffset;
  }
}

void PatchService::PreExecuteCommandLists(const unsigned* commandListKeys,
                                          size_t commandListCount) {
  std::vector<PendingPatch*> patchInfos;
  for (size_t i = 0; i < commandListCount; ++i) {
    const auto it = m_PendingPatchesByCommandList.find(commandListKeys[i]);
    if (it == m_PendingPatchesByCommandList.end()) {
      continue;
    }
    for (auto& pending : it->second) {
      patchInfos.push_back(&pending);
    }
  }

  if (patchInfos.empty()) {
    return;
  }

  std::vector<CapturePlayerGpuAddressService::GpuAddressMapping> gpuAddressMappings;
  m_Services.GpuAddress.GetMappings(gpuAddressMappings);

  for (PendingPatch* patchInfo : patchInfos) {
    if (patchInfo->type == PendingPatch::Type::Build) {
      PatchBuild(*patchInfo, gpuAddressMappings);
    } else if (patchInfo->type == PendingPatch::Type::DispatchRays) {
      PatchDispatchRays(*patchInfo, gpuAddressMappings);
    } else if (patchInfo->type == PendingPatch::Type::ExecuteIndirect) {
      PatchExecuteIndirect(*patchInfo, gpuAddressMappings);
    }
  }
}

void PatchService::PostExecuteCommandLists(ID3D12CommandQueue* queue,
                                           const unsigned* commandListKeys,
                                           size_t commandListCount) {
  for (size_t i = 0; i < commandListCount; ++i) {
    const unsigned key = commandListKeys[i];
    m_PendingPatchesByCommandList.erase(key);

    const auto itCurrentBuffers = m_CurrentPatchBuffersByCommandList.find(key);
    if (itCurrentBuffers == m_CurrentPatchBuffersByCommandList.end()) {
      continue;
    }
    for (unsigned patchBufferIndex : itCurrentBuffers->second) {
      const HRESULT hr = queue->Signal(m_PatchBufferFences[patchBufferIndex].fence,
                                       ++m_PatchBufferFences[patchBufferIndex].fenceValue);
      assert(SUCCEEDED(hr));
      m_PatchBufferFences[patchBufferIndex].waitingForExecute = false;
    }
    m_CurrentPatchBuffersByCommandList.erase(itCurrentBuffers);
  }
}

void PatchService::Initialize(ID3D12GraphicsCommandList* commandList) {
  if (m_Initialized) {
    return;
  }
  for (unsigned i = 0; i < m_PatchBufferInitialPoolSize; ++i) {
    AddPatchBuffer(commandList);
  }
  m_Initialized = true;
}

size_t PatchService::Align(size_t value, size_t alignment) {
  return ((value + alignment - 1) / alignment) * alignment;
}

void PatchService::AddPatchBuffer(ID3D12GraphicsCommandList* commandList) {
  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
  assert(SUCCEEDED(hr));

  D3D12_HEAP_PROPERTIES heapPropertiesUpload{};
  heapPropertiesUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapPropertiesUpload.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapPropertiesUpload.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapPropertiesUpload.CreationNodeMask = 1;
  heapPropertiesUpload.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  resourceDesc.Width = m_PatchBufferSize;

  m_PatchBuffers.emplace_back();
  hr = device->CreateCommittedResource(&heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                       D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                       IID_PPV_ARGS(&m_PatchBuffers[m_PatchBufferPoolSize]));
  assert(SUCCEEDED(hr));

  m_PatchBufferFences.emplace_back();
  hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                           IID_PPV_ARGS(&m_PatchBufferFences[m_PatchBufferPoolSize].fence));
  assert(SUCCEEDED(hr));

  ++m_PatchBufferPoolSize;
}

unsigned PatchService::GetPatchBufferIndex(unsigned commandListKey,
                                           ID3D12GraphicsCommandList* commandList) {
  for (unsigned i = 0; i < m_PatchBufferPoolSize; ++i) {
    if (m_PatchBufferFences[i].waitingForExecute) {
      continue;
    }
    const UINT64 value = m_PatchBufferFences[i].fence->GetCompletedValue();
    if (value == UINT64_MAX) {
      LOG_ERROR << "CCode PatchService - getPatchBufferIndex - device removed!";
      std::exit(EXIT_FAILURE);
    }
    if (value == m_PatchBufferFences[i].fenceValue) {
      m_CurrentPatchBuffersByCommandList[commandListKey].push_back(i);
      m_PatchBufferFences[i].waitingForExecute = true;
      return i;
    }
  }

  AddPatchBuffer(commandList);
  const unsigned newIndex = m_PatchBufferPoolSize - 1;
  m_CurrentPatchBuffersByCommandList[commandListKey].push_back(newIndex);
  m_PatchBufferFences[newIndex].waitingForExecute = true;
  return newIndex;
}

void PatchService::PatchBindingTableRecords(
    std::vector<char>& patchedData,
    unsigned stride,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  const unsigned recordCount = static_cast<unsigned>(patchedData.size() / stride);
  for (unsigned recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
    uint8_t* const p =
        reinterpret_cast<uint8_t*>(patchedData.data()) + static_cast<size_t>(recordIndex) * stride;

    CapturePlayerShaderIdentifierService::ShaderIdentifier captureShaderIdentifier;
    memcpy(captureShaderIdentifier.data(), p, captureShaderIdentifier.size());
    CapturePlayerShaderIdentifierService::ShaderIdentifier* playerShaderIdentifier =
        m_Services.ShaderIdentifiers.GetPlayerIdentifierByCaptureIdentifier(
            captureShaderIdentifier);
    if (playerShaderIdentifier) {
      memcpy(p, playerShaderIdentifier->data(), playerShaderIdentifier->size());
    }

    unsigned byteOffset = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    while (byteOffset < stride) {
      UINT64* address = reinterpret_cast<UINT64*>(p + byteOffset);
      if (!*address) {
        byteOffset += sizeof(UINT64);
        continue;
      }

      if (CapturePlayerDescriptorHandleService::DescriptorHeapInfo* heapInfo =
              m_Services.DescriptorHandles.GetViewDescriptorHeapInfoByCaptureHandle(*address)) {
        const UINT64 offset = *address - heapInfo->CaptureStart;
        *address = heapInfo->PlayerStart + offset;
        byteOffset += sizeof(UINT64);
        continue;
      }
      if (CapturePlayerDescriptorHandleService::DescriptorHeapInfo* heapInfo =
              m_Services.DescriptorHandles.GetSamplerDescriptorHeapInfoByCaptureHandle(*address)) {
        const UINT64 offset = *address - heapInfo->CaptureStart;
        *address = heapInfo->PlayerStart + offset;
        byteOffset += sizeof(UINT64);
        continue;
      }
      if (const UINT64 playerAddress = GetPlayerAddress(*address, gpuAddressMappings)) {
        *address = playerAddress;
        byteOffset += sizeof(UINT64);
        continue;
      }
      byteOffset += sizeof(UINT64);
    }
  }
}

void PatchService::PatchBuild(
    const PendingPatch& patchInfo,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  const std::filesystem::path path =
      m_CpuPatchDir / "instances_dump" / std::to_string(patchInfo.commandKey);
  std::vector<char> patchedData = ReadPatchBinaryFile(path);
  if (patchedData.empty()) {
    return;
  }

  if (patchedData.size() > m_PatchBufferSize) {
    LOG_ERROR << "CCode PatchService - patch buffer is too small!";
    std::exit(EXIT_FAILURE);
  }

  for (unsigned i = 0; i < patchInfo.numInstanceDescs; ++i) {
    auto* instance = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(
        patchedData.data() + sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * i);
    if (!instance->AccelerationStructure) {
      continue;
    }
    if (const UINT64 playerAddress =
            GetPlayerAddress(instance->AccelerationStructure, gpuAddressMappings)) {
      instance->AccelerationStructure = playerAddress;
    }
  }

  void* mappedData{};
  m_PatchBuffers[patchInfo.patchBufferIndex]->Map(0, nullptr, &mappedData);
  memcpy(mappedData, patchedData.data(), patchedData.size());
  m_PatchBuffers[patchInfo.patchBufferIndex]->Unmap(0, nullptr);
}

void PatchService::PatchDispatchRays(
    const PendingPatch& patchInfo,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  const D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc = patchInfo.dispatchRaysDesc;

  unsigned patchBufferOffset = 0;
  auto patchBindingTable = [&](D3D12_GPU_VIRTUAL_ADDRESS startAddress, UINT64 sizeInBytes,
                               UINT64 strideInBytes, const std::string& type) {
    if (!startAddress || !sizeInBytes) {
      return;
    }
    if (strideInBytes == 0) {
      strideInBytes = sizeInBytes;
    }
    const unsigned stride = static_cast<unsigned>(strideInBytes);
    const unsigned count = static_cast<unsigned>(sizeInBytes / strideInBytes);

    const std::filesystem::path path =
        m_CpuPatchDir / "binding_table_dump" / (std::to_string(patchInfo.commandKey) + "-" + type);
    std::vector<char> patchedData = ReadPatchBinaryFile(path);
    if (patchedData.empty()) {
      return;
    }

    if (patchedData.size() > m_PatchBufferSize) {
      LOG_ERROR << "CCode PatchService - patch buffer is too small!";
      std::exit(EXIT_FAILURE);
    }

    PatchBindingTableRecords(patchedData, stride, gpuAddressMappings);

    void* mappedData{};
    m_PatchBuffers[patchInfo.patchBufferIndex]->Map(0, nullptr, &mappedData);
    memcpy(static_cast<char*>(mappedData) + patchBufferOffset, patchedData.data(),
           patchedData.size());
    m_PatchBuffers[patchInfo.patchBufferIndex]->Unmap(0, nullptr);

    patchBufferOffset += stride * count;
    patchBufferOffset = static_cast<unsigned>(
        Align(patchBufferOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
  };

  patchBindingTable(dispatchRaysDesc.RayGenerationShaderRecord.StartAddress,
                    dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes,
                    dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes, "RayGeneration");
  patchBindingTable(dispatchRaysDesc.MissShaderTable.StartAddress,
                    dispatchRaysDesc.MissShaderTable.SizeInBytes,
                    dispatchRaysDesc.MissShaderTable.StrideInBytes, "Miss");
  patchBindingTable(dispatchRaysDesc.HitGroupTable.StartAddress,
                    dispatchRaysDesc.HitGroupTable.SizeInBytes,
                    dispatchRaysDesc.HitGroupTable.StrideInBytes, "HitGroup");
  patchBindingTable(dispatchRaysDesc.CallableShaderTable.StartAddress,
                    dispatchRaysDesc.CallableShaderTable.SizeInBytes,
                    dispatchRaysDesc.CallableShaderTable.StrideInBytes, "Callable");
}

void PatchService::PatchExecuteIndirect(
    const PendingPatch& patchInfo,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  const auto commandSignaturesIt = m_CommandSignatures.find(patchInfo.commandSignatureKey);
  assert(commandSignaturesIt != m_CommandSignatures.end());
  const D3D12_COMMAND_SIGNATURE_DESC& commandSignature = commandSignaturesIt->second.desc;

  bool view = false;
  bool raytracing = false;
  for (unsigned i = 0; i < commandSignature.NumArgumentDescs; ++i) {
    const D3D12_INDIRECT_ARGUMENT_TYPE type = commandSignature.pArgumentDescs[i].Type;
    if (type == D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW ||
        type == D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW) {
      view = true;
    } else if (type == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS) {
      raytracing = true;
    }
  }
  assert(view || raytracing);

  unsigned raytracingPatchBuffer{};
  unsigned raytracingPatchBufferOffset{};
  std::unordered_map<D3D12_GPU_VIRTUAL_ADDRESS, D3D12_GPU_VIRTUAL_ADDRESS> raytracingPatchMapping;

  auto patchBindingTable = [&](D3D12_GPU_VIRTUAL_ADDRESS startAddress, UINT64 sizeInBytes,
                               UINT64 strideInBytes, unsigned index, const std::string& type) {
    if (!startAddress || !sizeInBytes) {
      return;
    }
    if (strideInBytes == 0) {
      strideInBytes = sizeInBytes;
    }
    const unsigned stride = static_cast<unsigned>(strideInBytes);
    const unsigned count = static_cast<unsigned>(sizeInBytes / strideInBytes);

    const std::filesystem::path path =
        m_CpuPatchDir / "execute_indirect_dump" / "binding_tables" /
        (std::to_string(patchInfo.commandKey) + "-" + std::to_string(index) + "-" + type);
    std::vector<char> patchedData = ReadPatchBinaryFile(path);
    if (patchedData.empty()) {
      return;
    }

    if (patchedData.size() > m_PatchBufferSize) {
      LOG_ERROR << "CCode PatchService - patch buffer is too small!";
      std::exit(EXIT_FAILURE);
    }

    PatchBindingTableRecords(patchedData, stride, gpuAddressMappings);

    void* mappedData{};
    m_PatchBuffers[raytracingPatchBuffer]->Map(0, nullptr, &mappedData);
    memcpy(static_cast<char*>(mappedData) + raytracingPatchBufferOffset, patchedData.data(),
           patchedData.size());
    m_PatchBuffers[raytracingPatchBuffer]->Unmap(0, nullptr);

    raytracingPatchMapping[startAddress] =
        m_PatchBuffers[raytracingPatchBuffer]->GetGPUVirtualAddress() + raytracingPatchBufferOffset;

    raytracingPatchBufferOffset += stride * count;
    raytracingPatchBufferOffset = static_cast<unsigned>(
        Align(raytracingPatchBufferOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
  };

  const auto dispatchRaysIt = m_ExecuteIndirectDispatchRays.find(patchInfo.commandKey);
  if (dispatchRaysIt != m_ExecuteIndirectDispatchRays.end()) {
    for (unsigned i = 0; i < dispatchRaysIt->second.size(); ++i) {
      const D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc = dispatchRaysIt->second.at(i);
      raytracingPatchBuffer = GetPatchBufferIndex(patchInfo.commandListKey, patchInfo.commandList);
      raytracingPatchBufferOffset = 0;

      patchBindingTable(dispatchRaysDesc.RayGenerationShaderRecord.StartAddress,
                        dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes,
                        dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes, i, "RayGeneration");
      patchBindingTable(dispatchRaysDesc.MissShaderTable.StartAddress,
                        dispatchRaysDesc.MissShaderTable.SizeInBytes,
                        dispatchRaysDesc.MissShaderTable.StrideInBytes, i, "Miss");
      patchBindingTable(dispatchRaysDesc.HitGroupTable.StartAddress,
                        dispatchRaysDesc.HitGroupTable.SizeInBytes,
                        dispatchRaysDesc.HitGroupTable.StrideInBytes, i, "HitGroup");
      patchBindingTable(dispatchRaysDesc.CallableShaderTable.StartAddress,
                        dispatchRaysDesc.CallableShaderTable.SizeInBytes,
                        dispatchRaysDesc.CallableShaderTable.StrideInBytes, i, "Callable");
    }
  }

  {
    const std::filesystem::path path = m_CpuPatchDir / "execute_indirect_dump" /
                                       (std::to_string(patchInfo.commandKey) + "-ArgumentBuffer");
    std::vector<char> patchedData = ReadPatchBinaryFile(path);
    if (patchedData.empty()) {
      return;
    }

    if (patchedData.size() > m_PatchBufferSize) {
      LOG_ERROR << "CCode PatchService - patch buffer is too small!";
      std::exit(EXIT_FAILURE);
    }

    const unsigned stride = commandSignature.ByteStride;
    const unsigned argumentCount = patchInfo.maxCommandCount;
    for (unsigned argumentIndex = 0; argumentIndex < argumentCount; ++argumentIndex) {
      uint8_t* const p = reinterpret_cast<uint8_t*>(patchedData.data()) + argumentIndex * stride;

      unsigned byteOffset = 0;
      while (byteOffset < stride) {
        UINT64* address = reinterpret_cast<UINT64*>(p + byteOffset);
        if (!*address) {
          byteOffset += sizeof(UINT32);
          continue;
        }

        if (raytracing) {
          const auto mappingIt = raytracingPatchMapping.find(*address);
          if (mappingIt != raytracingPatchMapping.end()) {
            *address = mappingIt->second;
            byteOffset += sizeof(UINT32);
            continue;
          }
        }

        if (view) {
          if (const UINT64 playerAddress = GetPlayerAddress(*address, gpuAddressMappings)) {
            *address = playerAddress;
            byteOffset += sizeof(UINT32);
            continue;
          }
        }
        byteOffset += sizeof(UINT32);
      }
    }

    void* mappedData{};
    m_PatchBuffers[patchInfo.patchBufferIndex]->Map(0, nullptr, &mappedData);
    memcpy(mappedData, patchedData.data(), patchedData.size());
    m_PatchBuffers[patchInfo.patchBufferIndex]->Unmap(0, nullptr);
  }
}

std::vector<char> PatchService::ReadPatchBinaryFile(const std::filesystem::path& path) {
  std::vector<char> data;
  if (!std::filesystem::exists(path)) {
    LOG_ERROR << "CCode PatchService - missing file: " << path.string();
    return data;
  }
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.good()) {
    LOG_ERROR << "CCode PatchService - failed to open file: " << path.string();
    return data;
  }
  const std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  data.resize(static_cast<size_t>(size));
  if (!file.read(data.data(), size)) {
    LOG_ERROR << "CCode PatchService - failed to read file: " << path.string();
    data.clear();
  }
  return data;
}

UINT64 PatchService::GetPlayerAddress(
    UINT64 captureAddress,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  int first = 0;
  int last = static_cast<int>(gpuAddressMappings.size()) - 1;
  while (first <= last) {
    const int mid = first + (last - first) / 2;
    const auto& mapping = gpuAddressMappings[mid];
    if (captureAddress >= mapping.CaptureStart &&
        captureAddress < mapping.CaptureStart + mapping.Size) {
      const uint64_t offset = captureAddress - mapping.CaptureStart;
      return mapping.PlayerStart + offset;
    }
    if (captureAddress >= mapping.CaptureStart + mapping.Size) {
      first = mid + 1;
    } else {
      last = mid - 1;
    }
  }
  return 0;
}

void PatchService::LoadExecuteIndirectDispatchRays() {
  const std::filesystem::path path = m_DataDir / "executeIndirectRaytracing.txt";
  std::ifstream stream(path);
  if (!stream) {
    return;
  }
  while (true) {
    unsigned callKey{};
    D3D12_DISPATCH_RAYS_DESC desc{};
    stream >> callKey;
    if (!stream) {
      break;
    }
    stream >> desc.RayGenerationShaderRecord.StartAddress >>
        desc.RayGenerationShaderRecord.SizeInBytes;
    stream >> desc.MissShaderTable.StartAddress >> desc.MissShaderTable.SizeInBytes >>
        desc.MissShaderTable.StrideInBytes;
    stream >> desc.HitGroupTable.StartAddress >> desc.HitGroupTable.SizeInBytes >>
        desc.HitGroupTable.StrideInBytes;
    stream >> desc.CallableShaderTable.StartAddress >> desc.CallableShaderTable.SizeInBytes >>
        desc.CallableShaderTable.StrideInBytes;
    stream >> desc.Width >> desc.Height >> desc.Depth;
    m_ExecuteIndirectDispatchRays[callKey].push_back(desc);
  }
}

} // namespace directx
