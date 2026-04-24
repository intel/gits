// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuPatchLayer.h"
#include "playerManager.h"
#include "resourceStateEnhanced.h"
#include "log.h"
#include "messageBus.h"

#include <fstream>
#include <optional>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

GpuPatchLayer::GpuPatchLayer(PlayerManager& manager)
    : Layer("RaytracingGpuPatch"),
      m_Manager(manager),
      m_DumpService(m_AddressService, m_ShaderIdentifierService, m_DescriptorHandleService) {
  m_AddressService.EnablePlayerAddressLookup();
  m_UseAddressPinning =
      Configurator::Get().directx.player.addressPinning == AddressPinningMode::USE;
  LoadExecuteIndirectDispatchRays();
  if (!m_UseAddressPinning) {
    LoadInstancesArraysOfPointers();
  }
}

GpuPatchLayer::~GpuPatchLayer() {
  for (const auto& patchBufferInfo : m_PatchBufferInfos) {
    WaitForFence(patchBufferInfo.Fence.D3d12Fence.Get(), patchBufferInfo.Fence.FenceValue);
  }

  for (const auto& fenceInfo : m_MappingFences) {
    WaitForFence(fenceInfo.D3d12Fence.Get(), fenceInfo.FenceValue);
  }

  for (const auto& fenceInfo : m_InstancesAopPatchBufferFences) {
    WaitForFence(fenceInfo.D3d12Fence.Get(), fenceInfo.FenceValue);
  }

  for (const auto& fenceInfo : m_InstancesAopStagingBufferFences) {
    WaitForFence(fenceInfo.D3d12Fence.Get(), fenceInfo.FenceValue);
  }
}

void GpuPatchLayer::Pre(IUnknownReleaseCommand& c) {
  if (c.Skip) {
    return;
  }
  if (c.m_Result.Value == 0) {
    m_AddressService.DestroyInterface(c.m_Object.Key);
    m_DescriptorHandleService.DestroyHeap(c.m_Object.Key);
    m_CommandListService.Remove(c.m_Object.Key);

    const auto mappingsIt = m_CurrentMappingsByCommandList.find(c.m_Object.Key);
    if (mappingsIt != m_CurrentMappingsByCommandList.end()) {
      m_MappingFences[mappingsIt->second].WaitingForExecute = false;
      m_CurrentMappingsByCommandList.erase(mappingsIt);
    }

    const auto patchBuffersIt = m_CurrentPatchBuffersByCommandList.find(c.m_Object.Key);
    if (patchBuffersIt != m_CurrentPatchBuffersByCommandList.end()) {
      for (unsigned patchBufferIndex : patchBuffersIt->second) {
        m_PatchBufferInfos[patchBufferIndex].Fence.WaitingForExecute = false;
      }
      m_CurrentPatchBuffersByCommandList.erase(patchBuffersIt);
    }

    const auto aoPPatchBuffersIt =
        m_CurrentInstancesAopPatchBuffersByCommandList.find(c.m_Object.Key);
    if (aoPPatchBuffersIt != m_CurrentInstancesAopPatchBuffersByCommandList.end()) {
      for (unsigned aoPPatchBuffersIndex : aoPPatchBuffersIt->second) {
        m_InstancesAopPatchBufferFences[aoPPatchBuffersIndex].WaitingForExecute = false;
      }
      m_CurrentInstancesAopPatchBuffersByCommandList.erase(aoPPatchBuffersIt);
    }

    const auto aoPStagingPatchBuffersIt =
        m_CurrentInstancesAopStagingBuffersByCommandList.find(c.m_Object.Key);
    if (aoPStagingPatchBuffersIt != m_CurrentInstancesAopStagingBuffersByCommandList.end()) {
      for (unsigned aoPStagingPatchBuffersIndex : aoPStagingPatchBuffersIt->second) {
        m_InstancesAopStagingBufferFences[aoPStagingPatchBuffersIndex].WaitingForExecute = false;
      }
      m_CurrentInstancesAopStagingBuffersByCommandList.erase(aoPStagingPatchBuffersIt);
    }
  }
}

void GpuPatchLayer::Pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (c.Skip) {
    return;
  }
  D3D12_RESOURCE_DESC desc = c.m_Object.Value->GetDesc();
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    m_AddressService.AddGpuCaptureAddress(c.m_Object.Value, c.m_Object.Key, desc.Width,
                                          c.m_Result.Value);
  }
  m_ResourceByKey[c.m_Object.Key] = c.m_Object.Value;
}

void GpuPatchLayer::Post(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (c.Skip) {
    return;
  }
  D3D12_RESOURCE_DESC desc = c.m_Object.Value->GetDesc();
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    m_AddressService.AddGpuPlayerAddress(c.m_Object.Value, c.m_Object.Key, desc.Width,
                                         c.m_Result.Value);
  }
}

void GpuPatchLayer::Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  if (c.Skip) {
    return;
  }
  CapturePlayerShaderIdentifierService::ShaderIdentifier shaderIdentifier;
  memcpy(shaderIdentifier.data(), c.m_Result.Value, shaderIdentifier.size());
  m_ShaderIdentifierService.AddCaptureShaderIdentifier(c.Key, shaderIdentifier,
                                                       c.m_pExportName.Value);
}

void GpuPatchLayer::Post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  if (c.Skip) {
    return;
  }
  CapturePlayerShaderIdentifierService::ShaderIdentifier shaderIdentifier;
  memcpy(shaderIdentifier.data(), c.m_Result.Value, shaderIdentifier.size());
  m_ShaderIdentifierService.AddPlayerShaderIdentifier(c.Key, shaderIdentifier,
                                                      c.m_pExportName.Value);
}

void GpuPatchLayer::Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (c.Skip) {
    return;
  }
  m_DescriptorHandleService.AddCaptureHandle(c.m_Object.Value, c.m_Object.Key, c.m_Result.Value);
}

void GpuPatchLayer::Post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (c.Skip) {
    return;
  }
  m_DescriptorHandleService.AddPlayerHandle(c.m_Object.Key, c.m_Result.Value);
}

