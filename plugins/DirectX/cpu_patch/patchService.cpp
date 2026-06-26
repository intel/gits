// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "patchService.h"
#include "pluginUtils.h"
#include "log.h"

#include "wrl.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <string>

namespace gits {
namespace DirectX {

PatchService::PatchService(const Configuration& gitsConfig,
                           CapturePlayerGpuAddressService& addressService)
    : m_GitsConfig(gitsConfig), m_AddressService(addressService) {
  m_Path = gitsConfig.common.player.streamDir / "cpu_patch";
  LOG_INFO << "CpuPatch - data location: " << m_Path.string();
  if (!std::filesystem::exists(m_Path)) {
    LOG_ERROR << "CpuPatch - data location does not exist";
  }

  size_t instancesFilesCount{};
  size_t instancesMaxSize{};
  if (std::filesystem::exists(m_Path / "instances_dump")) {
    for (const auto& entry : std::filesystem::directory_iterator(m_Path / "instances_dump")) {
      if (std::filesystem::is_regular_file(entry.path())) {
        ++instancesFilesCount;
        if (entry.file_size() > instancesMaxSize) {
          instancesMaxSize = entry.file_size();
        }
      }
    }
  }
  LOG_INFO << "CpuPatch - instances files found: " << std::to_string(instancesFilesCount);

  size_t bindingTableFilesCount{};
  std::unordered_map<unsigned, size_t> bindingTablesSizes;
  if (std::filesystem::exists(m_Path / "binding_table_dump")) {
    for (const auto& entry : std::filesystem::directory_iterator(m_Path / "binding_table_dump")) {
      if (std::filesystem::is_regular_file(entry.path())) {
        ++bindingTableFilesCount;
        size_t pos = entry.path().filename().string().find("-");
        unsigned key = std::stoul(entry.path().filename().string().substr(0, pos));
        bindingTablesSizes[key] += entry.file_size();
      }
    }
  }
  if (std::filesystem::exists(m_Path / "execute_indirect_dump" / "binding_tables")) {
    for (const auto& entry :
         std::filesystem::directory_iterator(m_Path / "execute_indirect_dump" / "binding_tables")) {
      if (std::filesystem::is_regular_file(entry.path())) {
        ++bindingTableFilesCount;
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
  LOG_INFO << "CpuPatch - binding table files found: " << std::to_string(bindingTableFilesCount);

  size_t argumentBufferFilesCount{};
  size_t argumentBufferMaxSize{};
  if (std::filesystem::exists(m_Path / "execute_indirect_dump")) {
    for (const auto& entry :
         std::filesystem::directory_iterator(m_Path / "execute_indirect_dump")) {
      if (std::filesystem::is_regular_file(entry.path())) {
        ++argumentBufferFilesCount;
        if (entry.file_size() > argumentBufferMaxSize) {
          argumentBufferMaxSize = entry.file_size();
        }
      }
    }
  }
  LOG_INFO << "CpuPatch - argument buffer files found: "
           << std::to_string(argumentBufferFilesCount);

  m_PatchBufferSize = std::max({instancesMaxSize, bindingTablesMaxSize, argumentBufferMaxSize});
  constexpr unsigned maxNumOffsetsPerPatchBuffer = 4;
  m_PatchBufferSize = Align(m_PatchBufferSize + maxNumOffsetsPerPatchBuffer *
                                                    D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
                            D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
  LOG_INFO << "CpuPatch - patch buffer size: " << FormatMemorySize(m_PatchBufferSize);

  LoadExecuteIndirectDispatchRays();
}

void PatchService::PreBuildRTAS(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) {
  if (command.m_pDesc.Value->Inputs.Type !=
          D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL ||
      command.m_pDesc.Value->Inputs.NumDescs == 0) {
    return;
  }

  ID3D12GraphicsCommandList* commandList = command.m_Object.Value;
  if (!m_Initialized) {
    Initialize(commandList);
  }

  unsigned patchBufferIndex = GetPatchBufferIndex(command.m_Object.Key, command.m_Object.Value);
  command.m_pDesc.Value->Inputs.InstanceDescs =
      m_PatchBuffers[patchBufferIndex]->GetGPUVirtualAddress();

  m_PatchInfoByCommandList[command.m_Object.Key].emplace_back(new PatchInfo());
  auto* patchInfo = m_PatchInfoByCommandList[command.m_Object.Key].back().get();
  patchInfo->PatchBufferIndex = patchBufferIndex;
  patchInfo->Type = PatchInfo::Build;
  patchInfo->Command.reset(
      new ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand(command));
}

void PatchService::PreDispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& command) {
  ID3D12GraphicsCommandList* commandList = command.m_Object.Value;
  if (!m_Initialized) {
    Initialize(commandList);
  }

  unsigned patchBufferIndex = GetPatchBufferIndex(command.m_Object.Key, command.m_Object.Value);
  D3D12_GPU_VIRTUAL_ADDRESS patchBufferAddress =
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
    unsigned stride = strideInBytes;
    unsigned count = sizeInBytes / strideInBytes;

    startAddress = patchBufferAddress + patchBufferOffset;

    patchBufferOffset += stride * count;
    patchBufferOffset = Align(patchBufferOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
  };

  D3D12_DISPATCH_RAYS_DESC& DispatchRaysDesc = *command.m_pDesc.Value;

  replaceBindingTable(DispatchRaysDesc.RayGenerationShaderRecord.StartAddress,
                      DispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes,
                      DispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes);
  replaceBindingTable(DispatchRaysDesc.MissShaderTable.StartAddress,
                      DispatchRaysDesc.MissShaderTable.SizeInBytes,
                      DispatchRaysDesc.MissShaderTable.StrideInBytes);
  replaceBindingTable(DispatchRaysDesc.HitGroupTable.StartAddress,
                      DispatchRaysDesc.HitGroupTable.SizeInBytes,
                      DispatchRaysDesc.HitGroupTable.StrideInBytes);
  replaceBindingTable(DispatchRaysDesc.CallableShaderTable.StartAddress,
                      DispatchRaysDesc.CallableShaderTable.SizeInBytes,
                      DispatchRaysDesc.CallableShaderTable.StrideInBytes);

  m_PatchInfoByCommandList[command.m_Object.Key].emplace_back(new PatchInfo());
  auto* patchInfo = m_PatchInfoByCommandList[command.m_Object.Key].back().get();
  patchInfo->PatchBufferIndex = patchBufferIndex;
  patchInfo->Type = PatchInfo::DispatchRays;
  patchInfo->Command.reset(new ID3D12GraphicsCommandList4DispatchRaysCommand(command));
}

void PatchService::PreExecuteIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& command) {
  m_ExecuteIndirectLastArgumentBufferOffset = command.m_ArgumentBufferOffset.Value;

  auto commandSignaturesIt = m_CommandSignatures.find(command.m_pCommandSignature.Key);
  GITS_ASSERT(commandSignaturesIt != m_CommandSignatures.end());
  const D3D12_COMMAND_SIGNATURE_DESC& commandSignature = *commandSignaturesIt->second->Value;

  bool view = false;
  bool raytracing = false;
  for (unsigned i = 0; i < commandSignature.NumArgumentDescs; ++i) {
    D3D12_INDIRECT_ARGUMENT_TYPE type = commandSignature.pArgumentDescs[i].Type;
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

  ID3D12GraphicsCommandList* commandList = command.m_Object.Value;
  if (!m_Initialized) {
    Initialize(commandList);
  }

  unsigned patchBufferIndex = GetPatchBufferIndex(command.m_Object.Key, command.m_Object.Value);

  command.m_pArgumentBuffer.Value = m_PatchBuffers[patchBufferIndex];
  command.m_ArgumentBufferOffset.Value = 0;

  m_PatchInfoByCommandList[command.m_Object.Key].emplace_back(new PatchInfo());
  auto* patchInfo = m_PatchInfoByCommandList[command.m_Object.Key].back().get();
  patchInfo->PatchBufferIndex = patchBufferIndex;
  patchInfo->Type = PatchInfo::ExecuteIndirect;
  patchInfo->Command.reset(new ID3D12GraphicsCommandListExecuteIndirectCommand(command));
}

void PatchService::PostExecuteIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& command) {
  command.m_ArgumentBufferOffset.Value = m_ExecuteIndirectLastArgumentBufferOffset;
}

void PatchService::PreExecute(ID3D12CommandQueueExecuteCommandListsCommand& command) {
  std::vector<PatchInfo*> patchInfos;
  for (unsigned key : command.m_ppCommandLists.Keys) {
    auto it = m_PatchInfoByCommandList.find(key);
    if (it != m_PatchInfoByCommandList.end()) {
      for (auto& itPatchInfo : it->second) {
        patchInfos.push_back(itPatchInfo.get());
      }
    }
  }

  if (patchInfos.empty()) {
    return;
  }

  std::vector<CapturePlayerGpuAddressService::GpuAddressMapping> gpuAddressMappings;
  m_AddressService.GetMappings(gpuAddressMappings);

  for (auto* patchInfo : patchInfos) {
    if (patchInfo->Type == PatchInfo::Build) {
      PatchBuild(*patchInfo, gpuAddressMappings);
    } else if (patchInfo->Type == PatchInfo::DispatchRays) {
      PatchDispatchRays(*patchInfo, gpuAddressMappings);
    } else if (patchInfo->Type == PatchInfo::ExecuteIndirect) {
      PatchExecuteIndirect(*patchInfo, gpuAddressMappings);
    }
  }
}

void PatchService::PostExecute(ID3D12CommandQueueExecuteCommandListsCommand& command) {
  for (unsigned key : command.m_ppCommandLists.Keys) {
    auto itPatchInfo = m_PatchInfoByCommandList.find(key);
    if (itPatchInfo != m_PatchInfoByCommandList.end()) {
      m_PatchInfoByCommandList.erase(itPatchInfo);
    }

    auto itCurrentBuffers = m_CurrentPatchBuffersByCommandList.find(key);
    if (itCurrentBuffers != m_CurrentPatchBuffersByCommandList.end()) {
      for (unsigned patchBufferIndex : itCurrentBuffers->second) {
        HRESULT hr =
            command.m_Object.Value->Signal(m_PatchBufferFences[patchBufferIndex].Fence,
                                           ++m_PatchBufferFences[patchBufferIndex].FenceValue);
        GITS_ASSERT(hr == S_OK);
        m_PatchBufferFences[patchBufferIndex].WaitingForExecute = false;
      }
      m_CurrentPatchBuffersByCommandList.erase(itCurrentBuffers);
    }
  }
}

void PatchService::ReleaseObject(IUnknownReleaseCommand& command) {
  if (command.m_Result.Value == 0) {
    m_DescriptorHandleService.DestroyHeap(command.m_Object.Key);
  }
}

void PatchService::PreGetShaderId(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& command) {
  ShaderIdentifierService::ShaderIdentifier shaderIdentifier;
  memcpy(shaderIdentifier.data(), command.m_Result.Value, shaderIdentifier.size());
  m_ShaderIdentifierService.AddCaptureShaderIdentifier(command.Key, shaderIdentifier,
                                                       command.m_pExportName.Value);
}

void PatchService::PostGetShaderId(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& command) {
  ShaderIdentifierService::ShaderIdentifier shaderIdentifier;
  memcpy(shaderIdentifier.data(), command.m_Result.Value, shaderIdentifier.size());
  m_ShaderIdentifierService.AddPlayerShaderIdentifier(command.Key, shaderIdentifier,
                                                      command.m_pExportName.Value);
}

void PatchService::PreGetDescriptorHandle(
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) {
  m_DescriptorHandleService.AddCaptureHandle(command.m_Object.Value, command.m_Object.Key,
                                             command.m_Result.Value);
}

void PatchService::PostGetDescriptorHandle(
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) {
  m_DescriptorHandleService.AddPlayerHandle(command.m_Object.Key, command.m_Result.Value);
}

void PatchService::CreateCommandSignature(
    unsigned commandSignatureKey, const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& desc) {
  m_CommandSignatures[commandSignatureKey].reset(
      new PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>(desc));
}

void PatchService::Initialize(ID3D12GraphicsCommandList* commandList) {
  if (m_Initialized) {
    return;
  }

  for (unsigned i = 0; i < PatchBufferInitialPoolSize; ++i) {
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
  GITS_ASSERT(hr == S_OK);

  D3D12_HEAP_PROPERTIES heapPropertiesUpload{};
  heapPropertiesUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapPropertiesUpload.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapPropertiesUpload.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapPropertiesUpload.CreationNodeMask = 1;
  heapPropertiesUpload.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  resourceDesc.Width = m_PatchBufferSize;
  m_PatchBuffers.emplace_back();
  hr = device->CreateCommittedResource(&heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                       D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                       IID_PPV_ARGS(&m_PatchBuffers[m_PatchBufferPoolSize]));
  GITS_ASSERT(hr == S_OK);

  m_PatchBufferFences.emplace_back();
  hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                           IID_PPV_ARGS(&m_PatchBufferFences[m_PatchBufferPoolSize].Fence));
  GITS_ASSERT(hr == S_OK);

  ++m_PatchBufferPoolSize;
}

unsigned PatchService::GetPatchBufferIndex(unsigned commandListKey,
                                           ID3D12GraphicsCommandList* commandList) {
  for (unsigned i = 0; i < m_PatchBufferPoolSize; ++i) {
    if (m_PatchBufferFences[i].WaitingForExecute) {
      continue;
    }
    UINT64 value = m_PatchBufferFences[i].Fence->GetCompletedValue();
    if (value == UINT64_MAX) {
      LOG_ERROR << "CpuPatch - GetPatchBufferIndex - device removed!";
      exit(EXIT_FAILURE);
    }
    if (value == m_PatchBufferFences[i].FenceValue) {
      m_CurrentPatchBuffersByCommandList[commandListKey].push_back(i);
      m_PatchBufferFences[i].WaitingForExecute = true;
      return i;
    }
  }

  AddPatchBuffer(commandList);
  unsigned newIndex = m_PatchBufferPoolSize - 1;
  m_CurrentPatchBuffersByCommandList[commandListKey].push_back(newIndex);
  m_PatchBufferFences[newIndex].WaitingForExecute = true;

  return newIndex;
}

void PatchService::PatchBuild(
    const PatchInfo& patchInfo,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  std::filesystem::path path = m_Path / "instances_dump" / std::to_string(patchInfo.Command->Key);
  std::vector<char> patchedData = ReadFile(path);
  if (patchedData.empty()) {
    return;
  }

  if (patchedData.size() > m_PatchBufferSize) {
    LOG_ERROR << "CpuPatch - patch buffer is too small!";
    exit(EXIT_FAILURE);
  }

  auto* command =
      static_cast<ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand*>(
          patchInfo.Command.get());
  for (unsigned i = 0; i < command->m_pDesc.Value->Inputs.NumDescs; ++i) {
    D3D12_RAYTRACING_INSTANCE_DESC* instance = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(
        reinterpret_cast<char*>(patchedData.data()) + sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * i);
    if (!instance->AccelerationStructure) {
      continue;
    }
    UINT64 playerAddress = GetPlayerAddress(instance->AccelerationStructure, gpuAddressMappings);
    if (playerAddress) {
      instance->AccelerationStructure = playerAddress;
    }
  }

  void* mappedData{};
  m_PatchBuffers[patchInfo.PatchBufferIndex]->Map(0, nullptr, &mappedData);
  memcpy(mappedData, patchedData.data(), patchedData.size());
  m_PatchBuffers[patchInfo.PatchBufferIndex]->Unmap(0, nullptr);
}

void PatchService::PatchDispatchRays(
    const PatchInfo& patchInfo,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  auto* command =
      static_cast<ID3D12GraphicsCommandList4DispatchRaysCommand*>(patchInfo.Command.get());
  D3D12_DISPATCH_RAYS_DESC& DispatchRaysDesc = *command->m_pDesc.Value;

  unsigned patchBufferOffset = 0;
  auto patchBindingTable = [&](D3D12_GPU_VIRTUAL_ADDRESS startAddress, UINT64 sizeInBytes,
                               UINT64 strideInBytes, const std::string type) {
    if (!startAddress || !sizeInBytes) {
      return;
    }

    if (strideInBytes == 0) {
      strideInBytes = sizeInBytes;
    }
    unsigned stride = strideInBytes;
    unsigned count = sizeInBytes / strideInBytes;

    std::filesystem::path path =
        m_Path / "binding_table_dump" / (std::to_string(patchInfo.Command->Key) + "-" + type);
    std::vector<char> patchedData = ReadFile(path);
    if (patchedData.empty()) {
      return;
    }

    if (patchedData.size() > m_PatchBufferSize) {
      LOG_ERROR << "CpuPatch - patch buffer is too small!";
      exit(EXIT_FAILURE);
    }

    //patch
    {
      unsigned recordCount = patchedData.size() / stride;
      for (unsigned recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
        uint8_t* const p = reinterpret_cast<uint8_t*>(patchedData.data()) + recordIndex * stride;

        ShaderIdentifierService::ShaderIdentifier captureShaderIdentifier;
        memcpy(captureShaderIdentifier.data(), p, captureShaderIdentifier.size());
        ShaderIdentifierService::ShaderIdentifier* playerShaderIdentifier =
            m_ShaderIdentifierService.GetPlayerIdentifierByCaptureIdentifier(
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

          {
            DescriptorHandleService::DescriptorHeapInfo* heapInfo =
                m_DescriptorHandleService.GetViewDescriptorHeapInfoByCaptureHandle(*address);
            if (heapInfo) {
              UINT64 offset = *address - heapInfo->CaptureStart;
              *address = heapInfo->PlayerStart + offset;
              byteOffset += sizeof(UINT64);
              continue;
            }
          }
          {
            DescriptorHandleService::DescriptorHeapInfo* heapInfo =
                m_DescriptorHandleService.GetSamplerDescriptorHeapInfoByCaptureHandle(*address);
            if (heapInfo) {
              UINT64 offset = *address - heapInfo->CaptureStart;
              *address = heapInfo->PlayerStart + offset;
              byteOffset += sizeof(UINT64);
              continue;
            }
          }
          {
            UINT64 playerAddress = GetPlayerAddress(*address, gpuAddressMappings);
            if (playerAddress) {
              *address = playerAddress;
              byteOffset += sizeof(UINT64);
              continue;
            }
          }
          byteOffset += sizeof(UINT64);
        }
      }
    }

    void* mappedData{};
    m_PatchBuffers[patchInfo.PatchBufferIndex]->Map(0, nullptr, &mappedData);
    memcpy(static_cast<char*>(mappedData) + patchBufferOffset, patchedData.data(),
           patchedData.size());
    m_PatchBuffers[patchInfo.PatchBufferIndex]->Unmap(0, nullptr);

    patchBufferOffset += stride * count;
    patchBufferOffset = Align(patchBufferOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
  };

  patchBindingTable(DispatchRaysDesc.RayGenerationShaderRecord.StartAddress,
                    DispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes,
                    DispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes, "RayGeneration");
  patchBindingTable(DispatchRaysDesc.MissShaderTable.StartAddress,
                    DispatchRaysDesc.MissShaderTable.SizeInBytes,
                    DispatchRaysDesc.MissShaderTable.StrideInBytes, "Miss");
  patchBindingTable(DispatchRaysDesc.HitGroupTable.StartAddress,
                    DispatchRaysDesc.HitGroupTable.SizeInBytes,
                    DispatchRaysDesc.HitGroupTable.StrideInBytes, "HitGroup");
  patchBindingTable(DispatchRaysDesc.CallableShaderTable.StartAddress,
                    DispatchRaysDesc.CallableShaderTable.SizeInBytes,
                    DispatchRaysDesc.CallableShaderTable.StrideInBytes, "Callable");
}

void PatchService::PatchExecuteIndirect(
    const PatchInfo& patchInfo,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  auto* command =
      static_cast<ID3D12GraphicsCommandListExecuteIndirectCommand*>(patchInfo.Command.get());

  auto it = m_CommandSignatures.find(command->m_pCommandSignature.Key);
  GITS_ASSERT(it != m_CommandSignatures.end());
  const D3D12_COMMAND_SIGNATURE_DESC& commandSignature = *it->second->Value;

  bool view = false;
  bool raytracing = false;
  for (unsigned i = 0; i < commandSignature.NumArgumentDescs; ++i) {
    D3D12_INDIRECT_ARGUMENT_TYPE type = commandSignature.pArgumentDescs[i].Type;
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

  GITS_ASSERT(view || raytracing);

  unsigned raytracingPatchBuffer{};
  unsigned raytracingPatchBufferOffset{};
  std::unordered_map<D3D12_GPU_VIRTUAL_ADDRESS, D3D12_GPU_VIRTUAL_ADDRESS> raytracingPatchMapping;

  auto patchBindingTable = [&](D3D12_GPU_VIRTUAL_ADDRESS startAddress, UINT64 sizeInBytes,
                               UINT64 strideInBytes, unsigned index, const std::string type) {
    if (!startAddress || !sizeInBytes) {
      return;
    }

    if (strideInBytes == 0) {
      strideInBytes = sizeInBytes;
    }
    unsigned stride = strideInBytes;
    unsigned count = sizeInBytes / strideInBytes;

    std::filesystem::path path =
        m_Path / "execute_indirect_dump" / "binding_tables" /
        (std::to_string(patchInfo.Command->Key) + "-" + std::to_string(index) + "-" + type);
    std::vector<char> patchedData = ReadFile(path);
    if (patchedData.empty()) {
      return;
    }

    if (patchedData.size() > m_PatchBufferSize) {
      LOG_ERROR << "CpuPatch - patch buffer is too small!";
      exit(EXIT_FAILURE);
    }

    //patch
    {
      unsigned recordCount = patchedData.size() / stride;
      for (unsigned recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
        uint8_t* const p = reinterpret_cast<uint8_t*>(patchedData.data()) + recordIndex * stride;

        ShaderIdentifierService::ShaderIdentifier captureShaderIdentifier;
        memcpy(captureShaderIdentifier.data(), p, captureShaderIdentifier.size());
        ShaderIdentifierService::ShaderIdentifier* playerShaderIdentifier =
            m_ShaderIdentifierService.GetPlayerIdentifierByCaptureIdentifier(
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

          {
            DescriptorHandleService::DescriptorHeapInfo* heapInfo =
                m_DescriptorHandleService.GetViewDescriptorHeapInfoByCaptureHandle(*address);
            if (heapInfo) {
              UINT64 offset = *address - heapInfo->CaptureStart;
              *address = heapInfo->PlayerStart + offset;
              byteOffset += sizeof(UINT64);
              continue;
            }
          }
          {
            DescriptorHandleService::DescriptorHeapInfo* heapInfo =
                m_DescriptorHandleService.GetSamplerDescriptorHeapInfoByCaptureHandle(*address);
            if (heapInfo) {
              UINT64 offset = *address - heapInfo->CaptureStart;
              *address = heapInfo->PlayerStart + offset;
              byteOffset += sizeof(UINT64);
              continue;
            }
          }
          {
            UINT64 playerAddress = GetPlayerAddress(*address, gpuAddressMappings);
            if (playerAddress) {
              *address = playerAddress;
              byteOffset += sizeof(UINT64);
              continue;
            }
          }
          byteOffset += sizeof(UINT64);
        }
      }
    }

    void* mappedData{};
    m_PatchBuffers[raytracingPatchBuffer]->Map(0, nullptr, &mappedData);
    memcpy(static_cast<char*>(mappedData) + raytracingPatchBufferOffset, patchedData.data(),
           patchedData.size());
    m_PatchBuffers[raytracingPatchBuffer]->Unmap(0, nullptr);

    raytracingPatchMapping[startAddress] =
        m_PatchBuffers[raytracingPatchBuffer]->GetGPUVirtualAddress() + raytracingPatchBufferOffset;

    raytracingPatchBufferOffset += stride * count;
    raytracingPatchBufferOffset =
        Align(raytracingPatchBufferOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
  };

  auto DispatchRaysIt = m_ExecuteIndirectDispatchRays.find(command->Key);
  if (DispatchRaysIt != m_ExecuteIndirectDispatchRays.end()) {
    for (unsigned i = 0; i < DispatchRaysIt->second.size(); ++i) {
      const D3D12_DISPATCH_RAYS_DESC& DispatchRaysDesc = DispatchRaysIt->second.at(i);

      raytracingPatchBuffer = GetPatchBufferIndex(command->m_Object.Key, command->m_Object.Value);
      raytracingPatchBufferOffset = 0;

      patchBindingTable(DispatchRaysDesc.RayGenerationShaderRecord.StartAddress,
                        DispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes,
                        DispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes, i, "RayGeneration");
      patchBindingTable(DispatchRaysDesc.MissShaderTable.StartAddress,
                        DispatchRaysDesc.MissShaderTable.SizeInBytes,
                        DispatchRaysDesc.MissShaderTable.StrideInBytes, i, "Miss");
      patchBindingTable(DispatchRaysDesc.HitGroupTable.StartAddress,
                        DispatchRaysDesc.HitGroupTable.SizeInBytes,
                        DispatchRaysDesc.HitGroupTable.StrideInBytes, i, "HitGroup");
      patchBindingTable(DispatchRaysDesc.CallableShaderTable.StartAddress,
                        DispatchRaysDesc.CallableShaderTable.SizeInBytes,
                        DispatchRaysDesc.CallableShaderTable.StrideInBytes, i, "Callable");
    }
  }

  {
    std::filesystem::path path = m_Path / "execute_indirect_dump" /
                                 (std::to_string(patchInfo.Command->Key) + "-ArgumentBuffer");
    std::vector<char> patchedData = ReadFile(path);
    if (patchedData.empty()) {
      return;
    }

    if (patchedData.size() > m_PatchBufferSize) {
      LOG_ERROR << "CpuPatch - patch buffer is too small!";
      exit(EXIT_FAILURE);
    }

    unsigned stride = commandSignature.ByteStride;
    unsigned argumentCount = command->m_MaxCommandCount.Value;
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
          UINT64 playerAddress = GetPlayerAddress(*address, gpuAddressMappings);
          if (playerAddress) {
            *address = playerAddress;
            byteOffset += sizeof(UINT32);
            continue;
          }
        }
        byteOffset += sizeof(UINT32);
      }
    }

    void* mappedData{};
    m_PatchBuffers[patchInfo.PatchBufferIndex]->Map(0, nullptr, &mappedData);
    memcpy(mappedData, patchedData.data(), patchedData.size());
    m_PatchBuffers[patchInfo.PatchBufferIndex]->Unmap(0, nullptr);
  }
}

std::vector<char> PatchService::ReadFile(std::filesystem::path path) {
  std::vector<char> data;

  if (!std::filesystem::exists(path)) {
    LOG_ERROR << "CpuPatch - missing file: " << path.string();
    return data;
  }
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.good()) {
    LOG_ERROR << "CpuPatch - failed to open file: " << path.string();
    return data;
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  data.resize(size);
  if (!file.read(data.data(), size)) {
    LOG_ERROR << "CpuPatch - failed to read file: " << path.string();
    return data;
  }

  return data;
}

UINT64 PatchService::GetPlayerAddress(
    UINT64 captureAddress,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  int first = 0;
  int last = gpuAddressMappings.size() - 1;
  while (first <= last) {
    int mid = first + (last - first) / 2;
    const auto& mapping = gpuAddressMappings[mid];
    if (captureAddress >= mapping.CaptureStart &&
        captureAddress < mapping.CaptureStart + mapping.Size) {
      uint64_t offset = captureAddress - mapping.CaptureStart;
      uint64_t playbackAddress = mapping.PlayerStart + offset;
      return playbackAddress;
    } else if (captureAddress >= mapping.CaptureStart + mapping.Size) {
      first = mid + 1;
    } else {
      last = mid - 1;
    }
  }
  return 0;
}

void PatchService::LoadExecuteIndirectDispatchRays() {
  std::filesystem::path dumpPath = m_GitsConfig.common.player.streamDir;
  std::ifstream stream(dumpPath / "executeIndirectRaytracing.txt");
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

} // namespace DirectX
} // namespace gits
