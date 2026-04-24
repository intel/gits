// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingInstancesDump.h"
#include "analyzerRaytracingService.h"
#include "capturePlayerGpuAddressService.h"
#include "log.h"

namespace gits {
namespace DirectX {

void RaytracingInstancesDump::BuildTlas(ID3D12GraphicsCommandList* commandList,
                                        ID3D12Resource* resource,
                                        unsigned offset,
                                        unsigned size,
                                        D3D12_RESOURCE_STATES state,
                                        unsigned buildCall) {
  InstancesInfo* info = new InstancesInfo();
  info->offset = offset;
  info->size = size;
  info->BuildCall = buildCall;
  stageResource(commandList, resource, state, *info);
}

void RaytracingInstancesDump::BuildTlasArrayOfPointers(
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
  info->BuildCall = buildCall;
  info->ArrayOfPointersOffsets = std::move(arrayOfPointersOffsets);
  stageResource(commandList, resource, state, *info);
}

void RaytracingInstancesDump::dumpBuffer(DumpInfo& dumpInfo, void* data) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  InstancesInfo& instancesInfo = static_cast<InstancesInfo&>(dumpInfo);
  std::vector<std::pair<unsigned, unsigned>>& blases =
      m_RaytracingService.GetBlases(instancesInfo.BuildCall);

  D3D12_RAYTRACING_INSTANCE_DESC* instances = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(data);
  unsigned numInstances = instancesInfo.size / sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
  if (instancesInfo.ArrayOfPointersOffsets.empty()) {
    for (unsigned i = 0; i < numInstances; ++i) {
      CapturePlayerGpuAddressService::ResourceInfo* info =
          m_RaytracingService.GetGpuAddressService().GetResourceInfoByCaptureAddress(
              instances[i].AccelerationStructure);
      if (info) {
        unsigned offset = instances[i].AccelerationStructure - info->CaptureStart;
        blases.push_back(std::make_pair(info->Key, offset));
      }
    }
  } else {
    for (unsigned i = 0; i < instancesInfo.ArrayOfPointersOffsets.size(); ++i) {
      GITS_ASSERT(instancesInfo.ArrayOfPointersOffsets[i] < numInstances);
      unsigned index = instancesInfo.ArrayOfPointersOffsets[i];
      D3D12_GPU_VIRTUAL_ADDRESS blasAddress = instances[index].AccelerationStructure;
      CapturePlayerGpuAddressService::ResourceInfo* info =
          m_RaytracingService.GetGpuAddressService().GetResourceInfoByCaptureAddress(blasAddress);
      if (info) {
        unsigned offset = instances[index].AccelerationStructure - info->CaptureStart;
        blases.push_back(std::make_pair(info->Key, offset));
      }
    }
  }
}

} // namespace DirectX
} // namespace gits
