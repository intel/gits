// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuPatchLayer.h"
#include "playerManager.h"
#include "gits.h"
#include "log.h"

#include <fstream>

namespace gits {
namespace DirectX {

GpuPatchLayer::GpuPatchLayer(PlayerManager& manager)
    : Layer("RaytracingGpuPatch"),
      manager_(manager),
      dumpService_(addressService_, shaderIdentifierService_, descriptorHandleService_) {
  useAddressPinning_ = Configurator::Get().directx.player.addressPinning == AddressPinningMode::USE;
  loadExecuteIndirectDispatchRays();
  if (!useAddressPinning_) {
    loadInstancesArraysOfPointers();
  }
}

GpuPatchLayer::~GpuPatchLayer() {
  for (auto& it : commandSignatures_) {
    delete[] it.second.pArgumentDescs;
  }
}

void GpuPatchLayer::pre(IUnknownReleaseCommand& c) {
  if (c.result_.value == 0) {
    addressService_.destroyInterface(c.object_.key);
    descriptorHandleService_.destroyHeap(c.object_.key);
    commandListService_.remove(c.object_.key);
    genericReadResources_.erase(c.object_.key);
  }
}

void GpuPatchLayer::pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  D3D12_RESOURCE_DESC desc = c.object_.value->GetDesc();
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    addressService_.addGpuCaptureAddress(c.object_.value, c.object_.key, desc.Width,
                                         c.result_.value);
  }
  resourceByKey_[c.object_.key] = c.object_.value;
}

void GpuPatchLayer::post(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  D3D12_RESOURCE_DESC desc = c.object_.value->GetDesc();
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    addressService_.addGpuPlayerAddress(c.object_.value, c.object_.key, desc.Width,
                                        c.result_.value);
  }
}

void GpuPatchLayer::pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  CapturePlayerShaderIdentifierService::ShaderIdentifier shaderIdentifier;
  memcpy(shaderIdentifier.data(), c.result_.value, shaderIdentifier.size());
  shaderIdentifierService_.addCaptureShaderIdentifier(c.key, shaderIdentifier,
                                                      c.pExportName_.value);
}

void GpuPatchLayer::post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  CapturePlayerShaderIdentifierService::ShaderIdentifier shaderIdentifier;
  memcpy(shaderIdentifier.data(), c.result_.value, shaderIdentifier.size());
  shaderIdentifierService_.addPlayerShaderIdentifier(c.key, shaderIdentifier, c.pExportName_.value);
}

void GpuPatchLayer::pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  descriptorHandleService_.addCaptureHandle(c.object_.value, c.object_.key, c.result_.value);
}

void GpuPatchLayer::post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  descriptorHandleService_.addPlayerHandle(c.object_.key, c.result_.value);
}