void GpuPatchLayer::Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.Skip || m_UseAddressPinning) {
    return;
  }

  if (c.m_pDesc.Value->Inputs.Type != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL ||
      c.m_pDesc.Value->Inputs.NumDescs == 0) {
    return;
  }

  ID3D12GraphicsCommandList* commandList = c.m_Object.Value;
  if (!m_Initialized) {
    Initialize(commandList);
  }

  gits::MessageBus::get().publish(
      {PUBLISHER_PLAYER, TOPIC_GITS_WORKLOAD_BEGIN},
      std::make_shared<GitsWorkloadMessage>(
          commandList, "GITS_BuildRaytracingAccelerationStructure-Patch", c.m_Object.Key));

  unsigned mappingBufferIndex = GetMappingBufferIndex(c.m_Object.Key, c.m_Object.Value);
  commandList->CopyResource(m_GpuAddressBuffers[mappingBufferIndex].Get(),
                            m_GpuAddressStagingBuffers[mappingBufferIndex].Get());
  commandList->CopyResource(m_MappingCountBuffers[mappingBufferIndex].Get(),
                            m_MappingCountStagingBuffers[mappingBufferIndex].Get());

  unsigned instanceDescsKey = c.m_pDesc.InputKeys[0];
  ID3D12Resource* instanceDescs = m_ResourceByKey[instanceDescsKey];
  GITS_ASSERT(instanceDescs);

  if (c.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {

    unsigned offset = c.m_pDesc.InputOffsets[0];
    unsigned size = c.m_pDesc.Value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

    unsigned patchBufferIndex = GetPatchBufferIndex(c.m_Object.Key, c.m_Object.Value, size);

    {
      {
        D3D12_RESOURCE_BARRIER patchBufferBarrier{};
        patchBufferBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        patchBufferBarrier.Transition.pResource = m_PatchBuffers[patchBufferIndex].Get();
        patchBufferBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        patchBufferBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        commandList->ResourceBarrier(1, &patchBufferBarrier);
      }

      BarrierState currentState =
          GetAdjustedCurrentState(m_ResourceStateTracker, m_AddressService, commandList,
                                  c.m_pDesc.Value->Inputs.InstanceDescs, instanceDescs,
                                  instanceDescsKey, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
      ResourceStateEnhanced resourceStateEnhanced(commandList, instanceDescs, currentState);
      resourceStateEnhanced.SetState(D3D12_RESOURCE_STATE_COPY_SOURCE);

      commandList->CopyBufferRegion(m_PatchBuffers[patchBufferIndex].Get(), 0, instanceDescs,
                                    offset, size);
      {
        D3D12_RESOURCE_BARRIER patchBufferBarrier{};
        patchBufferBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        patchBufferBarrier.Transition.pResource = m_PatchBuffers[patchBufferIndex].Get();
        patchBufferBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        patchBufferBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        commandList->ResourceBarrier(1, &patchBufferBarrier);
      }

      resourceStateEnhanced.RevertState();
    }

    D3D12_GPU_VIRTUAL_ADDRESS patchBufferAddress =
        m_PatchBuffers[patchBufferIndex]->GetGPUVirtualAddress();
    c.m_pDesc.Value->Inputs.InstanceDescs = patchBufferAddress;

    m_DumpService.DumpInstances(commandList, m_PatchBuffers[patchBufferIndex].Get(),
                                instanceDescsKey, size,
                                BarrierState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS), c.Key, true);

    m_RaytracingShaderPatchService.PatchInstances(
        commandList, patchBufferAddress, c.m_pDesc.Value->Inputs.NumDescs,
        m_GpuAddressBuffers[mappingBufferIndex]->GetGPUVirtualAddress(),
        m_MappingCountBuffers[mappingBufferIndex]->GetGPUVirtualAddress());

    m_DumpService.DumpInstances(commandList, m_PatchBuffers[patchBufferIndex].Get(),
                                instanceDescsKey, size,
                                BarrierState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS), c.Key, false);

    {
      D3D12_RESOURCE_BARRIER barriers[2]{};
      barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      barriers[0].UAV.pResource = nullptr;

      barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barriers[1].Transition.pResource = m_PatchBuffers[patchBufferIndex].Get();
      barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

      commandList->ResourceBarrier(2, barriers);
    }

  } else if (c.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {

    if (!m_InitializedInstancesAop) {
      InitializeInstancesAoP(commandList);
    }

    auto itInstances = m_InstancesArraysOfPointers.find(c.Key);
    GITS_ASSERT(itInstances != m_InstancesArraysOfPointers.end());
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
          m_AddressService.GetResourceInfoByCaptureAddress(arrayOfPointers[i]);
      GITS_ASSERT(resourceInfo);
      auto it = instancesByResourceKey.find(resourceInfo->Key);
      if (it == instancesByResourceKey.end()) {
        instanceInfos[i] = &instancesByResourceKey[resourceInfo->Key];
        instanceInfos[i]->resource = resourceInfo->Resource;
        instanceInfos[i]->resourceKey = resourceInfo->Key;
        instanceInfos[i]->captureStart = resourceInfo->CaptureStart;
        instanceInfos[i]->patchBufferIndex = GetInstancesAoPPatchBufferIndex(c.m_Object.Key);
      } else {
        instanceInfos[i] = &it->second;
      }
      unsigned offset = arrayOfPointers[i] - resourceInfo->CaptureStart;
      instanceInfos[i]->offsets.insert(offset);
    }

    std::vector<D3D12_GPU_VIRTUAL_ADDRESS> patchedArrayOfPointers(arrayOfPointers.size());
    for (unsigned i = 0; i < arrayOfPointers.size(); ++i) {
      unsigned offset =
          arrayOfPointers[i] - instanceInfos[i]->captureStart - *instanceInfos[i]->offsets.begin();
      patchedArrayOfPointers[i] =
          m_InstancesAopPatchBuffers[instanceInfos[i]->patchBufferIndex]->GetGPUVirtualAddress() +
          offset;
    }

    unsigned size = c.m_pDesc.Value->Inputs.NumDescs * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
    if (size > INSTANCES_AOP_STAGING_BUFFER_SIZE) {
      LOG_ERROR << "Raytracing staging buffer is too small for array of pointers to instances!";
      exit(EXIT_FAILURE);
    }

    unsigned instancesAoPBufferIndex = GetInstancesAoPStagingBufferIndex(c.m_Object.Key);
    void* data{};
    HRESULT hr = m_InstancesAopStagingBuffers[instancesAoPBufferIndex]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, patchedArrayOfPointers.data(),
           sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * patchedArrayOfPointers.size());
    m_InstancesAopStagingBuffers[instancesAoPBufferIndex]->Unmap(0, nullptr);

    {
      BarrierState resourceState =
          GetAdjustedCurrentState(m_ResourceStateTracker, m_AddressService, commandList,
                                  c.m_pDesc.Value->Inputs.InstanceDescs, instanceDescs,
                                  instanceDescsKey, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

      unsigned offset = c.m_pDesc.InputOffsets[0];
      m_DumpService.DumpInstancesArrayOfPointers(commandList, instanceDescs, instanceDescsKey,
                                                 offset, size, resourceState, c.Key, true);
    }

    unsigned patchBufferIndex = GetPatchBufferIndex(c.m_Object.Key, c.m_Object.Value, size);

    D3D12_GPU_VIRTUAL_ADDRESS patchBufferAddress =
        m_PatchBuffers[patchBufferIndex]->GetGPUVirtualAddress();
    c.m_pDesc.Value->Inputs.InstanceDescs = patchBufferAddress;

    {
      D3D12_RESOURCE_BARRIER barrier{};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = m_PatchBuffers[patchBufferIndex].Get();
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
      commandList->ResourceBarrier(1, &barrier);

      commandList->CopyBufferRegion(m_PatchBuffers[patchBufferIndex].Get(), 0,
                                    m_InstancesAopStagingBuffers[instancesAoPBufferIndex], 0, size);

      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = m_PatchBuffers[patchBufferIndex].Get();
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      commandList->ResourceBarrier(1, &barrier);
    }

    m_DumpService.DumpInstancesArrayOfPointers(
        commandList, m_PatchBuffers[patchBufferIndex].Get(), instanceDescsKey, 0, size,
        BarrierState(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE), c.Key, false);

    for (auto& it : instancesByResourceKey) {
      InstanceInfo& instanceInfo = it.second;
      unsigned patchBufferIndex = instanceInfo.patchBufferIndex;

      std::vector<unsigned> offsets(instanceInfo.offsets.size());
      std::copy(instanceInfo.offsets.begin(), instanceInfo.offsets.end(), offsets.begin());

      unsigned size = offsets.back() + sizeof(D3D12_RAYTRACING_INSTANCE_DESC) - offsets[0];
      if (size > INSTANCES_AOP_PATCH_BUFFER_SIZE) {
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
      if (offsetsByteSize > INSTANCES_AOP_PATCH_OFFSETS_BUFFER_SIZE) {
        LOG_ERROR << "Raytracing array of pointers offsets buffer is too small!";
        exit(EXIT_FAILURE);
      }

      void* data{};
      HRESULT hr =
          m_InstancesAopPatchOffsetsStagingBuffers[patchBufferIndex]->Map(0, nullptr, &data);
      GITS_ASSERT(hr == S_OK);
      memcpy(data, offsets.data(), offsetsByteSize);
      m_InstancesAopPatchOffsetsStagingBuffers[patchBufferIndex]->Unmap(0, nullptr);

      commandList->CopyResource(m_InstancesAopPatchOffsetsBuffers[patchBufferIndex],
                                m_InstancesAopPatchOffsetsStagingBuffers[patchBufferIndex]);

      {
        {
          D3D12_RESOURCE_BARRIER barrier{};
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Transition.pResource = m_InstancesAopPatchBuffers[patchBufferIndex];
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
          barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
          commandList->ResourceBarrier(1, &barrier);
        }

        BarrierState currentState = GetAdjustedCurrentState(
            m_ResourceStateTracker, m_AddressService, commandList,
            c.m_pDesc.Value->Inputs.InstanceDescs, instanceInfo.resource, instanceDescsKey,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        ResourceStateEnhanced resourceStateEnhanced(commandList, instanceInfo.resource,
                                                    currentState);
        resourceStateEnhanced.SetState(D3D12_RESOURCE_STATE_COPY_SOURCE);

        commandList->CopyBufferRegion(m_InstancesAopPatchBuffers[patchBufferIndex], 0,
                                      instanceInfo.resource, startOffset, size);

        {
          D3D12_RESOURCE_BARRIER barrier{};
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Transition.pResource = m_InstancesAopPatchBuffers[patchBufferIndex];
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
          barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
          commandList->ResourceBarrier(1, &barrier);
        }

        resourceStateEnhanced.RevertState();
      }

      D3D12_GPU_VIRTUAL_ADDRESS patchBufferAddress =
          m_InstancesAopPatchBuffers[patchBufferIndex]->GetGPUVirtualAddress();
      D3D12_GPU_VIRTUAL_ADDRESS patchOffsetsBufferAddress =
          m_InstancesAopPatchOffsetsBuffers[patchBufferIndex]->GetGPUVirtualAddress();

      m_DumpService.DumpInstances(commandList, m_InstancesAopPatchBuffers[patchBufferIndex],
                                  instanceInfo.resourceKey, size,
                                  BarrierState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS), c.Key, true);

      m_RaytracingShaderPatchService.PatchInstancesOffset(
          commandList, patchBufferAddress, patchOffsetsBufferAddress, offsets.size(),
          m_GpuAddressBuffers[mappingBufferIndex]->GetGPUVirtualAddress(),
          m_MappingCountBuffers[mappingBufferIndex]->GetGPUVirtualAddress());

      m_DumpService.DumpInstances(
          commandList, m_InstancesAopPatchBuffers[patchBufferIndex], instanceInfo.resourceKey, size,
          BarrierState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS), c.Key, false);

      {
        D3D12_RESOURCE_BARRIER barriers[2]{};
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[0].UAV.pResource = nullptr;

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Transition.pResource = m_InstancesAopPatchBuffers[patchBufferIndex];
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        commandList->ResourceBarrier(2, barriers);
      }
    }
  }

  m_CommandListService.RestoreState(c.m_Object.Key, c.m_Object.Value);

  gits::MessageBus::get().publish(
      {PUBLISHER_PLAYER, TOPIC_GITS_WORKLOAD_END},
      std::make_shared<GitsWorkloadMessage>(
          commandList, "GITS_BuildRaytracingAccelerationStructure-Patch", c.m_Object.Key));
}

