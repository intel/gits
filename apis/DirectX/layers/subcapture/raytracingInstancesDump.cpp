// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingInstancesDump.h"
#include "analyzerRaytracingService.h"
#include "log.h"

namespace gits {
namespace DirectX {

void RaytracingInstancesDump::BuildTlas(ID3D12GraphicsCommandList* commandList,
                                        ID3D12Resource* resource,
                                        unsigned offset,
                                        unsigned size,
                                        BarrierState state,
                                        CommandKey buildCall) {
  InstancesInfo* info = new InstancesInfo();
  info->Offset = offset;
  info->Size = size;
  info->BuildCall = buildCall;
  StageResource(commandList, resource, state, *info);
}

void RaytracingInstancesDump::BuildTlasArrayOfPointers(
    ID3D12GraphicsCommandList* commandList,
    ID3D12Resource* resource,
    unsigned offset,
    unsigned size,
    BarrierState state,
    CommandKey buildCall,
    std::vector<unsigned>& arrayOfPointersOffsets) {
  InstancesInfo* info = new InstancesInfo();
  info->Offset = offset;
  info->Size = size;
  info->BuildCall = buildCall;
  info->ArrayOfPointersOffsets = std::move(arrayOfPointersOffsets);
  StageResource(commandList, resource, state, *info);
}

void RaytracingInstancesDump::DumpBuffer(DumpInfo& dumpInfo, void* data) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  InstancesInfo& instancesInfo = static_cast<InstancesInfo&>(dumpInfo);
  std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& blases =
      m_RaytracingService.GetBlases(instancesInfo.BuildCall);

  D3D12_RAYTRACING_INSTANCE_DESC* instances = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(data);
  unsigned numInstances = instancesInfo.Size / sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
  if (instancesInfo.ArrayOfPointersOffsets.empty()) {
    for (unsigned i = 0; i < numInstances; ++i) {
      blases.push_back(instances[i].AccelerationStructure);
    }
  } else {
    for (unsigned i = 0; i < instancesInfo.ArrayOfPointersOffsets.size(); ++i) {
      GITS_ASSERT(instancesInfo.ArrayOfPointersOffsets[i] < numInstances);
      unsigned index = instancesInfo.ArrayOfPointersOffsets[i];
      D3D12_GPU_VIRTUAL_ADDRESS blasAddress = instances[index].AccelerationStructure;
      blases.push_back(blasAddress);
    }
  }
}

} // namespace DirectX
} // namespace gits