void GpuPatchLayer::pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (useAddressPinning_) {
    return;
  }

  if (c.pDesc_.value->Inputs.Type != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL ||
      c.pDesc_.value->Inputs.NumDescs == 0) {
    return;
  }

  ID3D12GraphicsCommandList* commandList = c.object_.value;
  if (!initialized_) {
    initialize(commandList);
  }

  auto& msgBus = CGits::Instance().GetMessageBus();
  msgBus.publish(
      {PUBLISHER_PLAYER, TOPIC_GITS_WORKLOAD_BEGIN},
      std::make_shared<GitsWorkloadMessage>(
          commandList, "GITS_BuildRaytracingAccelerationStructure-Patch", c.object_.key));

  unsigned mappingBufferIndex = getMappingBufferIndex(c.object_.key);
  commandList->CopyResource(gpuAddressBuffers_[mappingBufferIndex],
                            gpuAddressStagingBuffers_[mappingBufferIndex]);
  commandList->CopyResource(mappingCountBuffers_[mappingBufferIndex],
                            mappingCountStagingBuffers_[mappingBufferIndex]);

  unsigned instanceDescsKey = c.pDesc_.inputKeys[0];
  ID3D12Resource* instanceDescs = resourceByKey_[instanceDescsKey];
  GITS_ASSERT(instanceDescs);

  if (c.pDesc_.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {

    unsigned offset = c.pDesc_.inputOffsets[0];
    unsigned size = c.pDesc_.value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
    if (size > patchBufferSize_) {
      LOG_ERROR << "Raytracing patch buffer is too small for instances!";
      exit(EXIT_FAILURE);
    }

    unsigned patchBufferIndex = getPatchBufferIndex(c.object_.key, c.object_.value);

    {
      D3D12_RESOURCE_BARRIER barrier{};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = patchBuffers_[patchBufferIndex];
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
      commandList->ResourceBarrier(1, &barrier);

      // workaround for improper mapping gpu address to buffer in capture
      D3D12_RESOURCE_DESC desc = instanceDescs->GetDesc();
      bool denyShaderResource = desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
      bool setSourceBarrier =
          genericReadResources_.find(instanceDescsKey) == genericReadResources_.end() &&
          !denyShaderResource;
      if (setSourceBarrier) {
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = instanceDescs;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        commandList->ResourceBarrier(1, &barrier);
      }

      commandList->CopyBufferRegion(patchBuffers_[patchBufferIndex], 0, instanceDescs, offset,
                                    size);

      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = patchBuffers_[patchBufferIndex];
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      commandList->ResourceBarrier(1, &barrier);

      if (setSourceBarrier) {
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = instanceDescs;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        commandList->ResourceBarrier(1, &barrier);
      }
    }

    D3D12_GPU_VIRTUAL_ADDRESS patchBufferAddress =
        patchBuffers_[patchBufferIndex]->GetGPUVirtualAddress();
    c.pDesc_.value->Inputs.InstanceDescs = patchBufferAddress;

    dumpService_.dumpInstances(commandList, patchBuffers_[patchBufferIndex], instanceDescsKey, size,
                               c.key, true);

    raytracingShaderPatchService_.patchInstances(
        commandList, patchBufferAddress, c.pDesc_.value->Inputs.NumDescs,
        gpuAddressBuffers_[mappingBufferIndex]->GetGPUVirtualAddress(),
        mappingCountBuffers_[mappingBufferIndex]->GetGPUVirtualAddress());

    dumpService_.dumpInstances(commandList, patchBuffers_[patchBufferIndex], instanceDescsKey, size,
                               c.key, false);

    {
      D3D12_RESOURCE_BARRIER barriers[2]{};
      barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      barriers[0].UAV.pResource = nullptr;

      barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barriers[1].Transition.pResource = patchBuffers_[patchBufferIndex];
      barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

      commandList->ResourceBarrier(2, barriers);
    }

  } else if (c.pDesc_.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {

    if (!initializedInstancesAoP_) {
      initializeInstancesAoP(commandList);
    }

    auto itInstances = instancesArraysOfPointers_.find(c.key);
    GITS_ASSERT(itInstances != instancesArraysOfPointers_.end())
    std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& arrayOfPointers = itInstances->second;

    struct InstanceInfo {
      ID3D12Resource* resource{};
      unsigned patchBufferIndex{};
      unsigned resourceKey{};
      D3D12_GPU_VIRTUAL_ADDRESS captureStart;
      std::set<unsigned> offsets;
    };
    std::unordered_map<unsigned, InstanceInfo> instancesByResourceKey;

    std::vector<InstanceInfo*> instanceInfos(arrayOfPointers.size());
    for (unsigned i = 0; i < arrayOfPointers.size(); ++i) {
      CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
          addressService_.getResourceInfoByCaptureAddress(arrayOfPointers[i]);
      GITS_ASSERT(resourceInfo);
      auto it = instancesByResourceKey.find(resourceInfo->key);
      if (it == instancesByResourceKey.end()) {
        instanceInfos[i] = &instancesByResourceKey[resourceInfo->key];
        instanceInfos[i]->resource = resourceInfo->resource;
        instanceInfos[i]->resourceKey = resourceInfo->key;
        instanceInfos[i]->captureStart = resourceInfo->captureStart;
        instanceInfos[i]->patchBufferIndex = getInstancesAoPPatchBufferIndex(c.object_.key);
      } else {
        instanceInfos[i] = &it->second;
      }
      unsigned offset = arrayOfPointers[i] - resourceInfo->captureStart;
      instanceInfos[i]->offsets.insert(offset);
    }

    std::vector<D3D12_GPU_VIRTUAL_ADDRESS> patchedArrayOfPointers(arrayOfPointers.size());
    for (unsigned i = 0; i < arrayOfPointers.size(); ++i) {
      unsigned offset =
          arrayOfPointers[i] - instanceInfos[i]->captureStart - *instanceInfos[i]->offsets.begin();
      patchedArrayOfPointers[i] =
          instancesAoPPatchBuffers_[instanceInfos[i]->patchBufferIndex]->GetGPUVirtualAddress() +
          offset;
    }

    unsigned size = c.pDesc_.value->Inputs.NumDescs * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
    if (size > instancesAoPStagingBufferSize_) {
      LOG_ERROR << "Raytracing staging buffer is too small for array of pointers to instances!";
      exit(EXIT_FAILURE);
    }

    unsigned instancesAoPBufferIndex = getInstancesAoPStagingBufferIndex(c.object_.key);
    void* data{};
    HRESULT hr = instancesAoPStagingBuffers_[instancesAoPBufferIndex]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, patchedArrayOfPointers.data(),
           sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * patchedArrayOfPointers.size());
    instancesAoPStagingBuffers_[instancesAoPBufferIndex]->Unmap(0, nullptr);

    {
      D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      if (genericReadResources_.find(instanceDescsKey) != genericReadResources_.end()) {
        resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
      }
      unsigned offset = c.pDesc_.inputOffsets[0];
      dumpService_.dumpInstancesArrayOfPointers(commandList, instanceDescs, instanceDescsKey,
                                                offset, size, resourceState, c.key, true);
    }

    unsigned patchBufferIndex = getPatchBufferIndex(c.object_.key, c.object_.value);

    D3D12_GPU_VIRTUAL_ADDRESS patchBufferAddress =
        patchBuffers_[patchBufferIndex]->GetGPUVirtualAddress();
    c.pDesc_.value->Inputs.InstanceDescs = patchBufferAddress;

    {
      D3D12_RESOURCE_BARRIER barrier{};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = patchBuffers_[patchBufferIndex];
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
      commandList->ResourceBarrier(1, &barrier);

      commandList->CopyBufferRegion(patchBuffers_[patchBufferIndex], 0,
                                    instancesAoPStagingBuffers_[instancesAoPBufferIndex], 0, size);

      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = patchBuffers_[patchBufferIndex];
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      commandList->ResourceBarrier(1, &barrier);
    }

    dumpService_.dumpInstancesArrayOfPointers(
        commandList, patchBuffers_[patchBufferIndex], instanceDescsKey, 0, size,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, c.key, false);

    for (auto& it : instancesByResourceKey) {
      InstanceInfo& instanceInfo = it.second;
      unsigned patchBufferIndex = instanceInfo.patchBufferIndex;

      std::vector<unsigned> offsets(instanceInfo.offsets.size());
      std::copy(instanceInfo.offsets.begin(), instanceInfo.offsets.end(), offsets.begin());

      unsigned size = offsets.back() + sizeof(D3D12_RAYTRACING_INSTANCE_DESC) - offsets[0];
      if (size > instancesAoPPatchBufferSize_) {
        LOG_ERROR
            << "Raytracing patch buffer is too small for instances pointed by array of pointers!";
        exit(EXIT_FAILURE);
      }

      unsigned startOffset = offsets[0];
      for (unsigned i = 0; i < offsets.size(); ++i) {
        offsets[i] -= startOffset;
        offsets[i] /= sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
      }

      auto offsetsByteSize = sizeof(unsigned) * offsets.size();
      if (offsetsByteSize > instancesAoPPatchOffsetsBufferSize_) {
        LOG_ERROR << "Raytracing array of pointers offsets buffer is too small!";
        exit(EXIT_FAILURE);
      }

      void* data{};
      HRESULT hr =
          instancesAoPPatchOffsetsStagingBuffers_[patchBufferIndex]->Map(0, nullptr, &data);
      GITS_ASSERT(hr == S_OK);
      memcpy(data, offsets.data(), offsetsByteSize);
      instancesAoPPatchOffsetsStagingBuffers_[patchBufferIndex]->Unmap(0, nullptr);

      commandList->CopyResource(instancesAoPPatchOffsetsBuffers_[patchBufferIndex],
                                instancesAoPPatchOffsetsStagingBuffers_[patchBufferIndex]);

      {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = instancesAoPPatchBuffers_[patchBufferIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        commandList->ResourceBarrier(1, &barrier);

        // workaround for improper mapping gpu address to buffer in capture
        D3D12_RESOURCE_DESC desc = instanceInfo.resource->GetDesc();
        bool denyShaderResource = desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        bool setSourceBarrier =
            genericReadResources_.find(instanceInfo.resourceKey) == genericReadResources_.end() &&
            !denyShaderResource;
        if (setSourceBarrier) {
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Transition.pResource = instanceInfo.resource;
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
          barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
          commandList->ResourceBarrier(1, &barrier);
        }

        commandList->CopyBufferRegion(instancesAoPPatchBuffers_[patchBufferIndex], 0,
                                      instanceInfo.resource, startOffset, size);

        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = instancesAoPPatchBuffers_[patchBufferIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        commandList->ResourceBarrier(1, &barrier);

        if (setSourceBarrier) {
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Transition.pResource = instanceInfo.resource;
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
          barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
          commandList->ResourceBarrier(1, &barrier);
        }
      }

      D3D12_GPU_VIRTUAL_ADDRESS patchBufferAddress =
          instancesAoPPatchBuffers_[patchBufferIndex]->GetGPUVirtualAddress();
      D3D12_GPU_VIRTUAL_ADDRESS patchOffsetsBufferAddress =
          instancesAoPPatchOffsetsBuffers_[patchBufferIndex]->GetGPUVirtualAddress();

      dumpService_.dumpInstances(commandList, instancesAoPPatchBuffers_[patchBufferIndex],
                                 instanceInfo.resourceKey, size, c.key, true);

      raytracingShaderPatchService_.patchInstancesOffset(
          commandList, patchBufferAddress, patchOffsetsBufferAddress, offsets.size(),
          gpuAddressBuffers_[mappingBufferIndex]->GetGPUVirtualAddress(),
          mappingCountBuffers_[mappingBufferIndex]->GetGPUVirtualAddress());

      dumpService_.dumpInstances(commandList, instancesAoPPatchBuffers_[patchBufferIndex],
                                 instanceInfo.resourceKey, size, c.key, false);

      {
        D3D12_RESOURCE_BARRIER barriers[2]{};
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[0].UAV.pResource = nullptr;

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Transition.pResource = instancesAoPPatchBuffers_[patchBufferIndex];
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        commandList->ResourceBarrier(2, barriers);
      }
    }
  }

  commandListService_.restoreState(c.object_.key, c.object_.value);

  msgBus.publish(
      {PUBLISHER_PLAYER, TOPIC_GITS_WORKLOAD_END},
      std::make_shared<GitsWorkloadMessage>(
          commandList, "GITS_BuildRaytracingAccelerationStructure-Patch", c.object_.key));
}

void GpuPatchLayer::pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  ID3D12GraphicsCommandList* commandList = c.object_.value;
  if (!initialized_) {
    initialize(commandList);
  }

  auto& msgBus = CGits::Instance().GetMessageBus();
  msgBus.publish(
      {PUBLISHER_PLAYER, TOPIC_GITS_WORKLOAD_BEGIN},
      std::make_shared<GitsWorkloadMessage>(commandList, "GITS_DispatchRays-Patch", c.object_.key));

  unsigned patchBufferIndex = getPatchBufferIndex(c.object_.key, c.object_.value);

  unsigned mappingBufferIndex = getMappingBufferIndex(c.object_.key);
  if (!useAddressPinning_) {
    commandList->CopyResource(gpuAddressBuffers_[mappingBufferIndex],
                              gpuAddressStagingBuffers_[mappingBufferIndex]);
  }
  commandList->CopyResource(shaderIdentifierBuffers_[mappingBufferIndex],
                            shaderIdentifierStagingBuffers_[mappingBufferIndex]);
  commandList->CopyResource(viewDescriptorBuffers_[mappingBufferIndex],
                            viewDescriptorStagingBuffers_[mappingBufferIndex]);
  commandList->CopyResource(sampleDescriptorBuffers_[mappingBufferIndex],
                            sampleDescriptorStagingBuffers_[mappingBufferIndex]);
  commandList->CopyResource(mappingCountBuffers_[mappingBufferIndex],
                            mappingCountStagingBuffers_[mappingBufferIndex]);

  patchDispatchRays(c.object_.value, *c.pDesc_.value, patchBufferIndex, mappingBufferIndex, c.key);

  commandListService_.restoreState(c.object_.key, c.object_.value);

  msgBus.publish(
      {PUBLISHER_PLAYER, TOPIC_GITS_WORKLOAD_END},
      std::make_shared<GitsWorkloadMessage>(commandList, "GITS_DispatchRays-Patch", c.object_.key));
}

void GpuPatchLayer::patchDispatchRays(ID3D12GraphicsCommandList* commandList,
                                      D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc,
                                      unsigned patchBufferIndex,
                                      unsigned mappingBufferIndex,
                                      unsigned callKey) {

  D3D12_GPU_VIRTUAL_ADDRESS patchBufferAddress =
      patchBuffers_[patchBufferIndex]->GetGPUVirtualAddress();
  unsigned patchBufferOffset = 0;

  auto patchBindingTable = [&](D3D12_GPU_VIRTUAL_ADDRESS& startAddress, UINT64 sizeInBytes,
                               UINT64 strideInBytes,
                               GpuPatchDumpService::BindingTableType bindingTableType) {
    if (startAddress) {
      CapturePlayerGpuAddressService::ResourceInfo* info =
          addressService_.getResourceInfoByCaptureAddress(startAddress);
      GITS_ASSERT(info);
      unsigned offset = startAddress - info->captureStart;
      if (strideInBytes == 0) {
        strideInBytes = sizeInBytes;
      }
      unsigned stride = strideInBytes;
      unsigned count = sizeInBytes / strideInBytes;

      if (stride * count > patchBufferSize_) {
        LOG_ERROR << "Raytracing patch buffer is too small for binding table!";
        exit(EXIT_FAILURE);
      }

      {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = patchBuffers_[patchBufferIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        commandList->ResourceBarrier(1, &barrier);

        // workaround for improper mapping gpu address to buffer in capture
        D3D12_RESOURCE_DESC desc = info->resource->GetDesc();
        bool denyShaderResource = desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        bool setSourceBarrier =
            genericReadResources_.find(info->key) == genericReadResources_.end() &&
            !denyShaderResource;
        if (setSourceBarrier) {
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Transition.pResource = info->resource;
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
          barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
          commandList->ResourceBarrier(1, &barrier);
        }

        commandList->CopyBufferRegion(patchBuffers_[patchBufferIndex], patchBufferOffset,
                                      info->resource, offset, stride * count);

        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = patchBuffers_[patchBufferIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        commandList->ResourceBarrier(1, &barrier);

        if (setSourceBarrier) {
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Transition.pResource = info->resource;
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
          barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
          commandList->ResourceBarrier(1, &barrier);
        }
      }

      startAddress = patchBufferAddress + patchBufferOffset;

      dumpService_.dumpBindingTable(commandList, patchBuffers_[patchBufferIndex], patchBufferOffset,
                                    sizeInBytes, stride, callKey, bindingTableType, true);

      raytracingShaderPatchService_.patchBindingTable(
          commandList, patchBufferAddress + patchBufferOffset, count, stride,
          useAddressPinning_ ? 0 : gpuAddressBuffers_[mappingBufferIndex]->GetGPUVirtualAddress(),
          shaderIdentifierBuffers_[mappingBufferIndex]->GetGPUVirtualAddress(),
          viewDescriptorBuffers_[mappingBufferIndex]->GetGPUVirtualAddress(),
          sampleDescriptorBuffers_[mappingBufferIndex]->GetGPUVirtualAddress(),
          mappingCountBuffers_[mappingBufferIndex]->GetGPUVirtualAddress(), !useAddressPinning_);

      dumpService_.dumpBindingTable(commandList, patchBuffers_[patchBufferIndex], patchBufferOffset,
                                    sizeInBytes, stride, callKey, bindingTableType, false);

      patchBufferOffset += stride * count;
      patchBufferOffset =
          patchBufferOffset % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT == 0
              ? patchBufferOffset
              : ((patchBufferOffset / D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) + 1) *
                    D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    }
  };

  patchBindingTable(dispatchRaysDesc.RayGenerationShaderRecord.StartAddress,
                    dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes,
                    dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes,
                    GpuPatchDumpService::RayGeneration);
  patchBindingTable(dispatchRaysDesc.MissShaderTable.StartAddress,
                    dispatchRaysDesc.MissShaderTable.SizeInBytes,
                    dispatchRaysDesc.MissShaderTable.StrideInBytes, GpuPatchDumpService::Miss);
  patchBindingTable(dispatchRaysDesc.HitGroupTable.StartAddress,
                    dispatchRaysDesc.HitGroupTable.SizeInBytes,
                    dispatchRaysDesc.HitGroupTable.StrideInBytes, GpuPatchDumpService::HitGroup);
  patchBindingTable(dispatchRaysDesc.CallableShaderTable.StartAddress,
                    dispatchRaysDesc.CallableShaderTable.SizeInBytes,
                    dispatchRaysDesc.CallableShaderTable.StrideInBytes,
                    GpuPatchDumpService::Callable);

  if (patchBufferOffset > patchBufferSize_) {
    LOG_ERROR << "Raytracing patch buffer is too small for binding tables!";
    exit(EXIT_FAILURE);
  }

  {
    D3D12_RESOURCE_BARRIER barriers[2]{};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[0].UAV.pResource = nullptr;

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = patchBuffers_[patchBufferIndex];
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    commandList->ResourceBarrier(2, barriers);
  }
}

void GpuPatchLayer::post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  commandListService_.storeCommand(c);
}

void GpuPatchLayer::post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  commandListService_.storeCommand(c);
}

void GpuPatchLayer::post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) {
  commandListService_.storeCommand(c);
}

