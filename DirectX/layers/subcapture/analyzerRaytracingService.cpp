// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerRaytracingService.h"
#include "analyzerCommandListService.h"
#include "resourceStateEnhanced.h"
#include "log.h"

#include <fstream>
#include <queue>

namespace gits {
namespace DirectX {

AnalyzerRaytracingService::AnalyzerRaytracingService(
    DescriptorService& descriptorService,
    CapturePlayerGpuAddressService& gpuAddressService,
    CapturePlayerDescriptorHandleService& descriptorHandleService,
    CapturePlayerShaderIdentifierService& shaderIdentifierService,
    AnalyzerCommandListService& commandListService,
    DescriptorRootSignatureService& rootSignatureService,
    ResourceStateTracker& resourceStateTracker)
    : m_GpuAddressService(gpuAddressService),
      m_DescriptorHandleService(descriptorHandleService),
      m_DescriptorService(descriptorService),
      m_ShaderIdentifierService(shaderIdentifierService),
      m_InstancesDump(*this),
      m_BindingTablesDump(*this),
      m_CommandListService(commandListService),
      m_RootSignatureService(rootSignatureService),
      m_ResourceStateTracker(resourceStateTracker) {
  LoadInstancesArraysOfPointers();
}

void AnalyzerRaytracingService::CreateStateObject(ID3D12Device5CreateStateObjectCommand& c) {

  std::set<unsigned>& subobjects = m_StateObjectsDirectSubobjects[c.m_ppStateObject.Key];
  for (auto& it : c.m_pDesc.InterfaceKeysBySubobject) {
    subobjects.insert(it.second);
  }

  BindingTablesDump::StateObjectInfo* info = new BindingTablesDump::StateObjectInfo();
  FillStateObjectInfo(c.m_pDesc, info);
  m_StateObjectInfos[c.m_ppStateObject.Key].reset(info);
}

void AnalyzerRaytracingService::AddToStateObject(ID3D12Device7AddToStateObjectCommand& c) {
  std::set<unsigned>& subobjects = m_StateObjectsDirectSubobjects[c.m_ppNewStateObject.Key];
  for (auto& it : c.m_pAddition.InterfaceKeysBySubobject) {
    subobjects.insert(it.second);
  }
  subobjects.insert(c.m_pStateObjectToGrowFrom.Key);

  BindingTablesDump::StateObjectInfo* prevStateObjectInfo =
      m_StateObjectInfos[c.m_pStateObjectToGrowFrom.Key].get();
  GITS_ASSERT(prevStateObjectInfo);

  BindingTablesDump::StateObjectInfo* info = new BindingTablesDump::StateObjectInfo();
  info->GlobalRootSignature = prevStateObjectInfo->GlobalRootSignature;
  info->ExportToRootSignature = prevStateObjectInfo->ExportToRootSignature;

  FillStateObjectInfo(c.m_pAddition, info);

  m_StateObjectInfos[c.m_ppNewStateObject.Key].reset(info);
}

void AnalyzerRaytracingService::FillStateObjectInfo(
    D3D12_STATE_OBJECT_DESC_Argument& stateObjectDesc, BindingTablesDump::StateObjectInfo* info) {

  std::unordered_map<const D3D12_STATE_SUBOBJECT*, unsigned> localSignatures;
  std::unordered_map<std::wstring, std::unordered_set<std::wstring>> hitGroups;
  for (unsigned i = 0; i < stateObjectDesc.Value->NumSubobjects; ++i) {
    switch (stateObjectDesc.Value->pSubobjects[i].Type) {
    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
      info->GlobalRootSignature = stateObjectDesc.InterfaceKeysBySubobject[i];
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      localSignatures[&stateObjectDesc.Value->pSubobjects[i]] =
          stateObjectDesc.InterfaceKeysBySubobject[i];
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& desc =
          *static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(
              const_cast<void*>(stateObjectDesc.Value->pSubobjects[i].pDesc));
      auto it = localSignatures.find(desc.pSubobjectToAssociate);
      if (it != localSignatures.end()) {
        for (unsigned j = 0; j < desc.NumExports; ++j) {
          info->ExportToRootSignature[desc.pExports[j]] = it->second;
        }
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      unsigned stateObjectKey = stateObjectDesc.InterfaceKeysBySubobject[i];
      auto itCollection = m_StateObjectInfos.find(stateObjectKey);
      GITS_ASSERT(itCollection != m_StateObjectInfos.end());
      for (auto& it : itCollection->second->ExportToRootSignature) {
        info->ExportToRootSignature[it.first] = it.second;
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
      D3D12_HIT_GROUP_DESC& desc = *static_cast<D3D12_HIT_GROUP_DESC*>(
          const_cast<void*>(stateObjectDesc.Value->pSubobjects[i].pDesc));
      if (desc.AnyHitShaderImport) {
        hitGroups[desc.AnyHitShaderImport].insert(desc.HitGroupExport);
      }
      if (desc.ClosestHitShaderImport) {
        hitGroups[desc.ClosestHitShaderImport].insert(desc.HitGroupExport);
      }
      if (desc.IntersectionShaderImport) {
        hitGroups[desc.IntersectionShaderImport].insert(desc.HitGroupExport);
      }
    } break;
    }
  }

  std::unordered_map<std::wstring, unsigned> exportToRootSignature;
  for (auto& itExport : info->ExportToRootSignature) {
    auto itGroups = hitGroups.find(itExport.first);
    if (itGroups != hitGroups.end()) {
      for (auto& it : itGroups->second) {
        exportToRootSignature[it] = itExport.second;
      }
    }
  }
  for (auto& it : exportToRootSignature) {
    info->ExportToRootSignature[it.first] = it.second;
  }
}

void AnalyzerRaytracingService::SetPipelineState(
    ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  m_StateObjectByComandList[c.m_Object.Key] = c.m_pStateObject.Key;
}

void AnalyzerRaytracingService::SetDescriptorHeaps(unsigned commandListKey,
                                                   const std::vector<DescriptorHeapInfo>& infos) {
  BindingTablesDump::DescriptorHeaps descriptorHeaps{};
  for (const DescriptorHeapInfo& info : infos) {
    if (info.Key) {
      if (info.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
        descriptorHeaps.ViewDescriptorHeapKey = info.Key;
        descriptorHeaps.ViewDescriptorHeapSize = info.NumDescriptors;
      } else if (info.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
        descriptorHeaps.SamplerHeapKey = info.Key;
        descriptorHeaps.SamplerHeapSize = info.NumDescriptors;
      }
    }
  }
  m_DescriptorHeapsByComandList[commandListKey] = descriptorHeaps;
}

void AnalyzerRaytracingService::BuildTlas(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (!c.m_pDesc.Value->Inputs.NumDescs) {
    return;
  }

  if (c.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
    ID3D12Resource* instances = m_ResourceByKey[c.m_pDesc.InputKeys[0]];
    GITS_ASSERT(instances);
    unsigned offset = c.m_pDesc.InputOffsets[0];
    unsigned size = c.m_pDesc.Value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

    BarrierState currentState = GetAdjustedCurrentState(
        m_ResourceStateTracker, m_GpuAddressService, c.m_Object.Value,
        c.m_pDesc.Value->Inputs.InstanceDescs, instances, c.m_pDesc.InputKeys[0],
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    m_InstancesDump.BuildTlas(c.m_Object.Value, instances, offset, size, currentState, c.Key);

  } else if (c.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
    auto itInstances = m_InstancesArraysOfPointers.find(c.Key);
    GITS_ASSERT(itInstances != m_InstancesArraysOfPointers.end());
    std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& arrayOfPointers = itInstances->second;

    struct InstanceInfo {
      ID3D12Resource* resource{};
      unsigned resourceKey{};
      D3D12_GPU_VIRTUAL_ADDRESS captureStart;
      std::set<unsigned> offsets;
    };
    std::unordered_map<unsigned, InstanceInfo> instancesByResourceKey;

    std::vector<InstanceInfo*> instanceInfos(arrayOfPointers.size());
    for (unsigned i = 0; i < arrayOfPointers.size(); ++i) {
      CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
          m_GpuAddressService.GetResourceInfoByCaptureAddress(arrayOfPointers[i]);
      GITS_ASSERT(resourceInfo);
      auto it = instancesByResourceKey.find(resourceInfo->Key);
      if (it == instancesByResourceKey.end()) {
        instanceInfos[i] = &instancesByResourceKey[resourceInfo->Key];
        instanceInfos[i]->resource = resourceInfo->Resource;
        instanceInfos[i]->resourceKey = resourceInfo->Key;
        instanceInfos[i]->captureStart = resourceInfo->CaptureStart;
      } else {
        instanceInfos[i] = &it->second;
      }
      unsigned offset = arrayOfPointers[i] - resourceInfo->CaptureStart;
      instanceInfos[i]->offsets.insert(offset);
    }

    for (auto& it : instancesByResourceKey) {
      InstanceInfo& instanceInfo = it.second;

      m_CommandListService.AddObjectForRestore(instanceInfo.resourceKey);

      std::vector<unsigned> offsets(instanceInfo.offsets.size());
      std::copy(instanceInfo.offsets.begin(), instanceInfo.offsets.end(), offsets.begin());

      unsigned size = offsets.back() + sizeof(D3D12_RAYTRACING_INSTANCE_DESC) - offsets[0];

      unsigned startOffset = offsets[0];
      for (unsigned i = 0; i < offsets.size(); ++i) {
        offsets[i] -= startOffset;
        offsets[i] /= sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
      }

      BarrierState currentState = GetAdjustedCurrentState(
          m_ResourceStateTracker, m_GpuAddressService, c.m_Object.Value,
          c.m_pDesc.Value->Inputs.InstanceDescs, instanceInfo.resource, c.m_pDesc.InputKeys[0],
          D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

      m_InstancesDump.BuildTlasArrayOfPointers(c.m_Object.Value, instanceInfo.resource, startOffset,
                                               size, currentState, c.Key, offsets);
    }
  }

  m_TlasBuildKeys[{c.m_pDesc.DestAccelerationStructureKey,
                   c.m_pDesc.DestAccelerationStructureOffset}] = c.Key;
}

void AnalyzerRaytracingService::DispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {

  auto dump = [&](unsigned resourceKey, unsigned offset, UINT64 size, UINT64 stride,
                  D3D12_GPU_VIRTUAL_ADDRESS address) {
    if (resourceKey) {
      ID3D12Resource* resource = m_ResourceByKey[resourceKey];
      GITS_ASSERT(resource);
      DumpBindingTable(c.m_Object.Value, c.m_Object.Key, resource, resourceKey, offset, size,
                       stride, address);
    }
  };

  dump(c.m_pDesc.RayGenerationShaderRecordKey, c.m_pDesc.RayGenerationShaderRecordOffset,
       c.m_pDesc.Value->RayGenerationShaderRecord.SizeInBytes,
       c.m_pDesc.Value->RayGenerationShaderRecord.SizeInBytes,
       c.m_pDesc.Value->RayGenerationShaderRecord.StartAddress);
  dump(c.m_pDesc.MissShaderTableKey, c.m_pDesc.MissShaderTableOffset,
       c.m_pDesc.Value->MissShaderTable.SizeInBytes, c.m_pDesc.Value->MissShaderTable.StrideInBytes,
       c.m_pDesc.Value->MissShaderTable.StartAddress);
  dump(c.m_pDesc.HitGroupTableKey, c.m_pDesc.HitGroupTableOffset,
       c.m_pDesc.Value->HitGroupTable.SizeInBytes, c.m_pDesc.Value->HitGroupTable.StrideInBytes,
       c.m_pDesc.Value->HitGroupTable.StartAddress);
  dump(c.m_pDesc.CallableShaderTableKey, c.m_pDesc.CallableShaderTableOffset,
       c.m_pDesc.Value->CallableShaderTable.SizeInBytes,
       c.m_pDesc.Value->CallableShaderTable.StrideInBytes,
       c.m_pDesc.Value->CallableShaderTable.StartAddress);
}

void AnalyzerRaytracingService::DumpBindingTable(ID3D12GraphicsCommandList* commandList,
                                                 unsigned commandListKey,
                                                 ID3D12Resource* resource,
                                                 unsigned resourceKey,
                                                 unsigned offset,
                                                 UINT64 size,
                                                 UINT64 stride,
                                                 D3D12_GPU_VIRTUAL_ADDRESS address) {
  if (stride == 0) {
    stride = size;
  }

  unsigned stateObjectKey = m_StateObjectByComandList[commandListKey];
  GITS_ASSERT(stateObjectKey);
  BindingTablesDump::StateObjectInfo* stateObjectInfo = m_StateObjectInfos[stateObjectKey].get();
  GITS_ASSERT(stateObjectInfo);

  auto itDescriptorHeaps = m_DescriptorHeapsByComandList.find(commandListKey);
  GITS_ASSERT(itDescriptorHeaps != m_DescriptorHeapsByComandList.end());

  unsigned rootSignatureKey = m_CommandListService.GetComputeRootSignatureKey(commandListKey);

  BarrierState currentState = GetAdjustedCurrentState(
      m_ResourceStateTracker, m_GpuAddressService, commandList, address, resource, resourceKey,
      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

  m_BindingTablesDump.DumpBindingTable(commandList, resource, offset, size, stride, currentState,
                                       stateObjectInfo, itDescriptorHeaps->second,
                                       rootSignatureKey);
}

void AnalyzerRaytracingService::Flush() {
  m_InstancesDump.WaitUntilDumped();
  m_BindingTablesDump.WaitUntilDumped();
}

void AnalyzerRaytracingService::ExecuteCommandLists(unsigned key,
                                                    unsigned commandQueueKey,
                                                    ID3D12CommandQueue* commandQueue,
                                                    ID3D12CommandList** commandLists,
                                                    unsigned commandListNum) {
  m_InstancesDump.ExecuteCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                      commandListNum);
  m_BindingTablesDump.ExecuteCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                          commandListNum);
}

void AnalyzerRaytracingService::CommandQueueWait(unsigned key,
                                                 unsigned commandQueueKey,
                                                 unsigned fenceKey,
                                                 UINT64 fenceValue) {
  m_InstancesDump.CommandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
  m_BindingTablesDump.CommandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
}

void AnalyzerRaytracingService::CommandQueueSignal(unsigned key,
                                                   unsigned commandQueueKey,
                                                   unsigned fenceKey,
                                                   UINT64 fenceValue) {
  m_InstancesDump.CommandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
  m_BindingTablesDump.CommandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
}

void AnalyzerRaytracingService::FenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue) {
  m_InstancesDump.FenceSignal(key, fenceKey, fenceValue);
  m_BindingTablesDump.FenceSignal(key, fenceKey, fenceValue);
}

void AnalyzerRaytracingService::GetGPUVirtualAddress(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  m_ResourceByKey[c.m_Object.Key] = c.m_Object.Value;
}

std::set<unsigned> AnalyzerRaytracingService::GetStateObjectAllSubobjects(unsigned stateObjectKey) {

  std::set<unsigned> subobjects;
  // search the graph of connected subobjects
  {
    std::queue<unsigned> subobjectsToProcess;
    {
      std::set<unsigned>& directSubobjects = m_StateObjectsDirectSubobjects[stateObjectKey];
      for (unsigned directSubobjectKey : directSubobjects) {
        subobjectsToProcess.push(directSubobjectKey);
      }
    }

    while (!subobjectsToProcess.empty()) {
      unsigned key = subobjectsToProcess.front();
      subobjectsToProcess.pop();
      subobjects.insert(key);
      std::set<unsigned>& directSubobjects = m_StateObjectsDirectSubobjects[key];
      for (unsigned directSubobjectKey : directSubobjects) {
        if (subobjects.find(directSubobjectKey) == subobjects.end()) {
          subobjectsToProcess.push(directSubobjectKey);
        }
      }
    }
  }

  return subobjects;
}

void AnalyzerRaytracingService::LoadInstancesArraysOfPointers() {
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

unsigned AnalyzerRaytracingService::FindTlas(const KeyOffset& tlas) {
  auto it = m_TlasBuildKeys.find(tlas);
  if (it != m_TlasBuildKeys.end()) {
    return it->second;
  }
  return 0;
}

void AnalyzerRaytracingService::GetTlases(std::set<unsigned>& tlases) {
  for (auto& it : m_TlasBuildKeys) {
    tlases.insert(it.second);
  }
}

} // namespace DirectX
} // namespace gits