void GpuPatchLayer::Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  if (c.Skip) {
    return;
  }
  ID3D12GraphicsCommandList* commandList = c.m_Object.Value;
  if (!m_Initialized) {
    Initialize(commandList);
  }

  gits::MessageBus::get().publish({PUBLISHER_PLAYER, TOPIC_GITS_WORKLOAD_BEGIN},
                                  std::make_shared<GitsWorkloadMessage>(
                                      commandList, "GITS_DispatchRays-Patch", c.m_Object.Key));

  unsigned patchBufferIndex = GetPatchBufferIndex(c.m_Object.Key, c.m_Object.Value,
                                                  GetDispatchRaysPatchSize(*c.m_pDesc.Value));

  unsigned mappingBufferIndex = GetMappingBufferIndex(c.m_Object.Key, c.m_Object.Value);
  if (!m_UseAddressPinning) {
    commandList->CopyResource(m_GpuAddressBuffers[mappingBufferIndex].Get(),
                              m_GpuAddressStagingBuffers[mappingBufferIndex].Get());
  }
  commandList->CopyResource(m_ShaderIdentifierBuffers[mappingBufferIndex].Get(),
                            m_ShaderIdentifierStagingBuffers[mappingBufferIndex].Get());
  commandList->CopyResource(m_ViewDescriptorBuffers[mappingBufferIndex].Get(),
                            m_ViewDescriptorStagingBuffers[mappingBufferIndex].Get());
  commandList->CopyResource(m_SampleDescriptorBuffers[mappingBufferIndex].Get(),
                            m_SampleDescriptorStagingBuffers[mappingBufferIndex].Get());
  commandList->CopyResource(m_MappingCountBuffers[mappingBufferIndex].Get(),
                            m_MappingCountStagingBuffers[mappingBufferIndex].Get());

  PatchDispatchRays(c.m_Object.Value, *c.m_pDesc.Value, patchBufferIndex, mappingBufferIndex,
                    c.Key);

  m_CommandListService.RestoreState(c.m_Object.Key, c.m_Object.Value);

  gits::MessageBus::get().publish({PUBLISHER_PLAYER, TOPIC_GITS_WORKLOAD_END},
                                  std::make_shared<GitsWorkloadMessage>(
                                      commandList, "GITS_DispatchRays-Patch", c.m_Object.Key));
}