void GpuPatchLayer::post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c) {
  commandListService_.storeCommand(c);
}

void GpuPatchLayer::post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  commandListService_.storeCommand(c);
}

void GpuPatchLayer::post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  commandListService_.storeCommand(c);
}

void GpuPatchLayer::post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  commandListService_.storeCommand(c);
}

void GpuPatchLayer::post(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  commandListService_.storeCommand(c);
}

void GpuPatchLayer::post(ID3D12DeviceCreateCommandListCommand& c) {
  commandListService_.storeCommand(c);
}

void GpuPatchLayer::post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  commandListService_.storeCommand(c);
}

void GpuPatchLayer::post(ID3D12GraphicsCommandListResetCommand& c) {
  commandListService_.reset(c.object_.key, c.pInitialState_.value);
}

void GpuPatchLayer::initialize(ID3D12GraphicsCommandList* commandList) {
  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);

  D3D12_HEAP_PROPERTIES heapPropertiesDefault{};
  heapPropertiesDefault.Type = D3D12_HEAP_TYPE_DEFAULT;
  heapPropertiesDefault.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapPropertiesDefault.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapPropertiesDefault.CreationNodeMask = 1;
  heapPropertiesDefault.VisibleNodeMask = 1;

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

  if (!useAddressPinning_) {
    for (unsigned i = 0; i < mappingBufferPoolSize_; ++i) {
      resourceDesc.Width = gpuAddressBufferSize_;
      resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
      hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE,
                                           &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                           IID_PPV_ARGS(&gpuAddressBuffers_[i]));
      GITS_ASSERT(hr == S_OK);

      resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
      hr = device->CreateCommittedResource(&heapPropertiesUpload, D3D12_HEAP_FLAG_NONE,
                                           &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                           IID_PPV_ARGS(&gpuAddressStagingBuffers_[i]));
      GITS_ASSERT(hr == S_OK);
    }
  }

  for (unsigned i = 0; i < mappingBufferPoolSize_; ++i) {
    resourceDesc.Width = shaderIdentifierBufferSize_;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE,
                                         &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&shaderIdentifierBuffers_[i]));
    GITS_ASSERT(hr == S_OK);

    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    hr = device->CreateCommittedResource(&heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&shaderIdentifierStagingBuffers_[i]));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < mappingBufferPoolSize_; ++i) {
    resourceDesc.Width = viewDescriptorBufferSize_;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE,
                                         &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&viewDescriptorBuffers_[i]));
    GITS_ASSERT(hr == S_OK);

    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    hr = device->CreateCommittedResource(&heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&viewDescriptorStagingBuffers_[i]));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < mappingBufferPoolSize_; ++i) {
    resourceDesc.Width = sampleDescriptorBufferSize_;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE,
                                         &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&sampleDescriptorBuffers_[i]));
    GITS_ASSERT(hr == S_OK);

    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    hr = device->CreateCommittedResource(&heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&sampleDescriptorStagingBuffers_[i]));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < mappingBufferPoolSize_; ++i) {
    resourceDesc.Width = mappingCountBufferSize_;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE,
                                         &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&mappingCountBuffers_[i]));
    GITS_ASSERT(hr == S_OK);

    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    hr = device->CreateCommittedResource(&heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&mappingCountStagingBuffers_[i]));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < mappingBufferPoolSize_; ++i) {
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mappingFences_[i].fence));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < patchBufferInitialPoolSize_; ++i) {
    addPatchBuffer(commandList);
  }

  initialized_ = true;
}

