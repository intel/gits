// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingInstancesDump.h"
#include "analyzerRaytracingService.h"
#include "capturePlayerGpuAddressService.h"
#include "gits.h"

namespace gits {
namespace DirectX {

void RaytracingInstancesDump::buildTlas(ID3D12GraphicsCommandList* commandList,
                                        ID3D12Resource* resource,
                                        unsigned offset,
                                        unsigned size,
                                        D3D12_RESOURCE_STATES state,
                                        unsigned buildCall) {
  InstancesInfo* info = new InstancesInfo();
  info->offset = offset;
  info->size = size;
  info->buildCall = buildCall;
  stageResource(commandList, resource, state, *info);
}

void RaytracingInstancesDump::buildTlasArrayOfPointers(
    ID3D12GraphicsCommandList* commandList,
    ID3D12Resource* resource,
    unsigned offset,
    unsigned size,
    D3D12_RESOURCE_STATES state,
    unsigned buildCall,
    std::vector<unsigned>& arrayOfPointersOffsets) {
  InstancesInfo* info = new InstancesInfo();
  info->offset = offset;
  info->size = size;
  info->buildCall = buildCall;
  info->arrayOfPointersOffsets = std::move(arrayOfPointersOffsets);
  stageResource(commandList, resource, state, *info);
}

void RaytracingInstancesDump::dumpBuffer(DumpInfo& dumpInfo, void* data) {
  std::lock_guard<std::mutex> lock(mutex_);

  InstancesInfo& instancesInfo = static_cast<InstancesInfo&>(dumpInfo);
  std::vector<std::pair<unsigned, unsigned>>& blases =
      raytracingService_.getBlasesForTlas(instancesInfo.buildCall);

  D3D12_RAYTRACING_INSTANCE_DESC* instances = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(data);
  unsigned numInstances = instancesInfo.size / sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
  if (instancesInfo.arrayOfPointersOffsets.empty()) {
    for (unsigned i = 0; i < numInstances; ++i) {
      CapturePlayerGpuAddressService::ResourceInfo* info =
          raytracingService_.getGpuAddressService().getResourceInfoByCaptureAddress(
              instances[i].AccelerationStructure);
      if (info) {
        unsigned offset = instances[i].AccelerationStructure - info->captureStart;
        blases.push_back(std::make_pair(info->key, offset));
      }
    }
  } else {
    for (unsigned i = 0; i < instancesInfo.arrayOfPointersOffsets.size(); ++i) {
      GITS_ASSERT(instancesInfo.arrayOfPointersOffsets[i] < numInstances);
      unsigned index = instancesInfo.arrayOfPointersOffsets[i];
      D3D12_GPU_VIRTUAL_ADDRESS blasAddress = instances[index].AccelerationStructure;
      CapturePlayerGpuAddressService::ResourceInfo* info =
          raytracingService_.getGpuAddressService().getResourceInfoByCaptureAddress(blasAddress);
      if (info) {
        unsigned offset = instances[index].AccelerationStructure - info->captureStart;
        blases.push_back(std::make_pair(info->key, offset));
      }
    }
  }
}

} // namespace DirectX
} // namespace gits