void GpuPatchLayer::PatchDispatchRays(ID3D12GraphicsCommandList* commandList,
                                      D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc,
                                      unsigned patchBufferIndex,
                                      unsigned mappingBufferIndex,
                                      unsigned callKey) {

  D3D12_GPU_VIRTUAL_ADDRESS patchBufferAddress =
      m_PatchBuffers[patchBufferIndex]->GetGPUVirtualAddress();
  unsigned patchBufferOffset = 0;

  auto patchBindingTable = [&](D3D12_GPU_VIRTUAL_ADDRESS& startAddress, UINT64 sizeInBytes,
                               UINT64 strideInBytes,
                               GpuPatchDumpService::BindingTableType bindingTableType) {
    if (!startAddress || !sizeInBytes) {
      return;
    }

    CapturePlayerGpuAddressService::ResourceInfo* info =
        m_AddressService.GetResourceInfoByCaptureAddress(startAddress);
    GITS_ASSERT(info);
    unsigned offset = startAddress - info->CaptureStart;
    if (strideInBytes == 0) {
      strideInBytes = sizeInBytes;
    }
    unsigned stride = strideInBytes;
    unsigned count = sizeInBytes / strideInBytes;

    {
      {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_PatchBuffers[patchBufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        commandList->ResourceBarrier(1, &barrier);
      }

      BarrierState currentState = GetAdjustedCurrentState(
          m_ResourceStateTracker, m_AddressService, commandList, startAddress, info->Resource,
          info->Key, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
      ResourceStateEnhanced resourceStateEnhanced(commandList, info->Resource, currentState);
      resourceStateEnhanced.SetState(D3D12_RESOURCE_STATE_COPY_SOURCE);

      commandList->CopyBufferRegion(m_PatchBuffers[patchBufferIndex].Get(), patchBufferOffset,
                                    info->Resource, offset, stride * count);

      {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_PatchBuffers[patchBufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        commandList->ResourceBarrier(1, &barrier);
      }

      resourceStateEnhanced.RevertState();
    }

    startAddress = patchBufferAddress + patchBufferOffset;

    m_DumpService.DumpBindingTable(
        commandList, m_PatchBuffers[patchBufferIndex].Get(), patchBufferOffset, sizeInBytes, stride,
        BarrierState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS), callKey, bindingTableType, true);

    m_RaytracingShaderPatchService.PatchBindingTable(
        commandList, patchBufferAddress + patchBufferOffset, count, stride,
        m_UseAddressPinning ? 0 : m_GpuAddressBuffers[mappingBufferIndex]->GetGPUVirtualAddress(),
        m_ShaderIdentifierBuffers[mappingBufferIndex]->GetGPUVirtualAddress(),
        m_ViewDescriptorBuffers[mappingBufferIndex]->GetGPUVirtualAddress(),
        m_SampleDescriptorBuffers[mappingBufferIndex]->GetGPUVirtualAddress(),
        m_MappingCountBuffers[mappingBufferIndex]->GetGPUVirtualAddress(), !m_UseAddressPinning);

    m_DumpService.DumpBindingTable(
        commandList, m_PatchBuffers[patchBufferIndex].Get(), patchBufferOffset, sizeInBytes, stride,
        BarrierState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS), callKey, bindingTableType, false);

    patchBufferOffset += stride * count;
    patchBufferOffset =
        patchBufferOffset % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT == 0
            ? patchBufferOffset
            : ((patchBufferOffset / D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) + 1) *
                  D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
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

  {
    D3D12_RESOURCE_BARRIER barriers[2]{};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[0].UAV.pResource = nullptr;

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = m_PatchBuffers[patchBufferIndex].Get();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    commandList->ResourceBarrier(2, barriers);
  }
}

size_t GpuPatchLayer::GetDispatchRaysPatchSize(const D3D12_DISPATCH_RAYS_DESC& desc) const {
  size_t size{};
  const auto addBindingTableSize = [&size](D3D12_GPU_VIRTUAL_ADDRESS startAddress,
                                           UINT64 sizeInBytes, UINT64 strideInBytes) {
    if (!startAddress || !sizeInBytes) {
      return;
    }
    if (strideInBytes == 0) {
      strideInBytes = sizeInBytes;
    }
    unsigned stride = strideInBytes;
    unsigned count = sizeInBytes / strideInBytes;
    size += stride * count;
    size = size % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT == 0
               ? size
               : ((size / D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) + 1) *
                     D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
  };
  addBindingTableSize(desc.RayGenerationShaderRecord.StartAddress,
                      desc.RayGenerationShaderRecord.SizeInBytes,
                      desc.RayGenerationShaderRecord.SizeInBytes);
  addBindingTableSize(desc.MissShaderTable.StartAddress, desc.MissShaderTable.SizeInBytes,
                      desc.MissShaderTable.StrideInBytes);
  addBindingTableSize(desc.HitGroupTable.StartAddress, desc.HitGroupTable.SizeInBytes,
                      desc.HitGroupTable.StrideInBytes);
  addBindingTableSize(desc.CallableShaderTable.StartAddress, desc.CallableShaderTable.SizeInBytes,
                      desc.CallableShaderTable.StrideInBytes);
  return size;
}

void GpuPatchLayer::WaitForFence(ID3D12Fence* fence, unsigned fenceValue) {
  if (!fence) {
    return;
  }

  UINT64 value = fence->GetCompletedValue();
  if (value >= fenceValue) {
    return;
  }
  if (!m_WaitForFenceEvent) {
    m_WaitForFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    GITS_ASSERT(m_WaitForFenceEvent);
  }
  HRESULT hr = fence->SetEventOnCompletion(fenceValue, m_WaitForFenceEvent);
  GITS_ASSERT(hr == S_OK);
  DWORD timeout = 60000; // 60 sec
  if (Configurator::Get().directx.player.infiniteWaitForFence) {
    timeout = INFINITE;
  }
  DWORD ret = WaitForSingleObject(m_WaitForFenceEvent, timeout);
  if (ret == WAIT_TIMEOUT) {
    value = fence->GetCompletedValue();
    LOG_ERROR << "Gpu patching - timeout while waiting for fence value " << fenceValue
              << ". Current value " << value;
  }
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  if (c.Skip) {
    return;
  }
  m_CommandListService.StoreCommand(c);
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (c.Skip) {
    return;
  }
  m_CommandListService.StoreCommand(c);
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) {
  if (c.Skip) {
    return;
  }
  m_CommandListService.StoreCommand(c);
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c) {
  if (c.Skip) {
    return;
  }
  m_CommandListService.StoreCommand(c);
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  if (c.Skip) {
    return;
  }
  m_CommandListService.StoreCommand(c);
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  if (c.Skip) {
    return;
  }
  m_CommandListService.StoreCommand(c);
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  if (c.Skip) {
    return;
  }
  m_CommandListService.StoreCommand(c);
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  if (c.Skip) {
    return;
  }
  m_CommandListService.StoreCommand(c);
}

void GpuPatchLayer::Post(ID3D12DeviceCreateCommandListCommand& c) {
  if (c.Skip) {
    return;
  }
  m_CommandListService.StoreCommand(c);
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  if (c.Skip) {
    return;
  }
  m_CommandListService.StoreCommand(c);
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandListResetCommand& c) {
  if (c.Skip) {
    return;
  }
  m_CommandListService.Reset(c.m_Object.Key, c.m_pInitialState.Value);
}

void GpuPatchLayer::Initialize(ID3D12GraphicsCommandList* commandList) {
  for (unsigned i = 0; i < MAPPING_BUFFER_INITIAL_POOL_SIZE; ++i) {
    AddMappingBuffer(commandList);
  }

  for (unsigned i = 0; i < PATCH_BUFFER_INITIAL_POOL_SIZE; ++i) {
    AddPatchBuffer(commandList, PATCH_BUFFER_INITIAL_POOL_SIZE);
  }

  m_Initialized = true;
}

void GpuPatchLayer::InitializeInstancesAoP(ID3D12GraphicsCommandList* commandList) {
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

  for (unsigned i = 0; i < m_InstancesAopPatchBuffers.size(); ++i) {
    resourceDesc.Width = INSTANCES_AOP_PATCH_BUFFER_SIZE;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE,
                                         &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&m_InstancesAopPatchBuffers[i]));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < m_InstancesAopStagingBuffers.size(); ++i) {
    resourceDesc.Width = INSTANCES_AOP_STAGING_BUFFER_SIZE;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    hr = device->CreateCommittedResource(&heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&m_InstancesAopStagingBuffers[i]));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < m_InstancesAopPatchOffsetsBuffers.size(); ++i) {
    resourceDesc.Width = INSTANCES_AOP_PATCH_OFFSETS_BUFFER_SIZE;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE,
                                         &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&m_InstancesAopPatchOffsetsBuffers[i]));
    GITS_ASSERT(hr == S_OK);

    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    hr = device->CreateCommittedResource(
        &heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&m_InstancesAopPatchOffsetsStagingBuffers[i]));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < INSTANCES_AOP_PATCH_BUFFER_POOL_SIZE; ++i) {
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                             IID_PPV_ARGS(&m_InstancesAopPatchBufferFences[i].D3d12Fence));
    GITS_ASSERT(hr == S_OK);
  }

  for (unsigned i = 0; i < INSTANCES_AOP_STAGING_BUFFER_POOL_SIZE; ++i) {
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                             IID_PPV_ARGS(&m_InstancesAopStagingBufferFences[i].D3d12Fence));
    GITS_ASSERT(hr == S_OK);
  }

  m_InitializedInstancesAop = true;
}

void GpuPatchLayer::AddPatchBuffer(ID3D12GraphicsCommandList* commandList,
                                   unsigned patchBufferSize) {
  m_PatchBuffers.emplace_back();
  m_PatchOffsetsBuffers.emplace_back();
  m_PatchOffsetsStagingBuffers.emplace_back();
  m_ExecuteIndirectRaytracingPatchBuffers.emplace_back();
  m_ExecuteIndirectRaytracingPatchStagingBuffers.emplace_back();
  m_ExecuteIndirectCountBuffers.emplace_back();
  m_ExecuteIndirectCountStagingBuffers.emplace_back();
  m_PatchBufferInfos.emplace_back();

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  CreateOrReplacePatchBufferObjects(device.Get(), m_PatchBufferPoolSize++, patchBufferSize);
}

