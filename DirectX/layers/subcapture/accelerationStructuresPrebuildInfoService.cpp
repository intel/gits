// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresPrebuildInfoService.h"
#include "stateTrackingService.h"
#include "analyzerResults.h"
#include "commandSerializersCustom.h"
#include "log.h"

namespace gits {
namespace DirectX {
void AccelerationStructuresPrebuildInfoService::AddPrebuildInfo(
    ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& c) {
  if (!c.m_pDesc.Value || !c.m_pInfo.Value) {
    return;
  }
  const auto existing = m_PrebuildInfoEntries.find(c.m_pDesc);
  if (existing != m_PrebuildInfoEntries.end()) {
    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& existingInfo =
        existing->second.Info;
    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& info = *c.m_pInfo.Value;
    GITS_ASSERT(existingInfo.ResultDataMaxSizeInBytes == info.ResultDataMaxSizeInBytes &&
                    existingInfo.ScratchDataSizeInBytes == info.ScratchDataSizeInBytes &&
                    existingInfo.UpdateScratchDataSizeInBytes == info.UpdateScratchDataSizeInBytes,
                std::string("existing prebuild command key: ") +
                    std::to_string(existing->second.PrebuildCommandKey) +
                    ", current prebuild command key: " + std::to_string(c.Key));
    if (m_StateService.GetAnalyzerResults().RestorePrebuildInfo(c.Key)) {
      existing->second.PrebuildCommandKey = c.Key;
    }
    return;
  }

  m_PrebuildInfoEntries.emplace(c.m_pDesc, PrebuildInfoEntry{c.Key, *c.m_pInfo.Value});
}

std::optional<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO>
AccelerationStructuresPrebuildInfoService::GetPrebuildInfoForInputs(
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs) const {
  const auto prebuildInfoIt = m_PrebuildInfoEntries.find(&inputs);
  if (prebuildInfoIt != m_PrebuildInfoEntries.end()) {
    return prebuildInfoIt->second.Info;
  }
  return std::nullopt;
}

void AccelerationStructuresPrebuildInfoService::RestorePrebuildInfos() {
  const ObjectKey deviceKey = m_StateService.GetDeviceKey();
  for (auto& [inputs, entry] : m_PrebuildInfoEntries) {
    if (!m_StateService.GetAnalyzerResults().RestorePrebuildInfo(entry.PrebuildCommandKey)) {
      continue;
    }
    PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS> prebuildDescInputs(
        inputs);
    StripRaytracingBuildInputs(prebuildDescInputs);

    ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand prebuild;
    prebuild.Key = m_StateService.GetUniqueCommandKey();
    prebuild.m_Object.Key = deviceKey;
    prebuild.m_pDesc.Value = prebuildDescInputs.Value;
    prebuild.m_pDesc.InputKeys = prebuildDescInputs.InputKeys;
    prebuild.m_pDesc.InputOffsets = prebuildDescInputs.InputOffsets;
    prebuild.m_pInfo.Value = &entry.Info;
    m_Recorder.Record(
        ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoSerializer(prebuild));
  }

  if (m_DummyResourceKey) {
    IUnknownReleaseCommand releaseDummyResource{};
    releaseDummyResource.Key = m_StateService.GetUniqueCommandKey();
    releaseDummyResource.m_Object.Key = m_DummyResourceKey;
    m_Recorder.Record(IUnknownReleaseSerializer(releaseDummyResource));
    m_DummyResourceKey = {};
  }
}

void AccelerationStructuresPrebuildInfoService::RecordDummyResource() {
  m_DummyResourceKey = m_StateService.GetUniqueObjectKey();

  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
  heapProperties.CreationNodeMask = 1;
  heapProperties.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Width = sizeof(float) * 3 * 4;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ID3D12DeviceCreateCommittedResourceCommand createResource;
  createResource.Key = m_StateService.GetUniqueCommandKey();
  createResource.m_Object.Key = m_StateService.GetDeviceKey();
  createResource.m_pHeapProperties.Value = &heapProperties;
  createResource.m_HeapFlags.Value = D3D12_HEAP_FLAG_NONE;
  createResource.m_pDesc.Value = &resourceDesc;
  createResource.m_InitialResourceState.Value = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  createResource.m_pOptimizedClearValue.Value = nullptr;
  createResource.m_riidResource.Value = IID_ID3D12Resource;
  createResource.m_ppvResource.Key = m_DummyResourceKey;
  m_Recorder.Record(ID3D12DeviceCreateCommittedResourceSerializer(createResource));
}

void AccelerationStructuresPrebuildInfoService::StripRaytracingBuildInputs(
    PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& inputArgument) {
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = *inputArgument.Value;
  const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS> inputsBeforeStrip(
      inputArgument);

  const size_t inputKeysCountBeforeStrip = inputArgument.InputKeys.size();
  const size_t inputOffsetsCountBeforeStrip = inputArgument.InputOffsets.size();
  inputArgument.InputKeys.clear();
  inputArgument.InputOffsets.clear();

  auto stripGpuVirtualAddress = [&inputArgument](D3D12_GPU_VIRTUAL_ADDRESS& address) {
    address = 0;
    inputArgument.InputKeys.push_back(ObjectKey{});
    inputArgument.InputOffsets.push_back(0);
  };
  auto stripOptionalGpuVirtualAddressInUse = [this,
                                              &inputArgument](D3D12_GPU_VIRTUAL_ADDRESS& address) {
    if (address) {
      if (!m_DummyResourceKey) {
        RecordDummyResource();
      }
      address = 1;
      inputArgument.InputKeys.push_back(m_DummyResourceKey);
    } else {
      inputArgument.InputKeys.push_back(ObjectKey{});
    }
    inputArgument.InputOffsets.push_back(0);
  };
  auto stripBufferStride = [&stripGpuVirtualAddress](D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE& buffer) {
    stripGpuVirtualAddress(buffer.StartAddress);
    buffer.StrideInBytes = 0;
  };

  const auto stripTriangles = [&stripOptionalGpuVirtualAddressInUse, &stripGpuVirtualAddress,
                               &stripBufferStride](D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& desc) {
    stripOptionalGpuVirtualAddressInUse(desc.Transform3x4);
    stripGpuVirtualAddress(desc.IndexBuffer);
    stripBufferStride(desc.VertexBuffer);
  };

  const auto stripOmmLinkage = [&stripGpuVirtualAddress, &stripBufferStride](
                                   D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC& desc) {
    stripBufferStride(desc.OpacityMicromapIndexBuffer);
    stripGpuVirtualAddress(desc.OpacityMicromapArray);
  };

  const auto stripGeometry = [&stripTriangles, &stripOmmLinkage,
                              &stripBufferStride](D3D12_RAYTRACING_GEOMETRY_DESC& desc) {
    switch (desc.Type) {
    case D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES:
      stripTriangles(desc.Triangles);
      break;
    case D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS:
      stripBufferStride(desc.AABBs.AABBs);
      break;
    case D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES:
      if (desc.OmmTriangles.pTriangles) {
        stripTriangles(
            *const_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(desc.OmmTriangles.pTriangles));
      }
      if (desc.OmmTriangles.pOmmLinkage) {
        stripOmmLinkage(*const_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(
            desc.OmmTriangles.pOmmLinkage));
      }
      break;
    default:
      break;
    }
  };

  inputs.Flags &= ~D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

  if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    stripGpuVirtualAddress(inputs.InstanceDescs);
  } else if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < inputs.NumDescs; ++i) {
      if (inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        if (inputs.ppGeometryDescs && inputs.ppGeometryDescs[i]) {
          stripGeometry(*const_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(inputs.ppGeometryDescs[i]));
        }
      } else if (inputs.pGeometryDescs) {
        stripGeometry(const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(inputs.pGeometryDescs[i]));
      }
    }
  } else if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY &&
             inputs.pOpacityMicromapArrayDesc) {
    D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC& desc =
        *const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
            inputs.pOpacityMicromapArrayDesc);
    stripGpuVirtualAddress(desc.InputBuffer);
    stripBufferStride(desc.PerOmmDescs);
  }

  GITS_ASSERT(inputArgument.InputKeys.size() == inputKeysCountBeforeStrip);
  GITS_ASSERT(inputArgument.InputOffsets.size() == inputOffsetsCountBeforeStrip);
  GITS_ASSERT(RaytracingBuildInputsEqual{}(inputArgument, inputsBeforeStrip));
}

} // namespace DirectX
} // namespace gits
