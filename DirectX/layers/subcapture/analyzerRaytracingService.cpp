// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerRaytracingService.h"
#include "bindingService.h"
#include "gits.h"

#include <fstream>

namespace gits {
namespace DirectX {

AnalyzerRaytracingService::AnalyzerRaytracingService(
    DescriptorService& descriptorService,
    CapturePlayerGpuAddressService& gpuAddressService,
    CapturePlayerDescriptorHandleService& descriptorHandleService,
    BindingService& bindingService)
    : gpuAddressService_(gpuAddressService),
      descriptorHandleService_(descriptorHandleService),
      descriptorService_(descriptorService),
      instancesDump_(*this),
      bindingTablesDump_(*this),
      bindingService_(bindingService) {
  loadInstancesArraysOfPointers();
}

void AnalyzerRaytracingService::createStateObject(ID3D12Device5CreateStateObjectCommand& c) {
  std::vector<unsigned>& subobjects = stateObjectsSubobjects_[c.ppStateObject_.key];
  for (auto& it : c.pDesc_.interfaceKeysBySubobject) {
    subobjects.push_back(it.second);
  }
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

      bindingService_.addObjectForRestore(instanceInfo.resourceKey);

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

  tlases_.insert(std::make_pair(c.pDesc_.destAccelerationStructureKey,
                                c.pDesc_.destAccelerationStructureOffset));
}

void AnalyzerRaytracingService::dispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {

  auto dump = [&](unsigned resourceKey, unsigned offset, UINT64 size, UINT64 stride) {
    if (resourceKey) {
      ID3D12Resource* resource = resourceByKey_[resourceKey];
      GITS_ASSERT(resource);
      dumpBindingTable(c.object_.value, resource, resourceKey, offset, size, stride);
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
  bindingTablesDump_.dumpBindingTable(commandList, resource, offset, size, stride, state);
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

} // namespace DirectX
} // namespace gits