void GpuPatchLayer::CreateOrReplacePatchBufferObjects(ID3D12Device* device,
                                                      unsigned patchBufferIndex,
                                                      unsigned patchBufferSize) {
  GITS_ASSERT(patchBufferSize > 0);

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

  resourceDesc.Width = patchBufferSize;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  HRESULT hr{};
  hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr,
                                       IID_PPV_ARGS(&m_PatchBuffers[patchBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Width = PATCH_OFFSETS_BUFFER_SIZE;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr,
                                       IID_PPV_ARGS(&m_PatchOffsetsBuffers[patchBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  hr = device->CreateCommittedResource(
      &heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&m_PatchOffsetsStagingBuffers[patchBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Width = EXECUTE_INDIRECT_RAYTRACING_PATCH_BUFFER_SIZE;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  hr = device->CreateCommittedResource(
      &heapPropertiesDefault, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&m_ExecuteIndirectRaytracingPatchBuffers[patchBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  hr = device->CreateCommittedResource(
      &heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&m_ExecuteIndirectRaytracingPatchStagingBuffers[patchBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Width = EXECUTE_INDIRECT_COUNT_BUFFER_SIZE;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  hr = device->CreateCommittedResource(
      &heapPropertiesDefault, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&m_ExecuteIndirectCountBuffers[patchBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  hr = device->CreateCommittedResource(
      &heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&m_ExecuteIndirectCountStagingBuffers[patchBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  m_PatchBufferInfos[patchBufferIndex] = {};
  hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                           IID_PPV_ARGS(&m_PatchBufferInfos[patchBufferIndex].Fence.D3d12Fence));
  GITS_ASSERT(hr == S_OK);
  m_PatchBufferInfos[patchBufferIndex].Size = patchBufferSize;
}

void GpuPatchLayer::AddMappingBuffer(ID3D12GraphicsCommandList* commandList) {
  m_GpuAddressBuffers.emplace_back();
  m_GpuAddressStagingBuffers.emplace_back();
  m_ShaderIdentifierBuffers.emplace_back();
  m_ShaderIdentifierStagingBuffers.emplace_back();
  m_ViewDescriptorBuffers.emplace_back();
  m_ViewDescriptorStagingBuffers.emplace_back();
  m_SampleDescriptorBuffers.emplace_back();
  m_SampleDescriptorStagingBuffers.emplace_back();
  m_MappingCountBuffers.emplace_back();
  m_MappingCountStagingBuffers.emplace_back();
  m_MappingFences.emplace_back();

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  CreateMappingBufferObjects(device.Get(), m_MappingBufferPoolSize++);
}

void GpuPatchLayer::CreateMappingBufferObjects(ID3D12Device* device, unsigned mappingBufferIndex) {
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

  HRESULT hr{};

  if (!m_UseAddressPinning) {
    resourceDesc.Width = m_GpuAddressBufferSize;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE,
                                         &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&m_GpuAddressBuffers[mappingBufferIndex]));
    GITS_ASSERT(hr == S_OK);

    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    hr = device->CreateCommittedResource(
        &heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&m_GpuAddressStagingBuffers[mappingBufferIndex]));
    GITS_ASSERT(hr == S_OK);
  }

  resourceDesc.Width = m_ShaderIdentifierBufferSize;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  hr = device->CreateCommittedResource(
      &heapPropertiesDefault, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&m_ShaderIdentifierBuffers[mappingBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  hr = device->CreateCommittedResource(
      &heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&m_ShaderIdentifierStagingBuffers[mappingBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Width = m_ViewDescriptorBufferSize;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr,
                                       IID_PPV_ARGS(&m_ViewDescriptorBuffers[mappingBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  hr = device->CreateCommittedResource(
      &heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&m_ViewDescriptorStagingBuffers[mappingBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Width = m_SampleDescriptorBufferSize;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  hr = device->CreateCommittedResource(
      &heapPropertiesDefault, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&m_SampleDescriptorBuffers[mappingBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  hr = device->CreateCommittedResource(
      &heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&m_SampleDescriptorStagingBuffers[mappingBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Width = m_MappingCountBufferSize;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  hr = device->CreateCommittedResource(&heapPropertiesDefault, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr,
                                       IID_PPV_ARGS(&m_MappingCountBuffers[mappingBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  hr = device->CreateCommittedResource(
      &heapPropertiesUpload, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
      nullptr, IID_PPV_ARGS(&m_MappingCountStagingBuffers[mappingBufferIndex]));
  GITS_ASSERT(hr == S_OK);

  hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                           IID_PPV_ARGS(&m_MappingFences[mappingBufferIndex].D3d12Fence));
  GITS_ASSERT(hr == S_OK);
}

void GpuPatchLayer::Pre(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (c.Skip) {
    return;
  }
  std::vector<unsigned> mappingBuffers;
  for (unsigned key : c.m_ppCommandLists.Keys) {
    auto it = m_CurrentMappingsByCommandList.find(key);
    if (it != m_CurrentMappingsByCommandList.end()) {
      mappingBuffers.push_back(it->second);
    }
  }

  if (mappingBuffers.empty()) {
    return;
  }

  MappingCount mappingCount{};

  if (!m_UseAddressPinning) {
    std::vector<CapturePlayerGpuAddressService::GpuAddressMapping> gpuAddressMappings;
    m_AddressService.GetMappings(gpuAddressMappings);
    mappingCount.GpuAddressCount = gpuAddressMappings.size();

    if (gpuAddressMappings.size() >
        m_GpuAddressBufferSize / sizeof(CapturePlayerGpuAddressService::GpuAddressMapping)) {
      LOG_ERROR << "Raytracing gpuAddressMappings buffer is too small!";
      exit(EXIT_FAILURE);
    }
    for (unsigned index : mappingBuffers) {
      void* data{};
      HRESULT hr = m_GpuAddressStagingBuffers[index]->Map(0, nullptr, &data);
      GITS_ASSERT(hr == S_OK);
      memcpy(data, gpuAddressMappings.data(),
             sizeof(CapturePlayerGpuAddressService::GpuAddressMapping) * gpuAddressMappings.size());
      m_GpuAddressStagingBuffers[index]->Unmap(0, nullptr);
    }
  }

  std::vector<CapturePlayerShaderIdentifierService::ShaderIdentifierMapping>
      shaderIdentifierMappings;
  m_ShaderIdentifierService.GetMappings(shaderIdentifierMappings);
  mappingCount.ShaderIdentifierCount = shaderIdentifierMappings.size();
  if (shaderIdentifierMappings.size() >
      m_ShaderIdentifierBufferSize /
          sizeof(CapturePlayerShaderIdentifierService::ShaderIdentifierMapping)) {
    LOG_ERROR << "Raytracing shaderIdentifierMappings buffer is too small!";
    exit(EXIT_FAILURE);
  }
  for (unsigned index : mappingBuffers) {
    void* data{};
    HRESULT hr = m_ShaderIdentifierStagingBuffers[index]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, shaderIdentifierMappings.data(),
           sizeof(CapturePlayerShaderIdentifierService::ShaderIdentifierMapping) *
               shaderIdentifierMappings.size());
    m_ShaderIdentifierStagingBuffers[index]->Unmap(0, nullptr);
  }

  std::vector<CapturePlayerDescriptorHandleService::DescriptorMapping> viewDescriptorMappings;
  m_DescriptorHandleService.GetViewMappings(viewDescriptorMappings);
  mappingCount.ViewDescriptorCount = viewDescriptorMappings.size();
  if (viewDescriptorMappings.size() >
      m_ViewDescriptorBufferSize /
          sizeof(CapturePlayerDescriptorHandleService::DescriptorMapping)) {
    LOG_ERROR << "Raytracing viewDescriptorMappings buffer is too small!";
    exit(EXIT_FAILURE);
  }
  for (unsigned index : mappingBuffers) {
    void* data{};
    HRESULT hr = m_ViewDescriptorStagingBuffers[index]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, viewDescriptorMappings.data(),
           sizeof(CapturePlayerDescriptorHandleService::DescriptorMapping) *
               viewDescriptorMappings.size());
    m_ViewDescriptorStagingBuffers[index]->Unmap(0, nullptr);
  }

  std::vector<CapturePlayerDescriptorHandleService::DescriptorMapping> sampleDescriptorMappings;
  m_DescriptorHandleService.GetSamplerMappings(sampleDescriptorMappings);
  mappingCount.SampleDescriptorCount = sampleDescriptorMappings.size();
  if (sampleDescriptorMappings.size() >
      m_SampleDescriptorBufferSize /
          sizeof(CapturePlayerDescriptorHandleService::DescriptorMapping)) {
    LOG_ERROR << "Raytracing sampleDescriptorMappings buffer is too small!";
    exit(EXIT_FAILURE);
  }
  for (unsigned index : mappingBuffers) {
    void* data{};
    HRESULT hr = m_SampleDescriptorStagingBuffers[index]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, sampleDescriptorMappings.data(),
           sizeof(CapturePlayerDescriptorHandleService::DescriptorMapping) *
               sampleDescriptorMappings.size());
    m_SampleDescriptorStagingBuffers[index]->Unmap(0, nullptr);
  }

  for (unsigned index : mappingBuffers) {
    void* data{};
    HRESULT hr = m_MappingCountStagingBuffers[index]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, &mappingCount, sizeof(MappingCount));
    m_MappingCountStagingBuffers[index]->Unmap(0, nullptr);
  }

  m_ResourceStateTracker.ExecuteCommandLists(
      reinterpret_cast<ID3D12GraphicsCommandList**>(c.m_ppCommandLists.Value),
      c.m_NumCommandLists.Value);
}

void GpuPatchLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (c.Skip) {
    return;
  }
  for (unsigned key : c.m_ppCommandLists.Keys) {
    auto itMappings = m_CurrentMappingsByCommandList.find(key);
    if (itMappings != m_CurrentMappingsByCommandList.end()) {
      unsigned mappingBufferIndex = itMappings->second;
      HRESULT hr = c.m_Object.Value->Signal(m_MappingFences[mappingBufferIndex].D3d12Fence.Get(),
                                            ++m_MappingFences[mappingBufferIndex].FenceValue);
      GITS_ASSERT(hr == S_OK);
      m_MappingFences[mappingBufferIndex].WaitingForExecute = false;
      m_CurrentMappingsByCommandList.erase(itMappings);
    }

    auto itCurrentBuffers = m_CurrentPatchBuffersByCommandList.find(key);
    if (itCurrentBuffers != m_CurrentPatchBuffersByCommandList.end()) {
      for (unsigned patchBufferIndex : itCurrentBuffers->second) {
        HRESULT hr =
            c.m_Object.Value->Signal(m_PatchBufferInfos[patchBufferIndex].Fence.D3d12Fence.Get(),
                                     ++m_PatchBufferInfos[patchBufferIndex].Fence.FenceValue);
        GITS_ASSERT(hr == S_OK);
        m_PatchBufferInfos[patchBufferIndex].Fence.WaitingForExecute = false;
      }
      m_CurrentPatchBuffersByCommandList.erase(itCurrentBuffers);
    }

    if (!m_UseAddressPinning) {
      auto itInstancesAoPCurrentBuffers = m_CurrentInstancesAopPatchBuffersByCommandList.find(key);
      if (itInstancesAoPCurrentBuffers != m_CurrentInstancesAopPatchBuffersByCommandList.end()) {
        for (unsigned patchBufferIndex : itInstancesAoPCurrentBuffers->second) {
          HRESULT hr = c.m_Object.Value->Signal(
              m_InstancesAopPatchBufferFences[patchBufferIndex].D3d12Fence.Get(),
              ++m_InstancesAopPatchBufferFences[patchBufferIndex].FenceValue);
          GITS_ASSERT(hr == S_OK);
          m_InstancesAopPatchBufferFences[patchBufferIndex].WaitingForExecute = false;
        }
        m_CurrentInstancesAopPatchBuffersByCommandList.erase(itInstancesAoPCurrentBuffers);
      }

      auto itCurrentInstancesBuffers = m_CurrentInstancesAopStagingBuffersByCommandList.find(key);
      if (itCurrentInstancesBuffers != m_CurrentInstancesAopStagingBuffersByCommandList.end()) {
        for (unsigned patchBufferIndex : itCurrentInstancesBuffers->second) {
          HRESULT hr = c.m_Object.Value->Signal(
              m_InstancesAopStagingBufferFences[patchBufferIndex].D3d12Fence.Get(),
              ++m_InstancesAopStagingBufferFences[patchBufferIndex].FenceValue);
          GITS_ASSERT(hr == S_OK);
          m_InstancesAopStagingBufferFences[patchBufferIndex].WaitingForExecute = false;
        }
        m_CurrentInstancesAopStagingBuffersByCommandList.erase(itCurrentInstancesBuffers);
      }
    }
  }

  m_DumpService.ExecuteCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                    c.m_ppCommandLists.Value, c.m_NumCommandLists.Value);
}

void GpuPatchLayer::Post(ID3D12CommandQueueWaitCommand& c) {
  if (c.Skip) {
    return;
  }
  m_DumpService.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void GpuPatchLayer::Post(ID3D12CommandQueueSignalCommand& c) {
  if (c.Skip) {
    return;
  }
  m_DumpService.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void GpuPatchLayer::Post(ID3D12FenceSignalCommand& c) {
  if (c.Skip) {
    return;
  }
  m_DumpService.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
}

void GpuPatchLayer::Post(ID3D12DeviceCreateFenceCommand& c) {
  if (c.Skip) {
    return;
  }
  m_DumpService.FenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
}

void GpuPatchLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (c.Skip) {
    return;
  }
  m_DumpService.FenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
}

void GpuPatchLayer::Post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  if (c.Skip) {
    return;
  }
  m_CommandSignatures[c.m_ppvCommandSignature.Key].reset(
      new PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>(c.m_pDesc));
}

void GpuPatchLayer::Pre(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  if (c.Skip) {
    return;
  }
  m_ExecuteIndirectLastArgumentBufferOffset = c.m_ArgumentBufferOffset.Value;

  auto it = m_CommandSignatures.find(c.m_pCommandSignature.Key);
  GITS_ASSERT(it != m_CommandSignatures.end());
  D3D12_COMMAND_SIGNATURE_DESC& commandSignature = *it->second->Value;

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

  if (!raytracing && m_UseAddressPinning) {
    return;
  }

  ID3D12GraphicsCommandList* commandList = c.m_Object.Value;
  if (!m_Initialized) {
    Initialize(commandList);
  }

  gits::MessageBus::get().publish({PUBLISHER_PLAYER, TOPIC_GITS_WORKLOAD_BEGIN},
                                  std::make_shared<GitsWorkloadMessage>(
                                      commandList, "GITS_ExecuteIndirect-Patch", c.m_Object.Key));

  size_t size = c.m_MaxCommandCount.Value * commandSignature.ByteStride;
  GITS_ASSERT(c.m_pArgumentBuffer.Value->GetDesc().Width > c.m_ArgumentBufferOffset.Value);
  size_t maxSizeBuffer =
      c.m_pArgumentBuffer.Value->GetDesc().Width - c.m_ArgumentBufferOffset.Value;
  if (size > maxSizeBuffer) {
    size = maxSizeBuffer;
  }
  GITS_ASSERT(size <= PATCH_BUFFER_INITIAL_SIZE &&
              "ExecuteIndirect estimated argument buffer size larger than expected");

  unsigned patchBufferIndex = GetPatchBufferIndex(c.m_Object.Key, c.m_Object.Value, size);

  unsigned mappingBufferIndex{};
  if (m_CurrentMappingsByCommandList.find(c.m_Object.Key) == m_CurrentMappingsByCommandList.end()) {
    mappingBufferIndex = GetMappingBufferIndex(c.m_Object.Key, c.m_Object.Value);
    if (!m_UseAddressPinning) {
      commandList->CopyResource(m_GpuAddressBuffers[mappingBufferIndex].Get(),
                                m_GpuAddressStagingBuffers[mappingBufferIndex].Get());
    }
    commandList->CopyResource(m_MappingCountBuffers[mappingBufferIndex].Get(),
                              m_MappingCountStagingBuffers[mappingBufferIndex].Get());
  } else {
    mappingBufferIndex = GetMappingBufferIndex(c.m_Object.Key, c.m_Object.Value);
  }

  if (raytracing) {
    commandList->CopyResource(m_ShaderIdentifierBuffers[mappingBufferIndex].Get(),
                              m_ShaderIdentifierStagingBuffers[mappingBufferIndex].Get());
    commandList->CopyResource(m_ViewDescriptorBuffers[mappingBufferIndex].Get(),
                              m_ViewDescriptorStagingBuffers[mappingBufferIndex].Get());
    commandList->CopyResource(m_SampleDescriptorBuffers[mappingBufferIndex].Get(),
                              m_SampleDescriptorStagingBuffers[mappingBufferIndex].Get());
  }

  {
    {
      D3D12_RESOURCE_BARRIER barrier{};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = m_PatchBuffers[patchBufferIndex].Get();
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
      commandList->ResourceBarrier(1, &barrier);
    }

    BarrierState argumentBufferState =
        GetAdjustedCurrentState(m_ResourceStateTracker, m_AddressService, commandList,
                                c.m_pArgumentBuffer.Value, c.m_ArgumentBufferOffset.Value,
                                c.m_pArgumentBuffer.Key, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    ResourceStateEnhanced argumentBufferStateEnhanced(commandList, c.m_pArgumentBuffer.Value,
                                                      argumentBufferState);
    argumentBufferStateEnhanced.SetState(D3D12_RESOURCE_STATE_COPY_SOURCE);

    commandList->CopyBufferRegion(m_PatchBuffers[patchBufferIndex].Get(), 0,
                                  c.m_pArgumentBuffer.Value, c.m_ArgumentBufferOffset.Value, size);

    {
      D3D12_RESOURCE_BARRIER barrier{};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = m_PatchBuffers[patchBufferIndex].Get();
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      commandList->ResourceBarrier(1, &barrier);
    }

    argumentBufferStateEnhanced.RevertState();
  }
  c.m_pArgumentBuffer.Value = m_PatchBuffers[patchBufferIndex].Get();
  c.m_ArgumentBufferOffset.Value = 0;

  if (c.m_pCountBuffer.Value) {
    {
      D3D12_RESOURCE_BARRIER barrier{};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = m_ExecuteIndirectCountBuffers[patchBufferIndex].Get();
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
      commandList->ResourceBarrier(1, &barrier);
    }

    BarrierState countBufferState = GetAdjustedCurrentState(
        m_ResourceStateTracker, m_AddressService, commandList, c.m_pCountBuffer.Value,
        c.m_CountBufferOffset.Value, c.m_pCountBuffer.Key, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    ResourceStateEnhanced countBufferStateEnhanced(commandList, c.m_pCountBuffer.Value,
                                                   countBufferState);
    countBufferStateEnhanced.SetState(D3D12_RESOURCE_STATE_COPY_SOURCE);

    unsigned size = sizeof(unsigned);
    commandList->CopyBufferRegion(m_ExecuteIndirectCountBuffers[patchBufferIndex].Get(), 0,
                                  c.m_pCountBuffer.Value, c.m_CountBufferOffset.Value, size);

    {
      D3D12_RESOURCE_BARRIER barrier{};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = m_ExecuteIndirectCountBuffers[patchBufferIndex].Get();
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      commandList->ResourceBarrier(1, &barrier);
    }
    countBufferStateEnhanced.RevertState();

  } else {
    void* data{};
    HRESULT hr = m_ExecuteIndirectCountStagingBuffers[patchBufferIndex]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, &c.m_MaxCommandCount.Value, sizeof(unsigned));
    m_ExecuteIndirectCountStagingBuffers[patchBufferIndex]->Unmap(0, nullptr);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_ExecuteIndirectCountBuffers[patchBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    commandList->ResourceBarrier(1, &barrier);

    commandList->CopyResource(m_ExecuteIndirectCountBuffers[patchBufferIndex].Get(),
                              m_ExecuteIndirectCountStagingBuffers[patchBufferIndex].Get());

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    commandList->ResourceBarrier(1, &barrier);
  }

  std::vector<unsigned> patchOffsets;
  {
    GetPatchOffsets(commandSignature, patchOffsets);

    void* data{};
    HRESULT hr = m_PatchOffsetsStagingBuffers[patchBufferIndex]->Map(0, nullptr, &data);
    GITS_ASSERT(hr == S_OK);
    memcpy(data, patchOffsets.data(), sizeof(unsigned) * patchOffsets.size());
    m_PatchOffsetsStagingBuffers[patchBufferIndex]->Unmap(0, nullptr);

    commandList->CopyResource(m_PatchOffsetsBuffers[patchBufferIndex].Get(),
                              m_PatchOffsetsStagingBuffers[patchBufferIndex].Get());
  }

  struct RaytracingAddressMapping {
    uint64_t captureAddress;
    uint64_t playerAddress;
  };
  std::vector<RaytracingAddressMapping> raytracingPatches;
  {
    auto it = m_ExecuteIndirectDispatchRays.find(c.Key);
    if (it != m_ExecuteIndirectDispatchRays.end()) {
      D3D12_DISPATCH_RAYS_DESC& desc = it->second;
      D3D12_DISPATCH_RAYS_DESC patchedDesc = desc;
      unsigned raytracingPatchBufferIndex =
          GetPatchBufferIndex(c.m_Object.Key, c.m_Object.Value, GetDispatchRaysPatchSize(desc));
      PatchDispatchRays(commandList, patchedDesc, raytracingPatchBufferIndex, mappingBufferIndex,
                        c.Key);
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
          m_ExecuteIndirectRaytracingPatchStagingBuffers[patchBufferIndex]->Map(0, nullptr, &data);
      GITS_ASSERT(hr == S_OK);
      memcpy(data, raytracingPatches.data(),
             sizeof(RaytracingAddressMapping) * raytracingPatches.size());
      m_ExecuteIndirectRaytracingPatchStagingBuffers[patchBufferIndex]->Unmap(0, nullptr);

      commandList->CopyResource(
          m_ExecuteIndirectRaytracingPatchBuffers[patchBufferIndex].Get(),
          m_ExecuteIndirectRaytracingPatchStagingBuffers[patchBufferIndex].Get());
    }
  }

  {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_ExecuteIndirectRaytracingPatchBuffers[patchBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);
  }

  {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = nullptr;
    commandList->ResourceBarrier(1, &barrier);
  }

  {
    BarrierState argumentBufferState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    BarrierState countBufferState{};
    if (c.m_pCountBuffer.Value) {
      countBufferState =
          GetAdjustedCurrentState(m_ResourceStateTracker, m_AddressService, commandList,
                                  c.m_pCountBuffer.Value, c.m_CountBufferOffset.Value,
                                  c.m_pCountBuffer.Key, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    }

    m_DumpService.DumpExecuteIndirectArgumentBuffer(
        c.m_Object.Value, it->second->Value, c.m_MaxCommandCount.Value, c.m_pArgumentBuffer.Value,
        c.m_ArgumentBufferOffset.Value, argumentBufferState, c.m_pCountBuffer.Value,
        c.m_CountBufferOffset.Value, countBufferState, c.Key, true);

    m_ExecuteIndirectShaderPatchService.PatchArgumentBuffer(
        commandList, m_PatchBuffers[patchBufferIndex]->GetGPUVirtualAddress(),
        m_ExecuteIndirectCountBuffers[patchBufferIndex]->GetGPUVirtualAddress(),
        m_PatchOffsetsBuffers[patchBufferIndex]->GetGPUVirtualAddress(), patchOffsets.size(),
        m_ExecuteIndirectRaytracingPatchBuffers[patchBufferIndex]->GetGPUVirtualAddress(),
        raytracingPatches.size(), c.m_MaxCommandCount.Value, commandSignature.ByteStride,
        m_UseAddressPinning ? 0 : m_GpuAddressBuffers[mappingBufferIndex]->GetGPUVirtualAddress(),
        m_MappingCountBuffers[mappingBufferIndex]->GetGPUVirtualAddress());

    m_DumpService.DumpExecuteIndirectArgumentBuffer(
        c.m_Object.Value, it->second->Value, c.m_MaxCommandCount.Value, c.m_pArgumentBuffer.Value,
        c.m_ArgumentBufferOffset.Value, argumentBufferState, c.m_pCountBuffer.Value,
        c.m_CountBufferOffset.Value, countBufferState, c.Key, false);
  }

  {
    D3D12_RESOURCE_BARRIER barriers[2]{};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[0].UAV.pResource = nullptr;

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = m_PatchBuffers[patchBufferIndex].Get();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;

    commandList->ResourceBarrier(2, barriers);
  }

  m_CommandListService.RestoreState(c.m_Object.Key, c.m_Object.Value);

  gits::MessageBus::get().publish({PUBLISHER_PLAYER, TOPIC_GITS_WORKLOAD_END},
                                  std::make_shared<GitsWorkloadMessage>(
                                      commandList, "GITS_ExecuteIndirect-Patch", c.m_Object.Key));
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  if (c.Skip) {
    return;
  }
  c.m_ArgumentBufferOffset.Value = m_ExecuteIndirectLastArgumentBufferOffset;
}

void GpuPatchLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK ||
      c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  m_AddressService.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key, c.m_pDesc.Value->Flags);
  m_ResourceStateTracker.AddResource(*reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value),
                                     c.m_ppvResource.Key, c.m_InitialState.Value);
}

void GpuPatchLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK ||
      c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  m_AddressService.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key, c.m_pDesc.Value->Flags);
  m_ResourceStateTracker.AddResource(*reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value),
                                     c.m_ppvResource.Key, c.m_InitialState.Value);
}

void GpuPatchLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK ||
      c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  m_AddressService.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key, c.m_pDesc.Value->Flags);
  m_ResourceStateTracker.AddResource(*reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value),
                                     c.m_ppvResource.Key, c.m_InitialLayout.Value);
}

void GpuPatchLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK ||
      c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  m_ResourceStateTracker.AddResource(*reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value),
                                     c.m_ppvResource.Key, c.m_InitialResourceState.Value);
}

void GpuPatchLayer::Post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK ||
      c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  m_ResourceStateTracker.AddResource(*reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value),
                                     c.m_ppvResource.Key, c.m_InitialResourceState.Value);
}

void GpuPatchLayer::Post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK ||
      c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  m_ResourceStateTracker.AddResource(*reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value),
                                     c.m_ppvResource.Key, c.m_InitialResourceState.Value);
}

void GpuPatchLayer::Post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK ||
      c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  m_ResourceStateTracker.AddResource(*reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value),
                                     c.m_ppvResource.Key, c.m_InitialLayout.Value);
}

void GpuPatchLayer::Post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK ||
      c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  m_ResourceStateTracker.AddResource(*reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value),
                                     c.m_ppvResource.Key, c.m_InitialState.Value);
}

void GpuPatchLayer::Post(ID3D12Device4CreateReservedResource1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK ||
      c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  m_ResourceStateTracker.AddResource(*reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value),
                                     c.m_ppvResource.Key, c.m_InitialState.Value);
}

void GpuPatchLayer::Post(ID3D12Device10CreateReservedResource2Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK ||
      c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  m_ResourceStateTracker.AddResource(*reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value),
                                     c.m_ppvResource.Key, c.m_InitialLayout.Value);
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  if (c.Skip) {
    return;
  }
  m_ResourceStateTracker.ResourceBarrier(c.m_Object.Value, c.m_pBarriers.Value,
                                         c.m_NumBarriers.Value, c.m_pBarriers.ResourceKeys.data());
}