void GpuPatchLayer::initializeInstancesAoP(ID3D12GraphicsCommandList* commandList) {
  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);

  D3D12_HEAP_PROPERTIES heapPropertiesDefault{};
  heapPropertiesDefault.Type = D3D12_HEAP_TYPE_DEFAULT;
  heapPropertiesDefault.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapPropertiesDefault.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapPropertiesDefault.CreationNodeMask = 1;
  heapPropertiesDefault.VisibleNodeMask = 1;

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

  for (unsigned i = 0; i < instancesAoPPatchBuffers_.size(); ++i) {
    resourceDesc.Width = instancesAoPPatchBufferSize_;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE,
                                         &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&instancesAoPPatchBuffers_[i]));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < instancesAoPStagingBuffers_.size(); ++i) {
    resourceDesc.Width = instancesAoPStagingBufferSize_;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    hr = device->CreateCommittedResource(&heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&instancesAoPStagingBuffers_[i]));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < instancesAoPPatchOffsetsBuffers_.size(); ++i) {
    resourceDesc.Width = instancesAoPPatchOffsetsBufferSize_;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE,
                                         &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&instancesAoPPatchOffsetsBuffers_[i]));
    GITS_ASSERT(hr == S_OK);

    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    hr = device->CreateCommittedResource(&heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&instancesAoPPatchOffsetsStagingBuffers_[i]));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < instancesAoPPatchBufferPoolSize_; ++i) {
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                             IID_PPV_ARGS(&instancesAoPPatchBufferFences_[i].fence));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < instancesAoPStagingBufferPoolSize_; ++i) {
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                             IID_PPV_ARGS(&instancesAoPStagingBufferFences_[i].fence));
    GITS_ASSERT(hr == S_OK);
  }

  initializedInstancesAoP_ = true;
}

