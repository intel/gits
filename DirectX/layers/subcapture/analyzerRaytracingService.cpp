// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerRaytracingService.h"
#include "analyzerCommandListService.h"
#include "gits.h"
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
    RootSignatureService& rootSignatureService)
    : gpuAddressService_(gpuAddressService),
      descriptorHandleService_(descriptorHandleService),
      descriptorService_(descriptorService),
      shaderIdentifierService_(shaderIdentifierService),
      instancesDump_(*this),
      bindingTablesDump_(*this),
      commandListService_(commandListService),
      rootSignatureService_(rootSignatureService) {
  loadInstancesArraysOfPointers();
}

void AnalyzerRaytracingService::createStateObject(ID3D12Device5CreateStateObjectCommand& c) {

  std::set<unsigned>& subobjects = stateObjectsDirectSubobjects_[c.ppStateObject_.key];
  for (auto& it : c.pDesc_.interfaceKeysBySubobject) {
    subobjects.insert(it.second);
  }

  BindingTablesDump::StateObjectInfo* info = new BindingTablesDump::StateObjectInfo();
  fillStateObjectInfo(c.pDesc_, info);
  stateObjectInfos_[c.ppStateObject_.key].reset(info);
}

void AnalyzerRaytracingService::addToStateObject(ID3D12Device7AddToStateObjectCommand& c) {
  std::set<unsigned>& subobjects = stateObjectsDirectSubobjects_[c.ppNewStateObject_.key];
  for (auto& it : c.pAddition_.interfaceKeysBySubobject) {
    subobjects.insert(it.second);
  }
  subobjects.insert(c.pStateObjectToGrowFrom_.key);

  BindingTablesDump::StateObjectInfo* prevStateObjectInfo =
      stateObjectInfos_[c.pStateObjectToGrowFrom_.key].get();
  GITS_ASSERT(prevStateObjectInfo);

  BindingTablesDump::StateObjectInfo* info = new BindingTablesDump::StateObjectInfo();
  info->globalRootSignature = prevStateObjectInfo->globalRootSignature;
  info->exportToRootSignature = prevStateObjectInfo->exportToRootSignature;

  fillStateObjectInfo(c.pAddition_, info);

  stateObjectInfos_[c.ppNewStateObject_.key].reset(info);
}

