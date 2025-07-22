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

void RaytracingInstancesDump::dumpBuffer(DumpInfo& dumpInfo, void* data) {
  std::lock_guard<std::mutex> lock(mutex_);

  InstancesInfo& instancesInfo = static_cast<InstancesInfo&>(dumpInfo);
  std::vector<std::pair<unsigned, unsigned>>& blases =
      raytracingService_.getBlasesForTlas(instancesInfo.buildCall);

  unsigned numInstances = instancesInfo.size / sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
  D3D12_RAYTRACING_INSTANCE_DESC* instances = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(data);
  for (unsigned i = 0; i < numInstances; ++i) {
    CapturePlayerGpuAddressService::ResourceInfo* info =
        raytracingService_.getGpuAddressService().getResourceInfoByCaptureAddress(
            instances[i].AccelerationStructure);
    if (info) {
      unsigned offset = instances[i].AccelerationStructure - info->captureStart;
      blases.push_back(std::make_pair(info->key, offset));
    }
  }
}

} // namespace DirectX
} // namespace gits