void GpuPatchLayer::addPatchBuffer(ID3D12GraphicsCommandList* commandList) {
  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);

  D3D12_HEAP_PROPERTIES heapPropertiesDefault{};
  heapPropertiesDefault.Type = D3D12_HEAP_TYPE_DEFAULT;
  heapPropertiesDefault.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapPropertiesDefault.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapPropertiesDefault.CreationNodeMask = 1;
  heapPropertiesDefault.VisibleNodeMask = 1;

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

  resourceDesc.Width = patchBufferSize_;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  patchBuffers_.emplace_back();
  hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr,
                                       IID_PPV_ARGS(&patchBuffers_[patchBufferPoolSize_]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Width = patchOffsetsBufferSize_;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  patchOffsetsBuffers_.emplace_back();
  hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr,
                                       IID_PPV_ARGS(&patchOffsetsBuffers_[patchBufferPoolSize_]));
  GITS_ASSERT(hr == S_OK);

  patchOffsetsStagingBuffers_.emplace_back();
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  hr = device->CreateCommittedResource(
      &heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&patchOffsetsStagingBuffers_[patchBufferPoolSize_]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Width = executeIndirectRaytracingPatchBufferSize_;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  executeIndirectRaytracingPatchBuffers_.emplace_back();
  hr = device->CreateCommittedResource(
      &heapPropertiesDefault, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&executeIndirectRaytracingPatchBuffers_[patchBufferPoolSize_]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  executeIndirectRaytracingPatchStagingBuffers_.emplace_back();
  hr = device->CreateCommittedResource(
      &heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&executeIndirectRaytracingPatchStagingBuffers_[patchBufferPoolSize_]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Width = executeIndirectCountBufferSize_;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  executeIndirectCountBuffers_.emplace_back();
  hr = device->CreateCommittedResource(
      &heapPropertiesDefault, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&executeIndirectCountBuffers_[patchBufferPoolSize_]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  executeIndirectCountStagingBuffers_.emplace_back();
  hr = device->CreateCommittedResource(
      &heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&executeIndirectCountStagingBuffers_[patchBufferPoolSize_]));
  GITS_ASSERT(hr == S_OK);

  patchBufferFences_.emplace_back();
  hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                           IID_PPV_ARGS(&patchBufferFences_[patchBufferPoolSize_].fence));
  GITS_ASSERT(hr == S_OK);

  ++patchBufferPoolSize_;
}

void GpuPatchLayer::pre(ID3D12CommandQueueExecuteCommandListsCommand& c) {

  std::vector<unsigned> mappingBuffers;
  for (unsigned key : c.ppCommandLists_.keys) {
    auto it = currentMappingsByCommandList_.find(key);
    if (it != currentMappingsByCommandList_.end()) {
      mappingBuffers.push_back(it->second);
    }
  }

  if (mappingBuffers.empty()) {
    return;
  }

  MappingCount mappingCount{};

  if (!useAddressPinning_) {
    std::vector<CapturePlayerGpuAddressService::GpuAddressMapping> gpuAddressMappings;
    addressService_.getMappings(gpuAddressMappings);
    mappingCount.gpuAddressCount = gpuAddressMappings.size();

    if (gpuAddressMappings.size() >
        gpuAddressBufferSize_ / sizeof(CapturePlayerGpuAddressService::GpuAddressMapping)) {
      LOG_ERROR << "Raytracing gpuAddressMappings buffer is too small!";
      exit(EXIT_FAILURE);
    }
    for (unsigned index : mappingBuffers) {
      void* data{};
      HRESULT hr = gpuAddressStagingBuffers_[index]->Map(0, nullptr, &data);
      GITS_ASSERT(hr == S_OK);
      memcpy(data, gpuAddressMappings.data(),
             sizeof(CapturePlayerGpuAddressService::GpuAddressMapping) * gpuAddressMappings.size());
      gpuAddressStagingBuffers_[index]->Unmap(0, nullptr);
    }
  }

  std::vector<CapturePlayerShaderIdentifierService::ShaderIdentifierMapping>
      shaderIdentifierMappings;
  shaderIdentifierService_.getMappings(shaderIdentifierMappings);
  mappingCount.shaderIdentiferCount = shaderIdentifierMappings.size();
  if (shaderIdentifierMappings.size() >
      shaderIdentifierBufferSize_ /
          sizeof(CapturePlayerShaderIdentifierService::ShaderIdentifierMapping)) {
    LOG_ERROR << "Raytracing shaderIdentifierMappings buffer is too small!";
    exit(EXIT_FAILURE);
  }
  for (unsigned index : mappingBuffers) {
    void* data{};
    HRESULT hr = shaderIdentifierStagingBuffers_[index]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, shaderIdentifierMappings.data(),
           sizeof(CapturePlayerShaderIdentifierService::ShaderIdentifierMapping) *
               shaderIdentifierMappings.size());
    shaderIdentifierStagingBuffers_[index]->Unmap(0, nullptr);
  }

  std::vector<CapturePlayerDescriptorHandleService::DescriptorMapping> viewDescriptorMappings;
  descriptorHandleService_.getViewMappings(viewDescriptorMappings);
  mappingCount.viewDescriptorCount = viewDescriptorMappings.size();
  if (viewDescriptorMappings.size() >
      viewDescriptorBufferSize_ / sizeof(CapturePlayerDescriptorHandleService::DescriptorMapping)) {
    LOG_ERROR << "Raytracing viewDescriptorMappings buffer is too small!";
    exit(EXIT_FAILURE);
  }
  for (unsigned index : mappingBuffers) {
    void* data{};
    HRESULT hr = viewDescriptorStagingBuffers_[index]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, viewDescriptorMappings.data(),
           sizeof(CapturePlayerDescriptorHandleService::DescriptorMapping) *
               viewDescriptorMappings.size());
    viewDescriptorStagingBuffers_[index]->Unmap(0, nullptr);
  }

  std::vector<CapturePlayerDescriptorHandleService::DescriptorMapping> sampleDescriptorMappings;
  descriptorHandleService_.getSamplerMappings(sampleDescriptorMappings);
  mappingCount.sampleDescriptorCount = sampleDescriptorMappings.size();
  if (sampleDescriptorMappings.size() >
      sampleDescriptorBufferSize_ /
          sizeof(CapturePlayerDescriptorHandleService::DescriptorMapping)) {
    LOG_ERROR << "Raytracing sampleDescriptorMappings buffer is too small!";
    exit(EXIT_FAILURE);
  }
  for (unsigned index : mappingBuffers) {
    void* data{};
    HRESULT hr = sampleDescriptorStagingBuffers_[index]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, sampleDescriptorMappings.data(),
           sizeof(CapturePlayerDescriptorHandleService::DescriptorMapping) *
               sampleDescriptorMappings.size());
    sampleDescriptorStagingBuffers_[index]->Unmap(0, nullptr);
  }

  for (unsigned index : mappingBuffers) {
    void* data{};
    HRESULT hr = mappingCountStagingBuffers_[index]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, &mappingCount, sizeof(MappingCount));
    mappingCountStagingBuffers_[index]->Unmap(0, nullptr);
  }
}

void GpuPatchLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {

  for (unsigned key : c.ppCommandLists_.keys) {
    auto itMappings = currentMappingsByCommandList_.find(key);
    if (itMappings != currentMappingsByCommandList_.end()) {
      unsigned mappingBufferIndex = itMappings->second;
      HRESULT hr = c.object_.value->Signal(mappingFences_[mappingBufferIndex].fence,
                                           ++mappingFences_[mappingBufferIndex].fenceValue);
      GITS_ASSERT(hr == S_OK);
      mappingFences_[mappingBufferIndex].waitingForExecute = false;
      currentMappingsByCommandList_.erase(itMappings);
    }

    auto itCurrentBuffers = currentPatchBuffersByCommandList_.find(key);
    if (itCurrentBuffers != currentPatchBuffersByCommandList_.end()) {
      for (unsigned patchBufferIndex : itCurrentBuffers->second) {
        HRESULT hr = c.object_.value->Signal(patchBufferFences_[patchBufferIndex].fence,
                                             ++patchBufferFences_[patchBufferIndex].fenceValue);
        GITS_ASSERT(hr == S_OK);
        patchBufferFences_[patchBufferIndex].waitingForExecute = false;
      }
      currentPatchBuffersByCommandList_.erase(itCurrentBuffers);
    }

    if (!useAddressPinning_) {
      auto itInstancesAoPCurrentBuffers = currentInstancesAoPPatchBuffersByCommandList_.find(key);
      if (itInstancesAoPCurrentBuffers != currentInstancesAoPPatchBuffersByCommandList_.end()) {
        for (unsigned patchBufferIndex : itInstancesAoPCurrentBuffers->second) {
          HRESULT hr = c.object_.value->Signal(
              instancesAoPPatchBufferFences_[patchBufferIndex].fence,
              ++instancesAoPPatchBufferFences_[patchBufferIndex].fenceValue);
          GITS_ASSERT(hr == S_OK);
          instancesAoPPatchBufferFences_[patchBufferIndex].waitingForExecute = false;
        }
        currentInstancesAoPPatchBuffersByCommandList_.erase(itInstancesAoPCurrentBuffers);
      }

      auto itCurrentInstancesBuffers = currentInstancesAoPStagingBuffersByCommandList_.find(key);
      if (itCurrentInstancesBuffers != currentInstancesAoPStagingBuffersByCommandList_.end()) {
        for (unsigned patchBufferIndex : itCurrentInstancesBuffers->second) {
          HRESULT hr = c.object_.value->Signal(
              instancesAoPStagingBufferFences_[patchBufferIndex].fence,
              ++instancesAoPStagingBufferFences_[patchBufferIndex].fenceValue);
          GITS_ASSERT(hr == S_OK);
          instancesAoPStagingBufferFences_[patchBufferIndex].waitingForExecute = false;
        }
        currentInstancesAoPStagingBuffersByCommandList_.erase(itCurrentInstancesBuffers);
      }
    }
  }

  dumpService_.executeCommandLists(c.key, c.object_.key, c.object_.value, c.ppCommandLists_.value,
                                   c.NumCommandLists_.value);
}

void GpuPatchLayer::post(ID3D12CommandQueueWaitCommand& c) {
  dumpService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void GpuPatchLayer::post(ID3D12CommandQueueSignalCommand& c) {
  dumpService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void GpuPatchLayer::post(ID3D12FenceSignalCommand& c) {
  dumpService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
}

void GpuPatchLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  dumpService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
}

void GpuPatchLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  dumpService_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
}

void GpuPatchLayer::post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  D3D12_COMMAND_SIGNATURE_DESC& desc = commandSignatures_[c.ppvCommandSignature_.key] =
      *c.pDesc_.value;
  desc.pArgumentDescs = new D3D12_INDIRECT_ARGUMENT_DESC[desc.NumArgumentDescs];
  std::copy(c.pDesc_.value->pArgumentDescs,
            c.pDesc_.value->pArgumentDescs + c.pDesc_.value->NumArgumentDescs,
            const_cast<D3D12_INDIRECT_ARGUMENT_DESC*>(desc.pArgumentDescs));
}