void AnalyzerRaytracingService::fillStateObjectInfo(
    D3D12_STATE_OBJECT_DESC_Argument& stateObjectDesc, BindingTablesDump::StateObjectInfo* info) {

  std::unordered_map<const D3D12_STATE_SUBOBJECT*, unsigned> localSignatures;
  std::unordered_map<std::wstring, std::unordered_set<std::wstring>> hitGroups;
  for (unsigned i = 0; i < stateObjectDesc.value->NumSubobjects; ++i) {
    switch (stateObjectDesc.value->pSubobjects[i].Type) {
    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
      info->globalRootSignature = stateObjectDesc.interfaceKeysBySubobject[i];
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      localSignatures[&stateObjectDesc.value->pSubobjects[i]] =
          stateObjectDesc.interfaceKeysBySubobject[i];
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& desc =
          *static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(
              const_cast<void*>(stateObjectDesc.value->pSubobjects[i].pDesc));
      auto it = localSignatures.find(desc.pSubobjectToAssociate);
      if (it != localSignatures.end()) {
        for (unsigned j = 0; j < desc.NumExports; ++j) {
          info->exportToRootSignature[desc.pExports[j]] = it->second;
        }
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      unsigned stateObjectKey = stateObjectDesc.interfaceKeysBySubobject[i];
      auto itCollection = stateObjectInfos_.find(stateObjectKey);
      GITS_ASSERT(itCollection != stateObjectInfos_.end());
      for (auto& it : itCollection->second->exportToRootSignature) {
        info->exportToRootSignature[it.first] = it.second;
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
      D3D12_HIT_GROUP_DESC& desc = *static_cast<D3D12_HIT_GROUP_DESC*>(
          const_cast<void*>(stateObjectDesc.value->pSubobjects[i].pDesc));
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
  for (auto& itExport : info->exportToRootSignature) {
    auto itGroups = hitGroups.find(itExport.first);
    if (itGroups != hitGroups.end()) {
      for (auto& it : itGroups->second) {
        exportToRootSignature[it] = itExport.second;
      }
    }
  }
  for (auto& it : exportToRootSignature) {
    info->exportToRootSignature[it.first] = it.second;
  }
}

void AnalyzerRaytracingService::setPipelineState(
    ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  stateObjectByComandList_[c.object_.key] = c.pStateObject_.key;
}

void AnalyzerRaytracingService::setDescriptorHeaps(
    ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
  BindingTablesDump::DescriptorHeaps descriptorHeaps{};
  for (unsigned i = 0; i < c.NumDescriptorHeaps_.value; ++i) {
    if (c.ppDescriptorHeaps_.value[i]) {
      D3D12_DESCRIPTOR_HEAP_DESC desc = c.ppDescriptorHeaps_.value[i]->GetDesc();
      if (desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
        descriptorHeaps.viewDescriptorHeapKey = c.ppDescriptorHeaps_.keys[i];
        descriptorHeaps.viewDescriptorHeapSize = desc.NumDescriptors;
      } else if (desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
        descriptorHeaps.samplerHeapKey = c.ppDescriptorHeaps_.keys[i];
        descriptorHeaps.samplerHeapSize = desc.NumDescriptors;
      }
    }
  }
  descriptorHeapsByComandList_[c.object_.key] = descriptorHeaps;
}

void AnalyzerRaytracingService::buildTlas(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (!c.pDesc_.value->Inputs.NumDescs) {
    return;
  }

  if (c.pDesc_.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
    ID3D12Resource* instances = resourceByKey_[c.pDesc_.inputKeys[0]];
    GITS_ASSERT(instances);
    unsigned offset = c.pDesc_.inputOffsets[0];
    unsigned size = c.pDesc_.value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if (genericReadResources_.find(c.pDesc_.inputKeys[0]) != genericReadResources_.end()) {
      state = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    // workaround for improper mapping gpu address to buffer in capture
    if (instances->GetDesc().Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) {
      state = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    instancesDump_.buildTlas(c.object_.value, instances, offset, size, state, c.key);

  } else if (c.pDesc_.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
    auto itInstances = instancesArraysOfPointers_.find(c.key);
    GITS_ASSERT(itInstances != instancesArraysOfPointers_.end())
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
          gpuAddressService_.getResourceInfoByCaptureAddress(arrayOfPointers[i]);
      GITS_ASSERT(resourceInfo);
      auto it = instancesByResourceKey.find(resourceInfo->key);
      if (it == instancesByResourceKey.end()) {
        instanceInfos[i] = &instancesByResourceKey[resourceInfo->key];
        instanceInfos[i]->resource = resourceInfo->resource;
        instanceInfos[i]->resourceKey = resourceInfo->key;
        instanceInfos[i]->captureStart = resourceInfo->captureStart;
      } else {
        instanceInfos[i] = &it->second;
      }
      unsigned offset = arrayOfPointers[i] - resourceInfo->captureStart;
      instanceInfos[i]->offsets.insert(offset);
    }

    for (auto& it : instancesByResourceKey) {
      InstanceInfo& instanceInfo = it.second;

      commandListService_.addObjectForRestore(instanceInfo.resourceKey);

      std::vector<unsigned> offsets(instanceInfo.offsets.size());
      std::copy(instanceInfo.offsets.begin(), instanceInfo.offsets.end(), offsets.begin());

      unsigned size = offsets.back() + sizeof(D3D12_RAYTRACING_INSTANCE_DESC) - offsets[0];

      unsigned startOffset = offsets[0];
      for (unsigned i = 0; i < offsets.size(); ++i) {
        offsets[i] -= startOffset;
        offsets[i] /= sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
      }

      D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      if (genericReadResources_.find(instanceInfo.resourceKey) != genericReadResources_.end()) {
        state = D3D12_RESOURCE_STATE_GENERIC_READ;
      }

      // workaround for improper mapping gpu address to buffer in capture
      if (instanceInfo.resource->GetDesc().Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) {
        state = D3D12_RESOURCE_STATE_GENERIC_READ;
      }

      instancesDump_.buildTlasArrayOfPointers(c.object_.value, instanceInfo.resource, startOffset,
                                              size, state, c.key, offsets);
    }
  }

  tlasBuildKeys_[{c.pDesc_.destAccelerationStructureKey,
                  c.pDesc_.destAccelerationStructureOffset}] = c.key;
}

void AnalyzerRaytracingService::dispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {

  auto dump = [&](unsigned resourceKey, unsigned offset, UINT64 size, UINT64 stride) {
    if (resourceKey) {
      ID3D12Resource* resource = resourceByKey_[resourceKey];
      GITS_ASSERT(resource);
      dumpBindingTable(c.object_.value, c.object_.key, resource, resourceKey, offset, size, stride);
    }
  };

  dump(c.pDesc_.rayGenerationShaderRecordKey, c.pDesc_.rayGenerationShaderRecordOffset,
       c.pDesc_.value->RayGenerationShaderRecord.SizeInBytes,
       c.pDesc_.value->RayGenerationShaderRecord.SizeInBytes);
  dump(c.pDesc_.missShaderTableKey, c.pDesc_.missShaderTableOffset,
       c.pDesc_.value->MissShaderTable.SizeInBytes, c.pDesc_.value->MissShaderTable.StrideInBytes);
  dump(c.pDesc_.hitGroupTableKey, c.pDesc_.hitGroupTableOffset,
       c.pDesc_.value->HitGroupTable.SizeInBytes, c.pDesc_.value->HitGroupTable.StrideInBytes);
  dump(c.pDesc_.callableShaderTableKey, c.pDesc_.callableShaderTableOffset,
       c.pDesc_.value->CallableShaderTable.SizeInBytes,
       c.pDesc_.value->CallableShaderTable.StrideInBytes);
}

void AnalyzerRaytracingService::dumpBindingTable(ID3D12GraphicsCommandList* commandList,
                                                 unsigned commandListKey,
                                                 ID3D12Resource* resource,
                                                 unsigned resourceKey,
                                                 unsigned offset,
                                                 UINT64 size,
                                                 UINT64 stride) {
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  if (genericReadResources_.find(resourceKey) != genericReadResources_.end()) {
    state = D3D12_RESOURCE_STATE_GENERIC_READ;
  }

  // workaround for improper mapping gpu address to buffer in capture
  if (resource->GetDesc().Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) {
    state = D3D12_RESOURCE_STATE_GENERIC_READ;
  }

  if (stride == 0) {
    stride = size;
  }

  unsigned stateObjectKey = stateObjectByComandList_[commandListKey];
  GITS_ASSERT(stateObjectKey);
  BindingTablesDump::StateObjectInfo* stateObjectInfo = stateObjectInfos_[stateObjectKey].get();
  GITS_ASSERT(stateObjectInfo);

  auto itDescriptorHeaps = descriptorHeapsByComandList_.find(commandListKey);
  GITS_ASSERT(itDescriptorHeaps != descriptorHeapsByComandList_.end());

  unsigned rootSignatureKey = commandListService_.getComputeRootSignatureKey(commandListKey);

  bindingTablesDump_.dumpBindingTable(commandList, resource, offset, size, stride, state,
                                      stateObjectInfo, itDescriptorHeaps->second, rootSignatureKey);
}

void AnalyzerRaytracingService::flush() {
  instancesDump_.waitUntilDumped();
  bindingTablesDump_.waitUntilDumped();
}

void AnalyzerRaytracingService::executeCommandLists(unsigned key,
                                                    unsigned commandQueueKey,
                                                    ID3D12CommandQueue* commandQueue,
                                                    ID3D12CommandList** commandLists,
                                                    unsigned commandListNum) {
  instancesDump_.executeCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                     commandListNum);
  bindingTablesDump_.executeCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                         commandListNum);
}

void AnalyzerRaytracingService::commandQueueWait(unsigned key,
                                                 unsigned commandQueueKey,
                                                 unsigned fenceKey,
                                                 UINT64 fenceValue) {
  instancesDump_.commandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
  bindingTablesDump_.commandQueueWait(key, commandQueueKey, fenceKey, fenceValue);
}

void AnalyzerRaytracingService::commandQueueSignal(unsigned key,
                                                   unsigned commandQueueKey,
                                                   unsigned fenceKey,
                                                   UINT64 fenceValue) {
  instancesDump_.commandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
  bindingTablesDump_.commandQueueSignal(key, commandQueueKey, fenceKey, fenceValue);
}

void AnalyzerRaytracingService::fenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue) {
  instancesDump_.fenceSignal(key, fenceKey, fenceValue);
  bindingTablesDump_.fenceSignal(key, fenceKey, fenceValue);
}

void AnalyzerRaytracingService::getGPUVirtualAddress(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  resourceByKey_[c.object_.key] = c.object_.value;
}

std::set<unsigned> AnalyzerRaytracingService::getStateObjectAllSubobjects(unsigned stateObjectKey) {

  std::set<unsigned> subobjects;
  // search the graph of connected subobjects
  {
    std::queue<unsigned> subobjectsToProcess;
    {
      std::set<unsigned>& directSubobjects = stateObjectsDirectSubobjects_[stateObjectKey];
      for (unsigned directSubobjectKey : directSubobjects) {
        subobjectsToProcess.push(directSubobjectKey);
      }
    }

    while (!subobjectsToProcess.empty()) {
      unsigned key = subobjectsToProcess.front();
      subobjectsToProcess.pop();
      subobjects.insert(key);
      std::set<unsigned>& directSubobjects = stateObjectsDirectSubobjects_[key];
      for (unsigned directSubobjectKey : directSubobjects) {
        if (subobjects.find(directSubobjectKey) == subobjects.end()) {
          subobjectsToProcess.push(directSubobjectKey);
        }
      }
    }
  }

  return subobjects;
}

void AnalyzerRaytracingService::loadInstancesArraysOfPointers() {
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

unsigned AnalyzerRaytracingService::findTlas(KeyOffset& tlas) {
  auto it = tlasBuildKeys_.find(tlas);
  if (it != tlasBuildKeys_.end()) {
    return it->second;
  }
  return 0;
}

void AnalyzerRaytracingService::getTlases(std::set<unsigned>& tlases) {
  for (auto& it : tlasBuildKeys_) {
    tlases.insert(it.second);
  }
}

} // namespace DirectX
} // namespace gits
