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
    : gitsConfig_(gitsConfig), addressService_(addressService) {
  path_ = gitsConfig.common.player.streamDir / "cpu_patch";
  LOG_INFO << "CpuPatch - data location: " << path_.string();
  if (!std::filesystem::exists(path_)) {
    LOG_ERROR << "CpuPatch - data location does not exist";
  }

  size_t instancesFilesCount{};
  size_t instancesMaxSize{};
  if (std::filesystem::exists(path_ / "instances_dump")) {
    for (const auto& entry : std::filesystem::directory_iterator(path_ / "instances_dump")) {
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
  if (std::filesystem::exists(path_ / "binding_table_dump")) {
    for (const auto& entry : std::filesystem::directory_iterator(path_ / "binding_table_dump")) {
      if (std::filesystem::is_regular_file(entry.path())) {
        ++bindingTableFilesCount;
        size_t pos = entry.path().filename().string().find("-");
        unsigned key = std::stoul(entry.path().filename().string().substr(0, pos));
        bindingTablesSizes[key] += entry.file_size();
      }
    }
  }
  if (std::filesystem::exists(path_ / "execute_indirect_dump" / "binding_tables")) {
    for (const auto& entry :
         std::filesystem::directory_iterator(path_ / "execute_indirect_dump" / "binding_tables")) {
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
  if (std::filesystem::exists(path_ / "execute_indirect_dump")) {
    for (const auto& entry : std::filesystem::directory_iterator(path_ / "execute_indirect_dump")) {
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

  patchBufferSize_ = std::max({instancesMaxSize, bindingTablesMaxSize, argumentBufferMaxSize});
  constexpr unsigned maxNumOffsetsPerPatchBuffer = 4;
  patchBufferSize_ = align(patchBufferSize_ + maxNumOffsetsPerPatchBuffer *
                                                  D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
                           D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
  LOG_INFO << "CpuPatch - patch buffer size: " << FormatMemorySize(patchBufferSize_);

  loadExecuteIndirectDispatchRays();
}

void PatchService::preBuildRTAS(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.m_pDesc.Value->Inputs.Type != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL ||
      c.m_pDesc.Value->Inputs.NumDescs == 0) {
    return;
  }

  ID3D12GraphicsCommandList* commandList = c.m_Object.Value;
  if (!initialized_) {
    initialize(commandList);
  }

  unsigned patchBufferIndex = getPatchBufferIndex(c.m_Object.Key, c.m_Object.Value);
  c.m_pDesc.Value->Inputs.InstanceDescs = patchBuffers_[patchBufferIndex]->GetGPUVirtualAddress();

  patchInfoByCommandList_[c.m_Object.Key].emplace_back(new PatchInfo());
  auto* patchInfo = patchInfoByCommandList_[c.m_Object.Key].back().get();
  patchInfo->patchBufferIndex = patchBufferIndex;
  patchInfo->type = PatchInfo::Build;
  patchInfo->command.reset(
      new ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand(c));
}

void PatchService::preDispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  ID3D12GraphicsCommandList* commandList = c.m_Object.Value;
  if (!initialized_) {
    initialize(commandList);
  }

  unsigned patchBufferIndex = getPatchBufferIndex(c.m_Object.Key, c.m_Object.Value);
  D3D12_GPU_VIRTUAL_ADDRESS patchBufferAddress =
      patchBuffers_[patchBufferIndex]->GetGPUVirtualAddress();

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
    patchBufferOffset = align(patchBufferOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
  };

  D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc = *c.m_pDesc.Value;

  replaceBindingTable(dispatchRaysDesc.RayGenerationShaderRecord.StartAddress,
                      dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes,
                      dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes);
  replaceBindingTable(dispatchRaysDesc.MissShaderTable.StartAddress,
                      dispatchRaysDesc.MissShaderTable.SizeInBytes,
                      dispatchRaysDesc.MissShaderTable.StrideInBytes);
  replaceBindingTable(dispatchRaysDesc.HitGroupTable.StartAddress,
                      dispatchRaysDesc.HitGroupTable.SizeInBytes,
                      dispatchRaysDesc.HitGroupTable.StrideInBytes);
  replaceBindingTable(dispatchRaysDesc.CallableShaderTable.StartAddress,
                      dispatchRaysDesc.CallableShaderTable.SizeInBytes,
                      dispatchRaysDesc.CallableShaderTable.StrideInBytes);

  patchInfoByCommandList_[c.m_Object.Key].emplace_back(new PatchInfo());
  auto* patchInfo = patchInfoByCommandList_[c.m_Object.Key].back().get();
  patchInfo->patchBufferIndex = patchBufferIndex;
  patchInfo->type = PatchInfo::DispatchRays;
  patchInfo->command.reset(new ID3D12GraphicsCommandList4DispatchRaysCommand(c));
}

void PatchService::preExecuteIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  executeIndirectLastArgumentBufferOffset_ = c.m_ArgumentBufferOffset.Value;

  auto commandSignaturesIt = commandSignatures_.find(c.m_pCommandSignature.Key);
  GITS_ASSERT(commandSignaturesIt != commandSignatures_.end());
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

  ID3D12GraphicsCommandList* commandList = c.m_Object.Value;
  if (!initialized_) {
    initialize(commandList);
  }

  unsigned patchBufferIndex = getPatchBufferIndex(c.m_Object.Key, c.m_Object.Value);

  c.m_pArgumentBuffer.Value = patchBuffers_[patchBufferIndex];
  c.m_ArgumentBufferOffset.Value = 0;

  patchInfoByCommandList_[c.m_Object.Key].emplace_back(new PatchInfo());
  auto* patchInfo = patchInfoByCommandList_[c.m_Object.Key].back().get();
  patchInfo->patchBufferIndex = patchBufferIndex;
  patchInfo->type = PatchInfo::ExecuteIndirect;
  patchInfo->command.reset(new ID3D12GraphicsCommandListExecuteIndirectCommand(c));
}

void PatchService::postExecuteIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  c.m_ArgumentBufferOffset.Value = executeIndirectLastArgumentBufferOffset_;
}

void PatchService::preExecute(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  std::vector<PatchInfo*> patchInfos;
  for (unsigned key : c.m_ppCommandLists.Keys) {
    auto it = patchInfoByCommandList_.find(key);
    if (it != patchInfoByCommandList_.end()) {
      for (auto& itPatchInfo : it->second) {
        patchInfos.push_back(itPatchInfo.get());
      }
    }
  }

  if (patchInfos.empty()) {
    return;
  }

  std::vector<CapturePlayerGpuAddressService::GpuAddressMapping> gpuAddressMappings;
  addressService_.GetMappings(gpuAddressMappings);

  for (auto* patchInfo : patchInfos) {
    if (patchInfo->type == PatchInfo::Build) {
      patchBuild(*patchInfo, gpuAddressMappings);
    } else if (patchInfo->type == PatchInfo::DispatchRays) {
      patchDispatchRays(*patchInfo, gpuAddressMappings);
    } else if (patchInfo->type == PatchInfo::ExecuteIndirect) {
      patchExecuteIndirect(*patchInfo, gpuAddressMappings);
    }
  }
}

void PatchService::postExecute(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  for (unsigned key : c.m_ppCommandLists.Keys) {
    auto itPatchInfo = patchInfoByCommandList_.find(key);
    if (itPatchInfo != patchInfoByCommandList_.end()) {
      patchInfoByCommandList_.erase(itPatchInfo);
    }

    auto itCurrentBuffers = currentPatchBuffersByCommandList_.find(key);
    if (itCurrentBuffers != currentPatchBuffersByCommandList_.end()) {
      for (unsigned patchBufferIndex : itCurrentBuffers->second) {
        HRESULT hr = c.m_Object.Value->Signal(patchBufferFences_[patchBufferIndex].fence,
                                              ++patchBufferFences_[patchBufferIndex].fenceValue);
        GITS_ASSERT(hr == S_OK);
        patchBufferFences_[patchBufferIndex].waitingForExecute = false;
      }
      currentPatchBuffersByCommandList_.erase(itCurrentBuffers);
    }
  }
}

void PatchService::releaseObject(IUnknownReleaseCommand& c) {
  if (c.m_Result.Value == 0) {
    descriptorHandleService_.destroyHeap(c.m_Object.Key);
  }
}

void PatchService::preGetShaderId(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  ShaderIdentifierService::ShaderIdentifier shaderIdentifier;
  memcpy(shaderIdentifier.data(), c.m_Result.Value, shaderIdentifier.size());
  shaderIdentifierService_.addCaptureShaderIdentifier(c.Key, shaderIdentifier,
                                                      c.m_pExportName.Value);
}

void PatchService::postGetShaderId(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  ShaderIdentifierService::ShaderIdentifier shaderIdentifier;
  memcpy(shaderIdentifier.data(), c.m_Result.Value, shaderIdentifier.size());
  shaderIdentifierService_.addPlayerShaderIdentifier(c.Key, shaderIdentifier,
                                                     c.m_pExportName.Value);
}

void PatchService::preGetDescriptorHandle(
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  descriptorHandleService_.addCaptureHandle(c.m_Object.Value, c.m_Object.Key, c.m_Result.Value);
}

void PatchService::postGetDescriptorHandle(
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  descriptorHandleService_.addPlayerHandle(c.m_Object.Key, c.m_Result.Value);
}

void PatchService::createCommandSignature(
    unsigned commandSignatureKey, const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& desc) {
  commandSignatures_[commandSignatureKey].reset(
      new PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>(desc));
}

void PatchService::initialize(ID3D12GraphicsCommandList* commandList) {
  if (initialized_) {
    return;
  }

  for (unsigned i = 0; i < patchBufferInitialPoolSize_; ++i) {
    addPatchBuffer(commandList);
  }

  initialized_ = true;
}

size_t PatchService::align(size_t value, size_t alignment) {
  return ((value + alignment - 1) / alignment) * alignment;
}

void PatchService::addPatchBuffer(ID3D12GraphicsCommandList* commandList) {
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

  resourceDesc.Width = patchBufferSize_;
  patchBuffers_.emplace_back();
  hr = device->CreateCommittedResource(&heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                       D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                       IID_PPV_ARGS(&patchBuffers_[patchBufferPoolSize_]));
  GITS_ASSERT(hr == S_OK);

  patchBufferFences_.emplace_back();
  hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                           IID_PPV_ARGS(&patchBufferFences_[patchBufferPoolSize_].fence));
  GITS_ASSERT(hr == S_OK);

  ++patchBufferPoolSize_;
}

unsigned PatchService::getPatchBufferIndex(unsigned commandListKey,
                                           ID3D12GraphicsCommandList* commandList) {
  for (unsigned i = 0; i < patchBufferPoolSize_; ++i) {
    if (patchBufferFences_[i].waitingForExecute) {
      continue;
    }
    UINT64 value = patchBufferFences_[i].fence->GetCompletedValue();
    if (value == UINT64_MAX) {
      LOG_ERROR << "CpuPatch - getPatchBufferIndex - device removed!";
      exit(EXIT_FAILURE);
    }
    if (value == patchBufferFences_[i].fenceValue) {
      currentPatchBuffersByCommandList_[commandListKey].push_back(i);
      patchBufferFences_[i].waitingForExecute = true;
      return i;
    }
  }

  addPatchBuffer(commandList);
  unsigned newIndex = patchBufferPoolSize_ - 1;
  currentPatchBuffersByCommandList_[commandListKey].push_back(newIndex);
  patchBufferFences_[newIndex].waitingForExecute = true;

  return newIndex;
}

void PatchService::patchBuild(
    const PatchInfo& patchInfo,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  std::filesystem::path path = path_ / "instances_dump" / std::to_string(patchInfo.command->Key);
  std::vector<char> patchedData = readFile(path);
  if (patchedData.empty()) {
    return;
  }

  if (patchedData.size() > patchBufferSize_) {
    LOG_ERROR << "CpuPatch - patch buffer is too small!";
    exit(EXIT_FAILURE);
  }

  auto* command =
      static_cast<ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand*>(
          patchInfo.command.get());
  for (unsigned i = 0; i < command->m_pDesc.Value->Inputs.NumDescs; ++i) {
    D3D12_RAYTRACING_INSTANCE_DESC* instance = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(
        reinterpret_cast<char*>(patchedData.data()) + sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * i);
    if (!instance->AccelerationStructure) {
      continue;
    }
    UINT64 playerAddress = getPlayerAddress(instance->AccelerationStructure, gpuAddressMappings);
    if (playerAddress) {
      instance->AccelerationStructure = playerAddress;
    }
  }

  void* mappedData{};
  patchBuffers_[patchInfo.patchBufferIndex]->Map(0, nullptr, &mappedData);
  memcpy(mappedData, patchedData.data(), patchedData.size());
  patchBuffers_[patchInfo.patchBufferIndex]->Unmap(0, nullptr);
}

void PatchService::patchDispatchRays(
    const PatchInfo& patchInfo,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  auto* command =
      static_cast<ID3D12GraphicsCommandList4DispatchRaysCommand*>(patchInfo.command.get());
  D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc = *command->m_pDesc.Value;

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
        path_ / "binding_table_dump" / (std::to_string(patchInfo.command->Key) + "-" + type);
    std::vector<char> patchedData = readFile(path);
    if (patchedData.empty()) {
      return;
    }

    if (patchedData.size() > patchBufferSize_) {
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
            shaderIdentifierService_.getPlayerIdentifierByCaptureIdentifier(
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
                descriptorHandleService_.getViewDescriptorHeapInfoByCaptureHandle(*address);
            if (heapInfo) {
              UINT64 offset = *address - heapInfo->captureStart;
              *address = heapInfo->playerStart + offset;
              byteOffset += sizeof(UINT64);
              continue;
            }
          }
          {
            DescriptorHandleService::DescriptorHeapInfo* heapInfo =
                descriptorHandleService_.getSamplerDescriptorHeapInfoByCaptureHandle(*address);
            if (heapInfo) {
              UINT64 offset = *address - heapInfo->captureStart;
              *address = heapInfo->playerStart + offset;
              byteOffset += sizeof(UINT64);
              continue;
            }
          }
          {
            UINT64 playerAddress = getPlayerAddress(*address, gpuAddressMappings);
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
    patchBuffers_[patchInfo.patchBufferIndex]->Map(0, nullptr, &mappedData);
    memcpy(static_cast<char*>(mappedData) + patchBufferOffset, patchedData.data(),
           patchedData.size());
    patchBuffers_[patchInfo.patchBufferIndex]->Unmap(0, nullptr);

    patchBufferOffset += stride * count;
    patchBufferOffset = align(patchBufferOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
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

void PatchService::patchExecuteIndirect(
    const PatchInfo& patchInfo,
    const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings) {
  auto* command =
      static_cast<ID3D12GraphicsCommandListExecuteIndirectCommand*>(patchInfo.command.get());

  auto it = commandSignatures_.find(command->m_pCommandSignature.Key);
  GITS_ASSERT(it != commandSignatures_.end());
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
        path_ / "execute_indirect_dump" / "binding_tables" /
        (std::to_string(patchInfo.command->Key) + "-" + std::to_string(index) + "-" + type);
    std::vector<char> patchedData = readFile(path);
    if (patchedData.empty()) {
      return;
    }

    if (patchedData.size() > patchBufferSize_) {
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
            shaderIdentifierService_.getPlayerIdentifierByCaptureIdentifier(
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
                descriptorHandleService_.getViewDescriptorHeapInfoByCaptureHandle(*address);
            if (heapInfo) {
              UINT64 offset = *address - heapInfo->captureStart;
              *address = heapInfo->playerStart + offset;
              byteOffset += sizeof(UINT64);
              continue;
            }
          }
          {
            DescriptorHandleService::DescriptorHeapInfo* heapInfo =
                descriptorHandleService_.getSamplerDescriptorHeapInfoByCaptureHandle(*address);
            if (heapInfo) {
              UINT64 offset = *address - heapInfo->captureStart;
              *address = heapInfo->playerStart + offset;
              byteOffset += sizeof(UINT64);
              continue;
            }
          }
          {
            UINT64 playerAddress = getPlayerAddress(*address, gpuAddressMappings);
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
    patchBuffers_[raytracingPatchBuffer]->Map(0, nullptr, &mappedData);
    memcpy(static_cast<char*>(mappedData) + raytracingPatchBufferOffset, patchedData.data(),
           patchedData.size());
    patchBuffers_[raytracingPatchBuffer]->Unmap(0, nullptr);

    raytracingPatchMapping[startAddress] =
        patchBuffers_[raytracingPatchBuffer]->GetGPUVirtualAddress() + raytracingPatchBufferOffset;

    raytracingPatchBufferOffset += stride * count;
    raytracingPatchBufferOffset =
        align(raytracingPatchBufferOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
  };

  auto dispatchRaysIt = executeIndirectDispatchRays_.find(command->Key);
  if (dispatchRaysIt != executeIndirectDispatchRays_.end()) {
    for (unsigned i = 0; i < dispatchRaysIt->second.size(); ++i) {
      const D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc = dispatchRaysIt->second.at(i);

      raytracingPatchBuffer = getPatchBufferIndex(command->m_Object.Key, command->m_Object.Value);
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
    std::filesystem::path path = path_ / "execute_indirect_dump" /
                                 (std::to_string(patchInfo.command->Key) + "-ArgumentBuffer");
    std::vector<char> patchedData = readFile(path);
    if (patchedData.empty()) {
      return;
    }

    if (patchedData.size() > patchBufferSize_) {
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
          UINT64 playerAddress = getPlayerAddress(*address, gpuAddressMappings);
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
    patchBuffers_[patchInfo.patchBufferIndex]->Map(0, nullptr, &mappedData);
    memcpy(mappedData, patchedData.data(), patchedData.size());
    patchBuffers_[patchInfo.patchBufferIndex]->Unmap(0, nullptr);
  }
}

std::vector<char> PatchService::readFile(std::filesystem::path path) {
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

UINT64 PatchService::getPlayerAddress(
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

void PatchService::loadExecuteIndirectDispatchRays() {
  std::filesystem::path dumpPath = gitsConfig_.common.player.streamDir;
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

    executeIndirectDispatchRays_[callKey].push_back(desc);
  }
}

} // namespace DirectX
} // namespace gits