void GpuPatchLayer::pre(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  executeIndirectLastArgumentBufferOffset_ = c.ArgumentBufferOffset_.value;

  auto it = commandSignatures_.find(c.pCommandSignature_.key);
  GITS_ASSERT(it != commandSignatures_.end());
  D3D12_COMMAND_SIGNATURE_DESC& commandSignature = it->second;

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

  if (!raytracing && useAddressPinning_) {
    return;
  }

  ID3D12GraphicsCommandList* commandList = c.object_.value;
  if (!initialized_) {
    initialize(commandList);
  }

  auto& msgBus = CGits::Instance().GetMessageBus();
  msgBus.publish({PUBLISHER_PLAYER, TOPIC_GITS_WORKLOAD_BEGIN},
                 std::make_shared<GitsWorkloadMessage>(commandList, "GITS_ExecuteIndirect-Patch",
                                                       c.object_.key));

  unsigned patchBufferIndex = getPatchBufferIndex(c.object_.key, c.object_.value);

  unsigned mappingBufferIndex = getMappingBufferIndex(c.object_.key);
  if (!useAddressPinning_) {
    commandList->CopyResource(gpuAddressBuffers_[mappingBufferIndex],
                              gpuAddressStagingBuffers_[mappingBufferIndex]);
  }
  commandList->CopyResource(mappingCountBuffers_[mappingBufferIndex],
                            mappingCountStagingBuffers_[mappingBufferIndex]);
  if (raytracing) {
    commandList->CopyResource(shaderIdentifierBuffers_[mappingBufferIndex],
                              shaderIdentifierStagingBuffers_[mappingBufferIndex]);
    commandList->CopyResource(viewDescriptorBuffers_[mappingBufferIndex],
                              viewDescriptorStagingBuffers_[mappingBufferIndex]);
    commandList->CopyResource(sampleDescriptorBuffers_[mappingBufferIndex],
                              sampleDescriptorStagingBuffers_[mappingBufferIndex]);
  }

  {
    D3D12_RESOURCE_BARRIER barriers[2]{};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = patchBuffers_[patchBufferIndex];
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = c.pArgumentBuffer_.value;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    commandList->ResourceBarrier(2, barriers);

    unsigned size = c.MaxCommandCount_.value * commandSignature.ByteStride;
    commandList->CopyBufferRegion(patchBuffers_[patchBufferIndex], 0, c.pArgumentBuffer_.value,
                                  c.ArgumentBufferOffset_.value, size);

    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    commandList->ResourceBarrier(2, barriers);
  }
  c.pArgumentBuffer_.value = patchBuffers_[patchBufferIndex];
  c.ArgumentBufferOffset_.value = 0;

  if (c.pCountBuffer_.value) {
    D3D12_RESOURCE_BARRIER barriers[2]{};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = executeIndirectCountBuffers_[patchBufferIndex];
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = c.pCountBuffer_.value;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    commandList->ResourceBarrier(2, barriers);

    unsigned size = sizeof(unsigned);
    commandList->CopyBufferRegion(executeIndirectCountBuffers_[patchBufferIndex], 0,
                                  c.pCountBuffer_.value, c.CountBufferOffset_.value, size);

    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    commandList->ResourceBarrier(2, barriers);
  } else {
    void* data{};
    HRESULT hr = executeIndirectCountStagingBuffers_[patchBufferIndex]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, &c.MaxCommandCount_.value, sizeof(unsigned));
    executeIndirectCountStagingBuffers_[patchBufferIndex]->Unmap(0, nullptr);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = executeIndirectCountBuffers_[patchBufferIndex];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    commandList->ResourceBarrier(1, &barrier);

    commandList->CopyResource(executeIndirectCountBuffers_[patchBufferIndex],
                              executeIndirectCountStagingBuffers_[patchBufferIndex]);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    commandList->ResourceBarrier(1, &barrier);
  }

  std::vector<unsigned> patchOffsets;
  {
    getPatchOffsets(commandSignature, patchOffsets);

    void* data{};
    HRESULT hr = patchOffsetsStagingBuffers_[patchBufferIndex]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, patchOffsets.data(), sizeof(unsigned) * patchOffsets.size());
    patchOffsetsStagingBuffers_[patchBufferIndex]->Unmap(0, nullptr);

    commandList->CopyResource(patchOffsetsBuffers_[patchBufferIndex],
                              patchOffsetsStagingBuffers_[patchBufferIndex]);
  }

  struct RaytracingAddressMapping {
    uint64_t captureAddress;
    uint64_t playerAddress;
  };
  std::vector<RaytracingAddressMapping> raytracingPatches;
  {
    auto it = executeIndirectDispatchRays_.find(c.key);
    if (it != executeIndirectDispatchRays_.end()) {
      D3D12_DISPATCH_RAYS_DESC& desc = it->second;
      D3D12_DISPATCH_RAYS_DESC patchedDesc = desc;
      unsigned raytracingPatchBufferIndex = getPatchBufferIndex(c.object_.key, c.object_.value);
      patchDispatchRays(commandList, patchedDesc, raytracingPatchBufferIndex, mappingBufferIndex,
                        c.key);
      raytracingPatches.push_back(
          RaytracingAddressMapping{desc.RayGenerationShaderRecord.StartAddress,
                                   patchedDesc.RayGenerationShaderRecord.StartAddress});
      raytracingPatches.push_back(RaytracingAddressMapping{
          desc.MissShaderTable.StartAddress, patchedDesc.MissShaderTable.StartAddress});
      raytracingPatches.push_back(RaytracingAddressMapping{desc.HitGroupTable.StartAddress,
                                                           patchedDesc.HitGroupTable.StartAddress});
      raytracingPatches.push_back(RaytracingAddressMapping{
          desc.CallableShaderTable.StartAddress, patchedDesc.CallableShaderTable.StartAddress});

      void* data{};
      HRESULT hr =
          executeIndirectRaytracingPatchStagingBuffers_[patchBufferIndex]->Map(0, nullptr, &data);
      GITS_ASSERT(hr == S_OK);
      memcpy(data, raytracingPatches.data(),
             sizeof(RaytracingAddressMapping) * raytracingPatches.size());
      executeIndirectRaytracingPatchStagingBuffers_[patchBufferIndex]->Unmap(0, nullptr);

      commandList->CopyResource(executeIndirectRaytracingPatchBuffers_[patchBufferIndex],
                                executeIndirectRaytracingPatchStagingBuffers_[patchBufferIndex]);
    }
  }

  {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = nullptr;
    commandList->ResourceBarrier(1, &barrier);
  }

  dumpService_.dumpExecuteIndirectArgumentBuffer(
      c.object_.value, it->second, c.MaxCommandCount_.value, c.pArgumentBuffer_.value,
      c.ArgumentBufferOffset_.value, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, c.pCountBuffer_.value,
      c.CountBufferOffset_.value, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, c.key, true);

  executeIndirectShaderPatchService_.patchArgumentBuffer(
      commandList, patchBuffers_[patchBufferIndex]->GetGPUVirtualAddress(),
      executeIndirectCountBuffers_[patchBufferIndex]->GetGPUVirtualAddress(),
      patchOffsetsBuffers_[patchBufferIndex]->GetGPUVirtualAddress(), patchOffsets.size(),
      executeIndirectRaytracingPatchBuffers_[patchBufferIndex]->GetGPUVirtualAddress(),
      raytracingPatches.size(), c.MaxCommandCount_.value, commandSignature.ByteStride,
      useAddressPinning_ ? 0 : gpuAddressBuffers_[mappingBufferIndex]->GetGPUVirtualAddress(),
      mappingCountBuffers_[mappingBufferIndex]->GetGPUVirtualAddress());

  dumpService_.dumpExecuteIndirectArgumentBuffer(
      c.object_.value, it->second, c.MaxCommandCount_.value, c.pArgumentBuffer_.value,
      c.ArgumentBufferOffset_.value, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, c.pCountBuffer_.value,
      c.CountBufferOffset_.value, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, c.key, false);

  {
    D3D12_RESOURCE_BARRIER barriers[2]{};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[0].UAV.pResource = nullptr;

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = patchBuffers_[patchBufferIndex];
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;

    commandList->ResourceBarrier(2, barriers);
  }

  commandListService_.restoreState(c.object_.key, c.object_.value);

  msgBus.publish({PUBLISHER_PLAYER, TOPIC_GITS_WORKLOAD_END},
                 std::make_shared<GitsWorkloadMessage>(commandList, "GITS_ExecuteIndirect-Patch",
                                                       c.object_.key));
}

void GpuPatchLayer::post(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  c.ArgumentBufferOffset_.value = executeIndirectLastArgumentBufferOffset_;
}

void GpuPatchLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  addressService_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key, c.pDesc_.value->Flags);
  if (c.InitialState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
}

void GpuPatchLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  addressService_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key, c.pDesc_.value->Flags);
  if (c.result_.value != S_OK || c.InitialState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
}

void GpuPatchLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  addressService_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key, c.pDesc_.value->Flags);
  if (c.InitialLayout_.value == D3D12_BARRIER_LAYOUT_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
}