void GpuPatchLayer::GetPatchOffsets(const D3D12_COMMAND_SIGNATURE_DESC& commandSignature,
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
      offset += desc.Constant.Num32BitValuesToSet * sizeof(UINT);
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

unsigned GpuPatchLayer::GetMappingBufferIndex(unsigned commandListKey,
                                              ID3D12GraphicsCommandList* commandList) {
  auto it = m_CurrentMappingsByCommandList.find(commandListKey);
  if (it != m_CurrentMappingsByCommandList.end()) {
    return it->second;
  }

  for (unsigned i = 0; i < m_MappingBufferPoolSize; ++i) {
    if (m_MappingFences[i].WaitingForExecute) {
      continue;
    }
    UINT64 value = m_MappingFences[i].D3d12Fence->GetCompletedValue();
    if (value == UINT64_MAX) {
      LOG_ERROR << "GetMappingBufferIndex - device removed!";
      exit(EXIT_FAILURE);
    }
    if (value == m_MappingFences[i].FenceValue) {
      m_CurrentMappingsByCommandList[commandListKey] = i;
      m_MappingFences[i].WaitingForExecute = true;
      return i;
    }
  }

  AddMappingBuffer(commandList);
  unsigned newIndex = m_MappingBufferPoolSize - 1;
  m_CurrentMappingsByCommandList[commandListKey] = newIndex;
  m_MappingFences[newIndex].WaitingForExecute = true;

  return newIndex;
}

unsigned GpuPatchLayer::GetPatchBufferIndex(unsigned commandListKey,
                                            ID3D12GraphicsCommandList* commandList,
                                            size_t size) {
  std::optional<unsigned> smallestFittingBuffer;
  std::optional<unsigned> largestNotFittingBuffer;
  for (unsigned i = 0; i < m_PatchBufferPoolSize; ++i) {
    const auto& patchBufferInfo = m_PatchBufferInfos[i];
    if (patchBufferInfo.Fence.WaitingForExecute) {
      continue;
    }
    UINT64 value = patchBufferInfo.Fence.D3d12Fence->GetCompletedValue();
    if (value == UINT64_MAX) {
      LOG_ERROR << "GetPatchBufferIndex - device removed!";
      exit(EXIT_FAILURE);
    }
    if (value != patchBufferInfo.Fence.FenceValue) {
      continue;
    }

    if (patchBufferInfo.Size >= size) {
      if (!smallestFittingBuffer.has_value() ||
          m_PatchBufferInfos[smallestFittingBuffer.value()].Size > patchBufferInfo.Size) {
        smallestFittingBuffer = i;
      }
    } else {
      if (!largestNotFittingBuffer.has_value() ||
          m_PatchBufferInfos[largestNotFittingBuffer.value()].Size < patchBufferInfo.Size) {
        largestNotFittingBuffer = i;
      }
    }
  }

  unsigned patchBufferIndex{};
  if (smallestFittingBuffer.has_value()) {
    patchBufferIndex = smallestFittingBuffer.value();
  } else if (largestNotFittingBuffer.has_value()) {
    patchBufferIndex = largestNotFittingBuffer.value();
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(hr == S_OK);
    CreateOrReplacePatchBufferObjects(device.Get(), patchBufferIndex,
                                      size * PATCH_BUFFER_SIZE_MULTIPLIER);
  } else {
    AddPatchBuffer(commandList, size * PATCH_BUFFER_SIZE_MULTIPLIER);
    patchBufferIndex = m_PatchBufferPoolSize - 1;
  }

  m_CurrentPatchBuffersByCommandList[commandListKey].push_back(patchBufferIndex);
  m_PatchBufferInfos[patchBufferIndex].Fence.WaitingForExecute = true;
  return patchBufferIndex;
}

unsigned GpuPatchLayer::GetInstancesAoPPatchBufferIndex(unsigned commandListKey) {
  for (unsigned i = 0; i < INSTANCES_AOP_PATCH_BUFFER_POOL_SIZE; ++i) {
    if (m_InstancesAopPatchBufferFences[i].WaitingForExecute) {
      continue;
    }
    UINT64 value = m_InstancesAopPatchBufferFences[i].D3d12Fence->GetCompletedValue();
    if (value == UINT64_MAX) {
      LOG_ERROR << "GetInstancesAoPPatchBufferIndex - device removed!";
      exit(EXIT_FAILURE);
    }
    if (value == m_InstancesAopPatchBufferFences[i].FenceValue) {
      m_CurrentInstancesAopPatchBuffersByCommandList[commandListKey].push_back(i);
      m_InstancesAopPatchBufferFences[i].WaitingForExecute = true;
      return i;
    }
  }

  LOG_ERROR << "Instances array of pointers patch buffer pool is too small!";
  exit(EXIT_FAILURE);
}

unsigned GpuPatchLayer::GetInstancesAoPStagingBufferIndex(unsigned commandListKey) {
  for (unsigned i = 0; i < INSTANCES_AOP_STAGING_BUFFER_POOL_SIZE; ++i) {
    if (m_InstancesAopStagingBufferFences[i].WaitingForExecute) {
      continue;
    }
    UINT64 value = m_InstancesAopStagingBufferFences[i].D3d12Fence->GetCompletedValue();
    if (value == UINT64_MAX) {
      LOG_ERROR << "GetInstancesAoPStagingBufferIndex - device removed!";
      exit(EXIT_FAILURE);
    }
    if (value == m_InstancesAopStagingBufferFences[i].FenceValue) {
      m_CurrentInstancesAopStagingBuffersByCommandList[commandListKey].push_back(i);
      m_InstancesAopStagingBufferFences[i].WaitingForExecute = true;
      return i;
    }
  }

  LOG_ERROR << "Instances array of pointers staging buffer pool is too small!";
  exit(EXIT_FAILURE);
}

void GpuPatchLayer::LoadExecuteIndirectDispatchRays() {
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
    auto it = m_ExecuteIndirectDispatchRays.find(callKey);
    if (it != m_ExecuteIndirectDispatchRays.end()) {
      static bool logged = false;
      if (!logged) {
        LOG_ERROR << "Multiple DispatchRays for ExecuteIndirect call not supported!";
        logged = true;
      }
    }
    m_ExecuteIndirectDispatchRays[callKey] = desc;
  }
}

void GpuPatchLayer::LoadInstancesArraysOfPointers() {
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
    std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& addresses = m_InstancesArraysOfPointers[callKey];
    addresses.resize(count);
    for (unsigned i = 0; i < count; ++i) {
      stream.read(reinterpret_cast<char*>(&addresses[i]), sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
    }
  }
}

} // namespace DirectX
} // namespace gits
