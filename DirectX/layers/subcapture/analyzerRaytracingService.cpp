// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerRaytracingService.h"
#include "gits.h"

namespace gits {
namespace DirectX {

void AnalyzerRaytracingService::createStateObject(ID3D12Device5CreateStateObjectCommand& c) {
  std::vector<unsigned>& subobjects = stateObjectsSubobjects_[c.ppStateObject_.key];
  for (auto& it : c.pDesc_.interfaceKeysBySubobject) {
    subobjects.push_back(it.second);
  }
}

void AnalyzerRaytracingService::buildTlas(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (!c.pDesc_.value->Inputs.NumDescs ||
      c.pDesc_.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
    return;
  }
  ID3D12Resource* instances = resourceByKey_[c.pDesc_.inputKeys[0]];
  GITS_ASSERT(instances);
  unsigned offset = c.pDesc_.inputOffsets[0];
  unsigned size = c.pDesc_.value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  if (genericReadResources_.find(c.pDesc_.inputKeys[0]) != genericReadResources_.end()) {
    state = D3D12_RESOURCE_STATE_GENERIC_READ;
  }
  instancesDump_.buildTlas(c.object_.value, instances, offset, size, state, c.key);
  tlases_.insert(std::make_pair(c.pDesc_.destAccelerationStructureKey,
                                c.pDesc_.destAccelerationStructureOffset));
}

void AnalyzerRaytracingService::dispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {

  auto dumpBindingTable = [&](unsigned resourceKey, unsigned offset, UINT64 size, UINT64 stride) {
    if (resourceKey) {
      ID3D12Resource* resource = resourceByKey_[resourceKey];
      GITS_ASSERT(resource);
      D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      if (genericReadResources_.find(resourceKey) != genericReadResources_.end()) {
        state = D3D12_RESOURCE_STATE_GENERIC_READ;
      }
      if (stride == 0) {
        stride = size;
      }
      bindingTablesDump_.dumpBindingTable(c.object_.value, resource, offset, size, stride, state,
                                          c.key);
    }
  };

  dumpBindingTable(c.pDesc_.rayGenerationShaderRecordKey, c.pDesc_.rayGenerationShaderRecordOffset,
                   c.pDesc_.value->RayGenerationShaderRecord.SizeInBytes,
                   c.pDesc_.value->RayGenerationShaderRecord.SizeInBytes);
  dumpBindingTable(c.pDesc_.missShaderTableKey, c.pDesc_.missShaderTableOffset,
                   c.pDesc_.value->MissShaderTable.SizeInBytes,
                   c.pDesc_.value->MissShaderTable.StrideInBytes);
  dumpBindingTable(c.pDesc_.hitGroupTableKey, c.pDesc_.hitGroupTableOffset,
                   c.pDesc_.value->HitGroupTable.SizeInBytes,
                   c.pDesc_.value->HitGroupTable.StrideInBytes);
  dumpBindingTable(c.pDesc_.callableShaderTableKey, c.pDesc_.callableShaderTableOffset,
                   c.pDesc_.value->CallableShaderTable.SizeInBytes,
                   c.pDesc_.value->CallableShaderTable.StrideInBytes);
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

void AnalyzerRaytracingService::captureGPUVirtualAddress(
    ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  resourceByKey_[c.object_.key] = c.object_.value;
  D3D12_RESOURCE_DESC desc = c.object_.value->GetDesc();
  gpuAddressService_.addGpuCaptureAddress(c.object_.value, c.object_.key, desc.Width,
                                          c.result_.value);
}

void AnalyzerRaytracingService::playerGPUVirtualAddress(
    ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  D3D12_RESOURCE_DESC desc = c.object_.value->GetDesc();
  gpuAddressService_.addGpuPlayerAddress(c.object_.value, c.object_.key, desc.Width,
                                         c.result_.value);
}

void AnalyzerRaytracingService::captureGPUDescriptorHandle(
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  descriptorHandleService_.addCaptureHandle(c.object_.value, c.object_.key, c.result_.value);
}

void AnalyzerRaytracingService::playerGPUDescriptorHandle(
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  descriptorHandleService_.addPlayerHandle(c.object_.key, c.result_.value);
}

} // namespace DirectX
} // namespace gits