void GpuPatchLayer::post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
}

void GpuPatchLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
}

void GpuPatchLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
}

void GpuPatchLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  if (c.InitialLayout_.value == D3D12_BARRIER_LAYOUT_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
}

void GpuPatchLayer::getPatchOffsets(const D3D12_COMMAND_SIGNATURE_DESC& commandSignature,
                                    std::vector<unsigned>& patchOffsets) {
  unsigned offset = 0;
  for (unsigned j = 0; j < commandSignature.NumArgumentDescs; ++j) {
    const D3D12_INDIRECT_ARGUMENT_DESC& desc = commandSignature.pArgumentDescs[j];
    switch (desc.Type) {
    case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW: {
      offset += sizeof(D3D12_DRAW_ARGUMENTS);
      break;
    }
    case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED: {
      offset += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
      break;
    }
    case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH: {
      offset += sizeof(D3D12_DISPATCH_ARGUMENTS);
      break;
    }
    case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW: {
      patchOffsets.push_back(offset);
      offset += sizeof(D3D12_VERTEX_BUFFER_VIEW);
      break;
    }
    case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW: {
      patchOffsets.push_back(offset);
      offset += sizeof(D3D12_INDEX_BUFFER_VIEW);
      break;
    }
    case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT: {
      offset += sizeof(UINT);
      break;
    }
    case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW: {
      patchOffsets.push_back(offset);
      offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
      break;
    }
    case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW: {
      patchOffsets.push_back(offset);
      offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
      break;
    }
    case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW: {
      patchOffsets.push_back(offset);
      offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
      break;
    }
    case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS: {
      unsigned localOffset = offset;
      patchOffsets.push_back(localOffset);
      localOffset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS_RANGE);
      patchOffsets.push_back(localOffset);
      localOffset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE);
      patchOffsets.push_back(localOffset);
      localOffset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE);
      patchOffsets.push_back(localOffset);
      offset += sizeof(D3D12_DISPATCH_RAYS_DESC);
      break;
    }
    case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH: {
      offset += sizeof(D3D12_DISPATCH_MESH_ARGUMENTS);
      break;
    }
    }
  }
}

unsigned GpuPatchLayer::getMappingBufferIndex(unsigned commandListKey) {
  auto it = currentMappingsByCommandList_.find(commandListKey);
  if (it != currentMappingsByCommandList_.end()) {
    return it->second;
  }

  for (unsigned i = 0; i < mappingBufferPoolSize_; ++i) {
    if (mappingFences_[i].waitingForExecute) {
      continue;
    }
    UINT64 value = mappingFences_[i].fence->GetCompletedValue();
    if (value == UINT64_MAX) {
      LOG_ERROR << "getMappingBufferIndex - device removed!";
      exit(EXIT_FAILURE);
    }
    if (value == mappingFences_[i].fenceValue) {
      currentMappingsByCommandList_[commandListKey] = i;
      mappingFences_[i].waitingForExecute = true;
      return i;
    }
  }

  LOG_ERROR << "Mappings buffer pool is too small!";
  exit(EXIT_FAILURE);
}

unsigned GpuPatchLayer::getPatchBufferIndex(unsigned commandListKey,
                                            ID3D12GraphicsCommandList* commandList) {
  for (unsigned i = 0; i < patchBufferPoolSize_; ++i) {
    if (patchBufferFences_[i].waitingForExecute) {
      continue;
    }
    UINT64 value = patchBufferFences_[i].fence->GetCompletedValue();
    if (value == UINT64_MAX) {
      LOG_ERROR << "getPatchBufferIndex - device removed!";
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

unsigned GpuPatchLayer::getInstancesAoPPatchBufferIndex(unsigned commandListKey) {
  for (unsigned i = 0; i < instancesAoPPatchBufferPoolSize_; ++i) {
    if (instancesAoPPatchBufferFences_[i].waitingForExecute) {
      continue;
    }
    UINT64 value = instancesAoPPatchBufferFences_[i].fence->GetCompletedValue();
    if (value == UINT64_MAX) {
      LOG_ERROR << "getInstancesAoPPatchBufferIndex - device removed!";
      exit(EXIT_FAILURE);
    }
    if (value == instancesAoPPatchBufferFences_[i].fenceValue) {
      currentInstancesAoPPatchBuffersByCommandList_[commandListKey].push_back(i);
      instancesAoPPatchBufferFences_[i].waitingForExecute = true;
      return i;
    }
  }

  LOG_ERROR << "Instances array of pointers patch buffer pool is too small!";
  exit(EXIT_FAILURE);
}

unsigned GpuPatchLayer::getInstancesAoPStagingBufferIndex(unsigned commandListKey) {
  for (unsigned i = 0; i < instancesAoPStagingBufferPoolSize_; ++i) {
    if (instancesAoPStagingBufferFences_[i].waitingForExecute) {
      continue;
    }
    UINT64 value = instancesAoPStagingBufferFences_[i].fence->GetCompletedValue();
    if (value == UINT64_MAX) {
      LOG_ERROR << "getInstancesAoPStagingBufferIndex - device removed!";
      exit(EXIT_FAILURE);
    }
    if (value == instancesAoPStagingBufferFences_[i].fenceValue) {
      currentInstancesAoPStagingBuffersByCommandList_[commandListKey].push_back(i);
      instancesAoPStagingBufferFences_[i].waitingForExecute = true;
      return i;
    }
  }

  LOG_ERROR << "Instances array of pointers staging buffer pool is too small!";
  exit(EXIT_FAILURE);
}

void GpuPatchLayer::loadExecuteIndirectDispatchRays() {
  std::filesystem::path dumpPath = Configurator::Get().common.player.streamDir;
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
    auto it = executeIndirectDispatchRays_.find(callKey);
    if (it != executeIndirectDispatchRays_.end()) {
      static bool logged = false;
      if (!logged) {
        LOG_ERROR << "Multiple DispatchRays for ExecuteIndirect call not supported!";
        logged = true;
      }
    }
    executeIndirectDispatchRays_[callKey] = desc;
  }
}

void GpuPatchLayer::loadInstancesArraysOfPointers() {
  std::filesystem::path dumpPath = Configurator::Get().common.player.streamDir;
  std::ifstream stream(dumpPath / "raytracingArraysOfPointers.dat", std::ios::binary);
  while (true) {
    unsigned callKey{};
    stream.read(reinterpret_cast<char*>(&callKey), sizeof(unsigned));
    if (!stream) {
      break;
    }
    unsigned count{};
    stream.read(reinterpret_cast<char*>(&count), sizeof(unsigned));
    std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& addresses = instancesArraysOfPointers_[callKey];
    addresses.resize(count);
    for (unsigned i = 0; i < count; ++i) {
      stream.read(reinterpret_cast<char*>(&addresses[i]), sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
    }
  }
}

} // namespace DirectX
} // namespace gits
