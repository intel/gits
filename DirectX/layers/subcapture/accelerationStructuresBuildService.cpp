// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresBuildService.h"
#include "stateTrackingService.h"
#include "arguments.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
#include "reservedResourcesService.h"
#include "log.h"
#include "configurationLib.h"
#include "nvapi.h"

#include <algorithm>

namespace gits {
namespace DirectX {

AccelerationStructuresBuildService::AccelerationStructuresBuildService(
    StateTrackingService& stateService,
    SubcaptureRecorder& recorder,
    ReservedResourcesService& reservedResourcesService,
    ResourceStateTracker& resourceStateTracker,
    CapturePlayerGpuAddressService& gpuAddressService)
    : m_StateService(stateService),
      m_Recorder(recorder),
      m_ReservedResourcesService(reservedResourcesService),
      m_BufferContentRestore(stateService),
      m_ResourceStateTracker(resourceStateTracker),
      m_GpuAddressService(gpuAddressService) {
  m_SerializeMode = Configurator::Get().directx.features.subcapture.serializeAccelerationStructures;
  m_RestoreTlas = Configurator::Get().directx.features.subcapture.restoreTLASes;
  m_Optimize = Configurator::Get().directx.features.subcapture.optimize;
}

void AccelerationStructuresBuildService::BuildAccelerationStructure(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (m_SerializeMode &&
      c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    return;
  }
  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    if (m_Optimize) {
      if (!m_StateService.GetAnalyzerResults().RestoreTlas(c.Key)) {
        return;
      }
    } else {
      if (!m_Recorder.CommandListSubcapture() && !m_RestoreTlas) {
        return;
      }
    }
  }
  if (m_Restored) {
    return;
  }

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = c.m_pDesc.Value->Inputs;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info{};
  Microsoft::WRL::ComPtr<ID3D12Device5> device;
  HRESULT hr = c.m_Object.Value->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
  if (info.ScratchDataSizeInBytes > m_MaxBuildScratchSpace) {
    m_MaxBuildScratchSpace = info.ScratchDataSizeInBytes;
  }
  if (info.UpdateScratchDataSizeInBytes > m_MaxBuildScratchSpace) {
    m_MaxBuildScratchSpace = info.UpdateScratchDataSizeInBytes;
  }

  BuildRaytracingAccelerationStructureState* state =
      new BuildRaytracingAccelerationStructureState();
  state->CommandKey = c.Key;
  state->CommandListKey = m_CommandListDirectKey;
  state->Kind = RaytracingAccelerationStructureState::StateKind::Build;
  state->DestKey = c.m_pDesc.DestAccelerationStructureKey;
  state->DestOffset = c.m_pDesc.DestAccelerationStructureOffset;
  state->SourceKey = c.m_pDesc.SourceAccelerationStructureKey;
  state->SourceOffset = c.m_pDesc.SourceAccelerationStructureOffset;
  state->Update = c.m_pDesc.Value->Inputs.Flags &
                  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

  if (m_SerializeMode &&
      c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    m_Tlases.insert(std::make_pair(state->DestKey, state->DestOffset));
  }

  state->Desc.reset(
      new PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>(c.m_pDesc));

  m_StateService.KeepState(c.m_pDesc.DestAccelerationStructureKey);

  std::unordered_map<unsigned, std::vector<Interval>> intervalsByInputKey;
  auto addBufferAccess = [&](unsigned inputIndex, unsigned size) {
    unsigned inputKey = c.m_pDesc.InputKeys[inputIndex];
    unsigned inputOffset = c.m_pDesc.InputOffsets[inputIndex];

    auto& intervals = intervalsByInputKey[inputKey];
    intervals.push_back({inputOffset, inputOffset + size});
  };

  unsigned inputIndex = 0;
  if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL &&
      inputs.InstanceDescs) {
    unsigned size = inputs.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
    if (inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      size = inputs.NumDescs * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
    }
    addBufferAccess(inputIndex, size);
  } else if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          c.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? c.m_pDesc.Value->Inputs.pGeometryDescs[i]
              : *c.m_pDesc.Value->Inputs.ppGeometryDescs[i]);
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        if (desc.Triangles.Transform3x4) {
          unsigned size = sizeof(float) * 3 * 4;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.Triangles.IndexBuffer && desc.Triangles.IndexCount) {
          unsigned size = desc.Triangles.IndexCount *
                          (desc.Triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.Triangles.VertexBuffer.StartAddress && desc.Triangles.VertexCount) {
          unsigned stride = desc.Triangles.VertexBuffer.StrideInBytes;
          if (!stride) {
            if (desc.Triangles.VertexFormat == DXGI_FORMAT_R16G16B16A16_SNORM) {
              stride = 8;
            }
          }
          unsigned size = desc.Triangles.VertexCount * stride;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        if (desc.AABBs.AABBs.StartAddress && desc.AABBs.AABBCount) {
          unsigned size = desc.AABBs.AABBCount * desc.AABBs.AABBs.StrideInBytes;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
        if (desc.OmmTriangles.pTriangles) {
          auto& triangles = *desc.OmmTriangles.pTriangles;
          if (triangles.Transform3x4) {
            unsigned size = sizeof(float) * 3 * 4;
            addBufferAccess(inputIndex, size);
          }
          ++inputIndex;
          if (triangles.IndexBuffer && triangles.IndexCount) {
            unsigned size =
                triangles.IndexCount * (triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
            addBufferAccess(inputIndex, size);
          }
          ++inputIndex;
          if (triangles.VertexBuffer.StartAddress && triangles.VertexCount) {
            unsigned stride = triangles.VertexBuffer.StrideInBytes;
            if (!stride) {
              if (triangles.VertexFormat == DXGI_FORMAT_R16G16B16A16_SNORM) {
                stride = 8;
              }
            }
            unsigned size = triangles.VertexCount * stride;
            addBufferAccess(inputIndex, size);
          }
          ++inputIndex;
        }
        if (desc.OmmTriangles.pOmmLinkage) {
          auto& ommLinkage = *desc.OmmTriangles.pOmmLinkage;
          if (ommLinkage.OpacityMicromapIndexBuffer.StartAddress) {
            unsigned stride = ommLinkage.OpacityMicromapIndexBuffer.StrideInBytes;
            unsigned formatSize = 0;
            if (!stride) {
              if (ommLinkage.OpacityMicromapIndexFormat == DXGI_FORMAT_R32_UINT) {
                stride = 4;
                formatSize = 4;
              } else if (ommLinkage.OpacityMicromapIndexFormat == DXGI_FORMAT_R16_UINT) {
                stride = 2;
                formatSize = 2;
              } else if (ommLinkage.OpacityMicromapIndexFormat == DXGI_FORMAT_R8_UINT) {
                stride = 1;
                formatSize = 1;
              }
            } else {
              if (ommLinkage.OpacityMicromapIndexFormat == DXGI_FORMAT_R32_UINT) {
                formatSize = 4;
              } else if (ommLinkage.OpacityMicromapIndexFormat == DXGI_FORMAT_R16_UINT) {
                formatSize = 2;
              } else if (ommLinkage.OpacityMicromapIndexFormat == DXGI_FORMAT_R8_UINT) {
                formatSize = 1;
              }
            }
            GITS_ASSERT(stride);
            GITS_ASSERT(formatSize);
            GITS_ASSERT(ommLinkage.OpacityMicromapIndexBuffer.StartAddress % formatSize == 0);
            GITS_ASSERT(stride % formatSize == 0);
            unsigned ommCount{};
            if (desc.OmmTriangles.pTriangles && desc.OmmTriangles.pTriangles->IndexCount > 0) {
              ommCount = desc.OmmTriangles.pTriangles->IndexCount / 3;
            }
            unsigned size = ommCount * stride;
            if (size) {
              addBufferAccess(inputIndex, size);
            }
          }
          ++inputIndex;
          ++inputIndex;
        }
      }
    }
  } else if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (inputs.pOpacityMicromapArrayDesc) {
      auto& ommDesc = *const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
          inputs.pOpacityMicromapArrayDesc);
      UINT totalOmmCount{};
      size_t totalInputSize{};
      for (unsigned i = 0; i < ommDesc.NumOmmHistogramEntries; ++i) {
        const auto& histogramEntry = ommDesc.pOmmHistogram[i];
        totalOmmCount += histogramEntry.Count;
        size_t numMicroTriangles = 1ull << (2 * histogramEntry.SubdivisionLevel);
        size_t bitsPerTriangle{};
        if (histogramEntry.Format == D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT_OC1_4_STATE) {
          bitsPerTriangle = 2;
        } else if (histogramEntry.Format == D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT_OC1_2_STATE) {
          bitsPerTriangle = 1;
        }
        GITS_ASSERT(bitsPerTriangle);
        size_t bitsPerOMM = numMicroTriangles * bitsPerTriangle;
        size_t bytesPerOMM = (bitsPerOMM + 7) / 8;
        totalInputSize += histogramEntry.Count * bytesPerOMM;
      }
      GITS_ASSERT(totalOmmCount);
      GITS_ASSERT(totalInputSize);

      if (ommDesc.InputBuffer) {
        GITS_ASSERT(ommDesc.InputBuffer % 128 == 0);
        addBufferAccess(inputIndex, totalInputSize);
      }
      ++inputIndex;
      if (ommDesc.PerOmmDescs.StartAddress) {
        unsigned stride = ommDesc.PerOmmDescs.StrideInBytes;
        if (!stride) {
          stride = sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_DESC);
        }
        GITS_ASSERT(ommDesc.PerOmmDescs.StartAddress % 4 == 0);
        GITS_ASSERT(stride % 4 == 0);
        unsigned size = totalOmmCount * stride;
        addBufferAccess(inputIndex, size);
      }
      ++inputIndex;
    }
  }

  for (const auto& [inputKey, intervals] : intervalsByInputKey) {
    std::vector<Interval> merged = MergeIntervals(intervals);
    for (Interval interval : merged) {
      StoreBuffer(inputKey, interval.Start, interval.End - interval.Start, c.Key, c.m_Object.Value,
                  state);
    }
  }

  m_StatesByCommandList[c.m_Object.Key].emplace_back(state);
}

void AccelerationStructuresBuildService::CopyAccelerationStructure(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (m_SerializeMode) {
    auto it = m_Tlases.find(std::make_pair(c.m_DestAccelerationStructureData.InterfaceKey,
                                           c.m_DestAccelerationStructureData.Offset));
    if (it == m_Tlases.end()) {
      return;
    }
  }
  CopyRaytracingAccelerationStructureState* state = new CopyRaytracingAccelerationStructureState();
  state->CommandKey = c.Key;
  state->CommandListKey = m_CommandListDirectKey;
  state->Kind = RaytracingAccelerationStructureState::StateKind::Copy;
  state->DestAccelerationStructureData = c.m_DestAccelerationStructureData.Value;
  state->DestKey = c.m_DestAccelerationStructureData.InterfaceKey;
  state->DestOffset = c.m_DestAccelerationStructureData.Offset;
  state->SourceAccelerationStructureData = c.m_SourceAccelerationStructureData.Value;
  state->SourceKey = c.m_SourceAccelerationStructureData.InterfaceKey;
  state->SourceOffset = c.m_SourceAccelerationStructureData.Offset;
  state->Mode = c.m_Mode.Value;

  m_StateService.KeepState(c.m_DestAccelerationStructureData.InterfaceKey);

  m_StatesByCommandList[c.m_Object.Key].emplace_back(state);
}

void AccelerationStructuresBuildService::NvapiBuildAccelerationStructureEx(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  const NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX* pDesc =
      c.m_pParams.Value->pDesc;
  if (!m_RestoreTlas &&
      pDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }

  if (m_Restored) {
    return;
  }

  NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS_EX inputs = pDesc->inputs;

  Microsoft::WRL::ComPtr<ID3D12Device5> device;
  HRESULT hr = c.m_pCommandList.Value->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  NVAPI_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_EX_PARAMS params{};
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info{};
  params.version = NVAPI_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_EX_PARAMS_VER;
  params.pDesc = &inputs;
  params.pInfo = &info;
  NvAPI_D3D12_GetRaytracingAccelerationStructurePrebuildInfoEx(device.Get(), &params);
  if (info.ScratchDataSizeInBytes > m_MaxBuildScratchSpace) {
    m_MaxBuildScratchSpace = info.ScratchDataSizeInBytes;
  }
  if (info.UpdateScratchDataSizeInBytes > m_MaxBuildScratchSpace) {
    m_MaxBuildScratchSpace = info.UpdateScratchDataSizeInBytes;
  }

  NvAPIBuildRaytracingAccelerationStructureExState* state =
      new NvAPIBuildRaytracingAccelerationStructureExState();
  state->CommandKey = c.Key;
  state->CommandListKey = m_CommandListDirectKey;
  state->Kind = RaytracingAccelerationStructureState::StateKind::NvAPIBuild;
  state->DestKey = c.m_pParams.DestAccelerationStructureKey;
  state->DestOffset = c.m_pParams.DestAccelerationStructureOffset;
  state->SourceKey = c.m_pParams.SourceAccelerationStructureKey;
  state->SourceOffset = c.m_pParams.SourceAccelerationStructureOffset;
  state->Update = pDesc->inputs.flags &
                  NVAPI_D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE_EX;

  if (m_SerializeMode &&
      pDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    m_Tlases.insert(std::make_pair(state->DestKey, state->DestOffset));
  }

  state->Desc.reset(
      new PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>(c.m_pParams));

  m_StateService.KeepState(c.m_pParams.DestAccelerationStructureKey);

  std::unordered_map<unsigned, std::vector<Interval>> intervalsByInputKey;
  auto addBufferAccess = [&](unsigned inputIndex, unsigned size) {
    unsigned inputKey = c.m_pParams.InputKeys[inputIndex];
    unsigned inputOffset = c.m_pParams.InputOffsets[inputIndex];

    auto& intervals = intervalsByInputKey[inputKey];
    intervals.push_back({inputOffset, inputOffset + size});
  };

  unsigned inputIndex = 0;
  if (inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL && inputs.numDescs) {
    if (inputs.numDescs) {
      unsigned size = inputs.numDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
      addBufferAccess(inputIndex, size);
    }
  } else {
    for (unsigned i = 0; i < inputs.numDescs; ++i) {
      const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
          pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? *(const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(pDesc->inputs
                                                                              .pGeometryDescs) +
                                                                  pDesc->inputs
                                                                          .geometryDescStrideInBytes *
                                                                      i)
              : *pDesc->inputs.ppGeometryDescs[i];
      if (desc.type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        if (desc.triangles.Transform3x4) {
          unsigned size = sizeof(float) * 3 * 4;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.triangles.IndexBuffer && desc.triangles.IndexCount) {
          unsigned size = desc.triangles.IndexCount *
                          (desc.triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.triangles.VertexBuffer.StartAddress && desc.triangles.VertexCount) {
          unsigned stride = desc.triangles.VertexBuffer.StrideInBytes;
          if (!stride) {
            if (desc.triangles.VertexFormat == DXGI_FORMAT_R16G16B16A16_SNORM) {
              stride = 8;
            }
          }
          unsigned size = desc.triangles.VertexCount * stride;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
      } else if (desc.type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        if (desc.aabbs.AABBs.StartAddress && desc.aabbs.AABBCount) {
          unsigned size = desc.aabbs.AABBCount * desc.aabbs.AABBs.StrideInBytes;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
        if (desc.ommTriangles.triangles.Transform3x4) {
          unsigned size = sizeof(float) * 3 * 4;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.ommTriangles.triangles.IndexBuffer && desc.ommTriangles.triangles.IndexCount) {
          unsigned size = desc.ommTriangles.triangles.IndexCount *
                          (desc.ommTriangles.triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.ommTriangles.triangles.VertexBuffer.StartAddress &&
            desc.ommTriangles.triangles.VertexCount) {
          unsigned stride = desc.ommTriangles.triangles.VertexBuffer.StrideInBytes;
          if (!stride) {
            if (desc.ommTriangles.triangles.VertexFormat == DXGI_FORMAT_R16G16B16A16_SNORM) {
              stride = 8;
            }
          }
          unsigned size = desc.ommTriangles.triangles.VertexCount * stride;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.ommTriangles.ommAttachment.opacityMicromapIndexBuffer.StartAddress) {
          unsigned stride =
              desc.ommTriangles.ommAttachment.opacityMicromapIndexBuffer.StrideInBytes;
          if (!stride) {
            if (desc.ommTriangles.ommAttachment.opacityMicromapIndexFormat ==
                DXGI_FORMAT_R32_UINT) {
              stride = 4;
            } else if (desc.ommTriangles.ommAttachment.opacityMicromapIndexFormat ==
                       DXGI_FORMAT_R16_UINT) {
              stride = 2;
            }
          }
          GITS_ASSERT(stride);
          unsigned ommCount = desc.ommTriangles.triangles.IndexCount / 3;
          unsigned size = ommCount * stride;
          if (size) {
            addBufferAccess(inputIndex, size);
          }
        }
        ++inputIndex;
        ++inputIndex;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
        if (desc.dmmTriangles.triangles.Transform3x4) {
          unsigned size = sizeof(float) * 3 * 4;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.dmmTriangles.triangles.IndexBuffer && desc.dmmTriangles.triangles.IndexCount) {
          unsigned size = desc.dmmTriangles.triangles.IndexCount *
                          (desc.dmmTriangles.triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.dmmTriangles.triangles.VertexBuffer.StartAddress &&
            desc.dmmTriangles.triangles.VertexCount) {
          unsigned stride = desc.dmmTriangles.triangles.VertexBuffer.StrideInBytes;
          if (!stride) {
            if (desc.dmmTriangles.triangles.VertexFormat == DXGI_FORMAT_R16G16B16A16_SNORM) {
              stride = 8;
            }
          }
          unsigned size = desc.dmmTriangles.triangles.VertexCount * stride;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.dmmTriangles.dmmAttachment.triangleMicromapIndexBuffer.StartAddress) {
          unsigned stride =
              desc.dmmTriangles.dmmAttachment.triangleMicromapIndexBuffer.StrideInBytes;
          if (!stride) {
            if (desc.dmmTriangles.dmmAttachment.triangleMicromapIndexFormat ==
                DXGI_FORMAT_R32_UINT) {
              stride = 4;
            } else if (desc.dmmTriangles.dmmAttachment.triangleMicromapIndexFormat ==
                       DXGI_FORMAT_R16_UINT) {
              stride = 2;
            }
          }
          GITS_ASSERT(stride);
          unsigned dmmCount{};
          for (unsigned j = 0; j < desc.dmmTriangles.dmmAttachment.numDMMUsageCounts; ++j) {
            dmmCount += desc.dmmTriangles.dmmAttachment.pDMMUsageCounts[j].count;
          }
          if (!dmmCount) {
            dmmCount = desc.dmmTriangles.triangles.IndexCount / 3;
          }
          unsigned size = dmmCount * stride;
          if (size) {
            addBufferAccess(inputIndex, size);
          }
        }
        ++inputIndex;
        if (desc.dmmTriangles.dmmAttachment.trianglePrimitiveFlagsBuffer.StartAddress) {
          unsigned size = desc.dmmTriangles.triangles.VertexCount;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.dmmTriangles.dmmAttachment.vertexBiasAndScaleBuffer.StartAddress) {
          unsigned stride = desc.dmmTriangles.dmmAttachment.vertexBiasAndScaleBuffer.StrideInBytes;
          if (!stride) {
            if (desc.dmmTriangles.dmmAttachment.vertexBiasAndScaleFormat ==
                DXGI_FORMAT_R32G32_FLOAT) {
              stride = 8;
            } else if (desc.dmmTriangles.dmmAttachment.vertexBiasAndScaleFormat ==
                       DXGI_FORMAT_R16G16_FLOAT) {
              stride = 4;
            }
          }
          GITS_ASSERT(stride);
          unsigned size = desc.dmmTriangles.triangles.VertexCount * stride;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.dmmTriangles.dmmAttachment.vertexDisplacementVectorBuffer.StartAddress) {
          unsigned stride =
              desc.dmmTriangles.dmmAttachment.vertexDisplacementVectorBuffer.StrideInBytes;
          if (!stride) {
            // The Alpha channel is ignored
            if (desc.dmmTriangles.dmmAttachment.vertexDisplacementVectorFormat ==
                    DXGI_FORMAT_R32G32B32A32_FLOAT ||
                desc.dmmTriangles.dmmAttachment.vertexDisplacementVectorFormat ==
                    DXGI_FORMAT_R32G32B32_FLOAT) {
              stride = 12;
            } else if (desc.dmmTriangles.dmmAttachment.vertexDisplacementVectorFormat ==
                       DXGI_FORMAT_R16G16B16A16_FLOAT) {
              stride = 6;
            }
          }
          GITS_ASSERT(stride);
          unsigned size = desc.dmmTriangles.triangles.VertexCount * stride;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        ++inputIndex;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_SPHERES_EX) {
        if (desc.spheres.vertexPositionBuffer.StartAddress) {
          unsigned stride = desc.spheres.vertexPositionBuffer.StrideInBytes;
          // Supports the same formats as the triangle vertex buffers
          if (!stride) {
            if (desc.spheres.vertexPositionFormat == DXGI_FORMAT_R16G16B16A16_SNORM) {
              stride = 8;
            }
          }
          GITS_ASSERT(stride);
          unsigned size = desc.spheres.vertexCount * stride;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.spheres.vertexRadiusBuffer.StartAddress) {
          unsigned stride{};
          if (desc.spheres.vertexRadiusFormat == DXGI_FORMAT_R32_FLOAT) {
            stride = 4;
          } else if (desc.spheres.vertexRadiusFormat == DXGI_FORMAT_R16_FLOAT) {
            stride = 2;
          }
          GITS_ASSERT(stride);
          unsigned size = desc.spheres.vertexCount * stride;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.spheres.indexBuffer.StartAddress) {
          unsigned stride = desc.spheres.indexBuffer.StrideInBytes;
          if (!stride) {
            if (desc.spheres.indexFormat == DXGI_FORMAT_R32_UINT) {
              stride = 4;
            } else if (desc.spheres.indexFormat == DXGI_FORMAT_R16_UINT) {
              stride = 2;
            } else if (desc.spheres.indexFormat == DXGI_FORMAT_R8_UINT) {
              stride = 1;
            }
          }
          GITS_ASSERT(stride);
          unsigned size = desc.spheres.indexCount * stride;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_LSS_EX) {
        if (desc.lss.vertexPositionBuffer.StartAddress) {
          unsigned stride = desc.lss.vertexPositionBuffer.StrideInBytes;
          // Supports the same formats as the triangle vertex buffers
          if (!stride) {
            if (desc.lss.vertexPositionFormat == DXGI_FORMAT_R16G16B16A16_SNORM) {
              stride = 8;
            }
          }
          GITS_ASSERT(stride);
          unsigned size = desc.lss.primitiveCount * stride;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.lss.vertexRadiusBuffer.StartAddress) {
          unsigned stride{};
          if (desc.lss.vertexRadiusFormat == DXGI_FORMAT_R32_FLOAT) {
            stride = 4;
          } else if (desc.lss.vertexRadiusFormat == DXGI_FORMAT_R16_FLOAT) {
            stride = 2;
          }
          GITS_ASSERT(stride);
          unsigned size = desc.lss.primitiveCount * stride;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
        if (desc.lss.indexBuffer.StartAddress) {
          unsigned stride = desc.lss.indexBuffer.StrideInBytes;
          if (!stride) {
            if (desc.lss.indexFormat == DXGI_FORMAT_R32_UINT) {
              stride = 4;
            } else if (desc.lss.indexFormat == DXGI_FORMAT_R16_UINT) {
              stride = 2;
            } else if (desc.lss.indexFormat == DXGI_FORMAT_R8_UINT) {
              stride = 1;
            }
          }
          GITS_ASSERT(stride);
          unsigned size = desc.lss.indexCount * stride;
          addBufferAccess(inputIndex, size);
        }
        ++inputIndex;
      }
    }
  }

  for (const auto& [inputKey, intervals] : intervalsByInputKey) {
    std::vector<Interval> merged = MergeIntervals(intervals);
    for (Interval interval : merged) {
      StoreBuffer(inputKey, interval.Start, interval.End - interval.Start, c.Key,
                  c.m_pCommandList.Value, state);
    }
  }

  m_StatesByCommandList[c.m_pCommandList.Key].emplace_back(state);
}

void AccelerationStructuresBuildService::NvapiBuildOpacityMicromapArray(
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (m_Restored) {
    return;
  }

  const NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC* pDesc = c.m_pParams.Value->pDesc;

  NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_INPUTS inputs = pDesc->inputs;

  Microsoft::WRL::ComPtr<ID3D12Device5> device;
  HRESULT hr = c.m_pCommandList.Value->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  NVAPI_GET_RAYTRACING_OPACITY_MICROMAP_ARRAY_PREBUILD_INFO_PARAMS params{};
  NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_PREBUILD_INFO info{};
  params.version = NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS_VER;
  params.pDesc = &inputs;
  params.pInfo = &info;
  NvAPI_D3D12_GetRaytracingOpacityMicromapArrayPrebuildInfo(device.Get(), &params);
  if (info.scratchDataSizeInBytes > m_MaxBuildScratchSpace) {
    m_MaxBuildScratchSpace = info.scratchDataSizeInBytes;
  }

  NvAPIBuildRaytracingOpacityMicromapArrayState* state =
      new NvAPIBuildRaytracingOpacityMicromapArrayState();
  state->CommandKey = c.Key;
  state->CommandListKey = m_CommandListDirectKey;
  state->Kind = RaytracingAccelerationStructureState::StateKind::NvAPIOMM;
  state->DestKey = c.m_pParams.DestOpacityMicromapArrayDataKey;
  state->DestOffset = c.m_pParams.DestOpacityMicromapArrayDataOffset;

  state->Desc.reset(
      new PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>(c.m_pParams));

  m_StateService.KeepState(c.m_pParams.DestOpacityMicromapArrayDataKey);

  std::unordered_map<unsigned, std::vector<Interval>> intervalsByInputKey;
  auto addBufferAccess = [&](unsigned inputKey, unsigned inputOffset, unsigned size) {
    auto& intervals = intervalsByInputKey[inputKey];
    intervals.push_back({inputOffset, inputOffset + size});
  };

  {
    unsigned ommCount{};
    size_t inputSize{};
    for (unsigned i = 0; i < pDesc->inputs.numOMMUsageCounts; ++i) {
      const auto& usage = pDesc->inputs.pOMMUsageCounts[i];
      ommCount += usage.count;

      unsigned numMicroTriangles = 1u << (2 * usage.subdivisionLevel);
      unsigned bitsPerTriangle{};
      if (usage.format == NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT_OC1_4_STATE) {
        bitsPerTriangle = 2;
      } else if (usage.format == NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT_OC1_2_STATE) {
        bitsPerTriangle = 1;
      }
      GITS_ASSERT(bitsPerTriangle);

      unsigned bitsPerOMM = numMicroTriangles * bitsPerTriangle;
      unsigned bytesPerOMM = (bitsPerOMM + 7) / 8; // round up to full byte

      inputSize += usage.count * bytesPerOMM;
    }

    auto alignUp = [](size_t value, size_t alignment) {
      return (value + alignment - 1) & ~(alignment - 1);
    };
    inputSize = alignUp(inputSize, 256);

    if (pDesc->inputs.inputBuffer) {
      addBufferAccess(c.m_pParams.InputBufferKey, c.m_pParams.InputBufferOffset, inputSize);
    }
    if (pDesc->inputs.perOMMDescs.StartAddress) {
      unsigned stride = pDesc->inputs.perOMMDescs.StrideInBytes;
      if (!stride) {
        stride = sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT);
      }
      addBufferAccess(c.m_pParams.PerOMMDescsKey, c.m_pParams.PerOMMDescsOffset, stride * ommCount);
    }
  }

  for (const auto& [inputKey, intervals] : intervalsByInputKey) {
    std::vector<Interval> merged = MergeIntervals(intervals);
    for (Interval interval : merged) {
      StoreBuffer(inputKey, interval.Start, interval.End - interval.Start, c.Key,
                  c.m_pCommandList.Value, state);
    }
  }

  m_StatesByCommandList[c.m_pCommandList.Key].emplace_back(state);
}

void AccelerationStructuresBuildService::RestoreAccelerationStructures() {
  if (m_StatesById.empty()) {
    return;
  }

  CompleteSourcesFromAnalysis();
  RemoveSourcesWithoutDestinations();
  Optimize();

  m_BufferContentRestore.waitUntilDumped();

  InitUploadBuffer();

  {
    m_CommandQueueCopyKey = m_StateService.GetUniqueObjectKey();
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    ID3D12DeviceCreateCommandQueueCommand CreateCommandQueue;
    CreateCommandQueue.Key = m_StateService.GetUniqueCommandKey();
    CreateCommandQueue.m_Object.Key = m_DeviceKey;
    CreateCommandQueue.m_pDesc.Value = &commandQueueDesc;
    CreateCommandQueue.m_riid.Value = IID_ID3D12CommandQueue;
    CreateCommandQueue.m_ppCommandQueue.Key = m_CommandQueueCopyKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandQueueSerializer(CreateCommandQueue));

    m_CommandAllocatorCopyKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
    createCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
    createCommandAllocator.m_Object.Key = m_DeviceKey;
    createCommandAllocator.m_type.Value = D3D12_COMMAND_LIST_TYPE_COPY;
    createCommandAllocator.m_riid.Value = IID_ID3D12CommandAllocator;
    createCommandAllocator.m_ppCommandAllocator.Key = m_CommandAllocatorCopyKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandAllocatorSerializer(createCommandAllocator));

    m_CommandListCopyKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandListCommand createCommandList;
    createCommandList.Key = m_StateService.GetUniqueCommandKey();
    createCommandList.m_Object.Key = m_DeviceKey;
    createCommandList.m_nodeMask.Value = 0;
    createCommandList.m_pCommandAllocator.Key = createCommandAllocator.m_ppCommandAllocator.Key;
    createCommandList.m_type.Value = D3D12_COMMAND_LIST_TYPE_COPY;
    createCommandList.m_pInitialState.Value = nullptr;
    createCommandList.m_riid.Value = IID_ID3D12CommandList;
    createCommandList.m_ppCommandList.Key = m_CommandListCopyKey;
    m_StateService.GetRecorder().Record(ID3D12DeviceCreateCommandListSerializer(createCommandList));
  }
  {
    m_CommandQueueDirectKey = m_StateService.GetUniqueObjectKey();
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ID3D12DeviceCreateCommandQueueCommand CreateCommandQueue;
    CreateCommandQueue.Key = m_StateService.GetUniqueCommandKey();
    CreateCommandQueue.m_Object.Key = m_DeviceKey;
    CreateCommandQueue.m_pDesc.Value = &commandQueueDesc;
    CreateCommandQueue.m_riid.Value = IID_ID3D12CommandQueue;
    CreateCommandQueue.m_ppCommandQueue.Key = m_CommandQueueDirectKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandQueueSerializer(CreateCommandQueue));

    m_CommandAllocatorDirectKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
    createCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
    createCommandAllocator.m_Object.Key = m_DeviceKey;
    createCommandAllocator.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
    createCommandAllocator.m_riid.Value = IID_ID3D12CommandAllocator;
    createCommandAllocator.m_ppCommandAllocator.Key = m_CommandAllocatorDirectKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandAllocatorSerializer(createCommandAllocator));

    m_CommandListDirectKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandListCommand createCommandList;
    createCommandList.Key = m_StateService.GetUniqueCommandKey();
    createCommandList.m_Object.Key = m_DeviceKey;
    createCommandList.m_nodeMask.Value = 0;
    createCommandList.m_pCommandAllocator.Key = createCommandAllocator.m_ppCommandAllocator.Key;
    createCommandList.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
    createCommandList.m_pInitialState.Value = nullptr;
    createCommandList.m_riid.Value = IID_ID3D12CommandList;
    createCommandList.m_ppCommandList.Key = m_CommandListDirectKey;
    m_StateService.GetRecorder().Record(ID3D12DeviceCreateCommandListSerializer(createCommandList));
  }
  {
    m_FenceKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateFenceCommand createFence;
    createFence.Key = m_StateService.GetUniqueCommandKey();
    createFence.m_Object.Key = m_DeviceKey;
    createFence.m_InitialValue.Value = 0;
    createFence.m_Flags.Value = D3D12_FENCE_FLAG_NONE;
    createFence.m_riid.Value = IID_ID3D12Fence;
    createFence.m_ppFence.Key = m_FenceKey;
    m_StateService.GetRecorder().Record(ID3D12DeviceCreateFenceSerializer(createFence));
  }

  unsigned scratchResourceKey = m_StateService.GetUniqueObjectKey();

  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
  heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProperties.CreationNodeMask = 1;
  heapProperties.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = m_MaxBuildScratchSpace;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  ID3D12DeviceCreateCommittedResourceCommand createScratch;
  createScratch.Key = m_StateService.GetUniqueCommandKey();
  createScratch.m_Object.Key = m_DeviceKey;
  createScratch.m_pHeapProperties.Value = &heapProperties;
  createScratch.m_HeapFlags.Value = D3D12_HEAP_FLAG_NONE;
  createScratch.m_pDesc.Value = &resourceDesc;
  createScratch.m_InitialResourceState.Value = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  createScratch.m_pOptimizedClearValue.Value = nullptr;
  createScratch.m_riidResource.Value = IID_ID3D12Resource;
  createScratch.m_ppvResource.Key = scratchResourceKey;
  m_Recorder.Record(ID3D12DeviceCreateCommittedResourceSerializer(createScratch));

  ID3D12ResourceGetGPUVirtualAddressCommand getAddress{};
  getAddress.Key = m_StateService.GetUniqueCommandKey();
  getAddress.m_Object.Key = scratchResourceKey;
  m_Recorder.Record(ID3D12ResourceGetGPUVirtualAddressSerializer(getAddress));

  std::map<std::pair<unsigned, unsigned>, uint64_t> bufferHashesByKeyOffset;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> tiledResourceUpdatesRestored;
  for (auto& itState : m_StatesById) {
    if (itState.second->Kind == RaytracingAccelerationStructureState::StateKind::Build) {
      BuildRaytracingAccelerationStructureState* state =
          static_cast<BuildRaytracingAccelerationStructureState*>(itState.second);

      std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
          m_BufferContentRestore.GetRestoreInfos(state->CommandKey);

      std::set<unsigned> residencyKeys;
      for (const auto& info : restoreInfos) {
        InsertIfNotResident(info.BufferKey, residencyKeys);
      }
      InsertIfNotResident(state->Desc->DestAccelerationStructureKey, residencyKeys);
      InsertIfNotResident(state->Desc->SourceAccelerationStructureKey, residencyKeys);
      RecordMakeResident(residencyKeys);

      std::unordered_set<unsigned> restoredBuffers;
      size_t uploadBufferOffset{};
      for (AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
        auto itHash = bufferHashesByKeyOffset.find(std::pair(info.BufferKey, info.Offset));
        if (itHash != bufferHashesByKeyOffset.end() && itHash->second == info.BufferHash) {
          continue;
        }
        bufferHashesByKeyOffset[std::pair(info.BufferKey, info.Offset)] = info.BufferHash;
        restoredBuffers.insert(info.BufferKey);

        for (auto& itTiledResource : state->TiledResources) {
          auto it = tiledResourceUpdatesRestored.find(info.BufferKey);
          if (it == tiledResourceUpdatesRestored.end() ||
              it->second.find(itTiledResource.second.UpdateId) == it->second.end()) {
            m_ReservedResourcesService.UpdateTileMappings(itTiledResource.second,
                                                          m_CommandQueueCopyKey, nullptr);
            tiledResourceUpdatesRestored[info.BufferKey].insert(itTiledResource.second.UpdateId);
          }
        }

        uploadBufferOffset += RestoreBuffer(info, uploadBufferOffset);
      }

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.Key = m_StateService.GetUniqueCommandKey();
        commandListClose.m_Object.Key = m_CommandListCopyKey;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListCloseSerializer(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand ExecuteCommandLists;
        ExecuteCommandLists.Key = m_StateService.GetUniqueCommandKey();
        ExecuteCommandLists.m_Object.Key = m_CommandQueueCopyKey;
        ExecuteCommandLists.m_NumCommandLists.Value = 1;
        ExecuteCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
        ExecuteCommandLists.m_ppCommandLists.Size = 1;
        ExecuteCommandLists.m_ppCommandLists.Keys.resize(1);
        ExecuteCommandLists.m_ppCommandLists.Keys[0] = m_CommandListCopyKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandQueueExecuteCommandListsSerializer(ExecuteCommandLists));

        ID3D12CommandQueueSignalCommand CommandQueueSignal;
        CommandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
        CommandQueueSignal.m_Object.Key = m_CommandQueueCopyKey;
        CommandQueueSignal.m_pFence.Key = m_FenceKey;
        CommandQueueSignal.m_Value.Value = ++m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(CommandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.Key = m_StateService.GetUniqueCommandKey();
        getCompletedValue.m_Object.Key = m_FenceKey;
        getCompletedValue.m_Result.Value = m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(
            ID3D12FenceGetCompletedValueSerializer(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.Key = m_StateService.GetUniqueCommandKey();
        commandAllocatorReset.m_Object.Key = m_CommandAllocatorCopyKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandAllocatorResetSerializer(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand CommandListReset;
        CommandListReset.Key = m_StateService.GetUniqueCommandKey();
        CommandListReset.m_Object.Key = m_CommandListCopyKey;
        CommandListReset.m_pAllocator.Key = m_CommandAllocatorCopyKey;
        CommandListReset.m_pInitialState.Key = 0;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListResetSerializer(CommandListReset));
      }

      for (auto& it : state->Buffers) {
        if (!it.second->IsMappable && restoredBuffers.find(it.first) != restoredBuffers.end()) {
          ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
          barrierCommand.Key = m_StateService.GetUniqueCommandKey();
          barrierCommand.m_Object.Key = m_CommandListDirectKey;
          barrierCommand.m_NumBarriers.Value = 1;
          D3D12_RESOURCE_BARRIER barrier{};
          barrierCommand.m_pBarriers.Value = &barrier;
          barrierCommand.m_pBarriers.Size = 1;
          barrierCommand.m_pBarriers.ResourceKeys.resize(1);
          barrierCommand.m_pBarriers.ResourceAfterKeys.resize(1);
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
          barrier.Transition.StateAfter = it.second->CurrentState;
          barrierCommand.m_pBarriers.ResourceKeys[0] = it.first;
          m_StateService.GetRecorder().Record(
              ID3D12GraphicsCommandListResourceBarrierSerializer(barrierCommand));
        }
      }

      state->Desc->ScratchAccelerationStructureKey = scratchResourceKey;
      state->Desc->ScratchAccelerationStructureOffset = 0;

      {
        ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand build;
        build.Key = state->CommandKey;
        build.m_Object.Key = m_CommandListDirectKey;
        build.m_pDesc.Value = state->Desc->Value;
        build.m_pDesc.DestAccelerationStructureKey = state->Desc->DestAccelerationStructureKey;
        build.m_pDesc.DestAccelerationStructureOffset =
            state->Desc->DestAccelerationStructureOffset;
        build.m_pDesc.SourceAccelerationStructureKey = state->Desc->SourceAccelerationStructureKey;
        build.m_pDesc.SourceAccelerationStructureOffset =
            state->Desc->SourceAccelerationStructureOffset;
        build.m_pDesc.ScratchAccelerationStructureKey =
            state->Desc->ScratchAccelerationStructureKey;
        build.m_pDesc.ScratchAccelerationStructureOffset =
            state->Desc->ScratchAccelerationStructureOffset;
        build.m_pDesc.InputKeys = state->Desc->InputKeys;
        build.m_pDesc.InputOffsets = state->Desc->InputOffsets;
        build.m_NumPostbuildInfoDescs.Value = 0;
        build.m_pPostbuildInfoDescs.Value = nullptr;
        m_Recorder.Record(
            ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureSerializer(build));
      }

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.Key = m_StateService.GetUniqueCommandKey();
        commandListClose.m_Object.Key = m_CommandListDirectKey;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListCloseSerializer(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand ExecuteCommandLists;
        ExecuteCommandLists.Key = m_StateService.GetUniqueCommandKey();
        ExecuteCommandLists.m_Object.Key = m_CommandQueueDirectKey;
        ExecuteCommandLists.m_NumCommandLists.Value = 1;
        ExecuteCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
        ExecuteCommandLists.m_ppCommandLists.Size = 1;
        ExecuteCommandLists.m_ppCommandLists.Keys.resize(1);
        ExecuteCommandLists.m_ppCommandLists.Keys[0] = m_CommandListDirectKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandQueueExecuteCommandListsSerializer(ExecuteCommandLists));

        ID3D12CommandQueueSignalCommand CommandQueueSignal;
        CommandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
        CommandQueueSignal.m_Object.Key = m_CommandQueueDirectKey;
        CommandQueueSignal.m_pFence.Key = m_FenceKey;
        CommandQueueSignal.m_Value.Value = ++m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(CommandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.Key = m_StateService.GetUniqueCommandKey();
        getCompletedValue.m_Object.Key = m_FenceKey;
        getCompletedValue.m_Result.Value = m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(
            ID3D12FenceGetCompletedValueSerializer(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.Key = m_StateService.GetUniqueCommandKey();
        commandAllocatorReset.m_Object.Key = m_CommandAllocatorDirectKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandAllocatorResetSerializer(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand CommandListReset;
        CommandListReset.Key = m_StateService.GetUniqueCommandKey();
        CommandListReset.m_Object.Key = m_CommandListDirectKey;
        CommandListReset.m_pAllocator.Key = m_CommandAllocatorDirectKey;
        CommandListReset.m_pInitialState.Key = 0;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListResetSerializer(CommandListReset));
      }
      RecordEvict(residencyKeys);
    } else if (itState.second->Kind == RaytracingAccelerationStructureState::StateKind::Copy) {
      CopyRaytracingAccelerationStructureState* state =
          static_cast<CopyRaytracingAccelerationStructureState*>(itState.second);

      std::set<unsigned> residencyKeys;
      InsertIfNotResident(state->DestKey, residencyKeys);
      InsertIfNotResident(state->SourceKey, residencyKeys);
      RecordMakeResident(residencyKeys);

      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand copy;
      copy.Key = state->CommandKey;
      copy.m_Object.Key = m_CommandListDirectKey;
      copy.m_DestAccelerationStructureData.Value = state->DestAccelerationStructureData;
      copy.m_DestAccelerationStructureData.InterfaceKey = state->DestKey;
      copy.m_DestAccelerationStructureData.Offset = state->DestOffset;
      copy.m_SourceAccelerationStructureData.Value = state->SourceAccelerationStructureData;
      copy.m_SourceAccelerationStructureData.InterfaceKey = state->SourceKey;
      copy.m_SourceAccelerationStructureData.Offset = state->SourceOffset;
      copy.m_Mode.Value = state->Mode;
      m_StateService.GetRecorder().Record(
          ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureSerializer(copy));

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.Key = m_StateService.GetUniqueCommandKey();
        commandListClose.m_Object.Key = m_CommandListDirectKey;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListCloseSerializer(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand ExecuteCommandLists;
        ExecuteCommandLists.Key = m_StateService.GetUniqueCommandKey();
        ExecuteCommandLists.m_Object.Key = m_CommandQueueDirectKey;
        ExecuteCommandLists.m_NumCommandLists.Value = 1;
        ExecuteCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
        ExecuteCommandLists.m_ppCommandLists.Size = 1;
        ExecuteCommandLists.m_ppCommandLists.Keys.resize(1);
        ExecuteCommandLists.m_ppCommandLists.Keys[0] = m_CommandListDirectKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandQueueExecuteCommandListsSerializer(ExecuteCommandLists));

        ID3D12CommandQueueSignalCommand CommandQueueSignal;
        CommandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
        CommandQueueSignal.m_Object.Key = m_CommandQueueDirectKey;
        CommandQueueSignal.m_pFence.Key = m_FenceKey;
        CommandQueueSignal.m_Value.Value = ++m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(CommandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.Key = m_StateService.GetUniqueCommandKey();
        getCompletedValue.m_Object.Key = m_FenceKey;
        getCompletedValue.m_Result.Value = m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(
            ID3D12FenceGetCompletedValueSerializer(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.Key = m_StateService.GetUniqueCommandKey();
        commandAllocatorReset.m_Object.Key = m_CommandAllocatorDirectKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandAllocatorResetSerializer(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand CommandListReset;
        CommandListReset.Key = m_StateService.GetUniqueCommandKey();
        CommandListReset.m_Object.Key = m_CommandListDirectKey;
        CommandListReset.m_pAllocator.Key = m_CommandAllocatorDirectKey;
        CommandListReset.m_pInitialState.Key = 0;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListResetSerializer(CommandListReset));
      }
      RecordEvict(residencyKeys);
    } else if (itState.second->Kind ==
               RaytracingAccelerationStructureState::StateKind::NvAPIBuild) {
      NvAPIBuildRaytracingAccelerationStructureExState* state =
          static_cast<NvAPIBuildRaytracingAccelerationStructureExState*>(itState.second);

      std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
          m_BufferContentRestore.GetRestoreInfos(state->CommandKey);

      std::set<unsigned> residencyKeys;
      for (const auto& info : restoreInfos) {
        InsertIfNotResident(info.BufferKey, residencyKeys);
      }
      InsertIfNotResident(state->Desc->DestAccelerationStructureKey, residencyKeys);
      InsertIfNotResident(state->Desc->SourceAccelerationStructureKey, residencyKeys);
      RecordMakeResident(residencyKeys);

      std::unordered_set<unsigned> restoredBuffers;
      size_t uploadBufferOffset{};
      for (AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
        auto itHash = bufferHashesByKeyOffset.find(std::pair(info.BufferKey, info.Offset));
        if (itHash != bufferHashesByKeyOffset.end() && itHash->second == info.BufferHash) {
          continue;
        }
        bufferHashesByKeyOffset[std::pair(info.BufferKey, info.Offset)] = info.BufferHash;
        restoredBuffers.insert(info.BufferKey);

        for (auto& itTiledResource : state->TiledResources) {
          auto it = tiledResourceUpdatesRestored.find(info.BufferKey);
          if (it == tiledResourceUpdatesRestored.end() ||
              it->second.find(itTiledResource.second.UpdateId) == it->second.end()) {
            m_ReservedResourcesService.UpdateTileMappings(itTiledResource.second,
                                                          m_CommandQueueCopyKey, nullptr);
            tiledResourceUpdatesRestored[info.BufferKey].insert(itTiledResource.second.UpdateId);
          }
        }

        uploadBufferOffset += RestoreBuffer(info, uploadBufferOffset);
      }

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.Key = m_StateService.GetUniqueCommandKey();
        commandListClose.m_Object.Key = m_CommandListCopyKey;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListCloseSerializer(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand ExecuteCommandLists;
        ExecuteCommandLists.Key = m_StateService.GetUniqueCommandKey();
        ExecuteCommandLists.m_Object.Key = m_CommandQueueCopyKey;
        ExecuteCommandLists.m_NumCommandLists.Value = 1;
        ExecuteCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
        ExecuteCommandLists.m_ppCommandLists.Size = 1;
        ExecuteCommandLists.m_ppCommandLists.Keys.resize(1);
        ExecuteCommandLists.m_ppCommandLists.Keys[0] = m_CommandListCopyKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandQueueExecuteCommandListsSerializer(ExecuteCommandLists));

        ID3D12CommandQueueSignalCommand CommandQueueSignal;
        CommandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
        CommandQueueSignal.m_Object.Key = m_CommandQueueCopyKey;
        CommandQueueSignal.m_pFence.Key = m_FenceKey;
        CommandQueueSignal.m_Value.Value = ++m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(CommandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.Key = m_StateService.GetUniqueCommandKey();
        getCompletedValue.m_Object.Key = m_FenceKey;
        getCompletedValue.m_Result.Value = m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(
            ID3D12FenceGetCompletedValueSerializer(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.Key = m_StateService.GetUniqueCommandKey();
        commandAllocatorReset.m_Object.Key = m_CommandAllocatorCopyKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandAllocatorResetSerializer(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand CommandListReset;
        CommandListReset.Key = m_StateService.GetUniqueCommandKey();
        CommandListReset.m_Object.Key = m_CommandListCopyKey;
        CommandListReset.m_pAllocator.Key = m_CommandAllocatorCopyKey;
        CommandListReset.m_pInitialState.Key = 0;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListResetSerializer(CommandListReset));
      }

      for (auto& it : state->Buffers) {
        if (!it.second->IsMappable && restoredBuffers.find(it.first) != restoredBuffers.end()) {
          ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
          barrierCommand.Key = m_StateService.GetUniqueCommandKey();
          barrierCommand.m_Object.Key = m_CommandListDirectKey;
          barrierCommand.m_NumBarriers.Value = 1;
          D3D12_RESOURCE_BARRIER barrier{};
          barrierCommand.m_pBarriers.Value = &barrier;
          barrierCommand.m_pBarriers.Size = 1;
          barrierCommand.m_pBarriers.ResourceKeys.resize(1);
          barrierCommand.m_pBarriers.ResourceAfterKeys.resize(1);
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
          barrier.Transition.StateAfter = it.second->CurrentState;
          barrierCommand.m_pBarriers.ResourceKeys[0] = it.first;
          m_StateService.GetRecorder().Record(
              ID3D12GraphicsCommandListResourceBarrierSerializer(barrierCommand));
        }
      }

      state->Desc->ScratchAccelerationStructureKey = scratchResourceKey;
      state->Desc->ScratchAccelerationStructureOffset = 0;

      {
        NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand build;
        build.Key = state->CommandKey;
        build.m_pCommandList.Key = m_CommandListDirectKey;
        build.m_pParams.Value = state->Desc->Value;
        build.m_pParams.DestAccelerationStructureKey = state->Desc->DestAccelerationStructureKey;
        build.m_pParams.DestAccelerationStructureOffset =
            state->Desc->DestAccelerationStructureOffset;
        build.m_pParams.SourceAccelerationStructureKey =
            state->Desc->SourceAccelerationStructureKey;
        build.m_pParams.SourceAccelerationStructureOffset =
            state->Desc->SourceAccelerationStructureOffset;
        build.m_pParams.ScratchAccelerationStructureKey =
            state->Desc->ScratchAccelerationStructureKey;
        build.m_pParams.ScratchAccelerationStructureOffset =
            state->Desc->ScratchAccelerationStructureOffset;
        build.m_pParams.InputKeys = state->Desc->InputKeys;
        build.m_pParams.InputOffsets = state->Desc->InputOffsets;
        build.m_pParams.Value->numPostbuildInfoDescs = 0;
        build.m_pParams.Value->pPostbuildInfoDescs = nullptr;
        m_Recorder.Record(NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer(build));
      }

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.Key = m_StateService.GetUniqueCommandKey();
        commandListClose.m_Object.Key = m_CommandListDirectKey;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListCloseSerializer(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand ExecuteCommandLists;
        ExecuteCommandLists.Key = m_StateService.GetUniqueCommandKey();
        ExecuteCommandLists.m_Object.Key = m_CommandQueueDirectKey;
        ExecuteCommandLists.m_NumCommandLists.Value = 1;
        ExecuteCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
        ExecuteCommandLists.m_ppCommandLists.Size = 1;
        ExecuteCommandLists.m_ppCommandLists.Keys.resize(1);
        ExecuteCommandLists.m_ppCommandLists.Keys[0] = m_CommandListDirectKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandQueueExecuteCommandListsSerializer(ExecuteCommandLists));

        ID3D12CommandQueueSignalCommand CommandQueueSignal;
        CommandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
        CommandQueueSignal.m_Object.Key = m_CommandQueueDirectKey;
        CommandQueueSignal.m_pFence.Key = m_FenceKey;
        CommandQueueSignal.m_Value.Value = ++m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(CommandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.Key = m_StateService.GetUniqueCommandKey();
        getCompletedValue.m_Object.Key = m_FenceKey;
        getCompletedValue.m_Result.Value = m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(
            ID3D12FenceGetCompletedValueSerializer(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.Key = m_StateService.GetUniqueCommandKey();
        commandAllocatorReset.m_Object.Key = m_CommandAllocatorDirectKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandAllocatorResetSerializer(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand CommandListReset;
        CommandListReset.Key = m_StateService.GetUniqueCommandKey();
        CommandListReset.m_Object.Key = m_CommandListDirectKey;
        CommandListReset.m_pAllocator.Key = m_CommandAllocatorDirectKey;
        CommandListReset.m_pInitialState.Key = 0;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListResetSerializer(CommandListReset));
      }
      RecordEvict(residencyKeys);
    } else if (itState.second->Kind == RaytracingAccelerationStructureState::StateKind::NvAPIOMM) {
      NvAPIBuildRaytracingOpacityMicromapArrayState* state =
          static_cast<NvAPIBuildRaytracingOpacityMicromapArrayState*>(itState.second);

      std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
          m_BufferContentRestore.GetRestoreInfos(state->CommandKey);

      std::set<unsigned> residencyKeys;
      for (const auto& info : restoreInfos) {
        InsertIfNotResident(info.BufferKey, residencyKeys);
      }
      InsertIfNotResident(state->Desc->DestOpacityMicromapArrayDataKey, residencyKeys);
      InsertIfNotResident(state->Desc->InputBufferKey, residencyKeys);
      InsertIfNotResident(state->Desc->PerOMMDescsKey, residencyKeys);
      for (unsigned key : state->Desc->DestPostBuildBufferKeys) {
        InsertIfNotResident(key, residencyKeys);
      }
      RecordMakeResident(residencyKeys);

      std::unordered_set<unsigned> restoredBuffers;
      size_t uploadBufferOffset{};
      for (AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
        auto itHash = bufferHashesByKeyOffset.find(std::pair(info.BufferKey, info.Offset));
        if (itHash != bufferHashesByKeyOffset.end() && itHash->second == info.BufferHash) {
          continue;
        }
        bufferHashesByKeyOffset[std::pair(info.BufferKey, info.Offset)] = info.BufferHash;
        restoredBuffers.insert(info.BufferKey);

        for (auto& itTiledResource : state->TiledResources) {
          auto it = tiledResourceUpdatesRestored.find(info.BufferKey);
          if (it == tiledResourceUpdatesRestored.end() ||
              it->second.find(itTiledResource.second.UpdateId) == it->second.end()) {
            m_ReservedResourcesService.UpdateTileMappings(itTiledResource.second,
                                                          m_CommandQueueCopyKey, nullptr);
            tiledResourceUpdatesRestored[info.BufferKey].insert(itTiledResource.second.UpdateId);
          }
        }

        uploadBufferOffset += RestoreBuffer(info, uploadBufferOffset);
      }

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.Key = m_StateService.GetUniqueCommandKey();
        commandListClose.m_Object.Key = m_CommandListCopyKey;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListCloseSerializer(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand ExecuteCommandLists;
        ExecuteCommandLists.Key = m_StateService.GetUniqueCommandKey();
        ExecuteCommandLists.m_Object.Key = m_CommandQueueCopyKey;
        ExecuteCommandLists.m_NumCommandLists.Value = 1;
        ExecuteCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
        ExecuteCommandLists.m_ppCommandLists.Size = 1;
        ExecuteCommandLists.m_ppCommandLists.Keys.resize(1);
        ExecuteCommandLists.m_ppCommandLists.Keys[0] = m_CommandListCopyKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandQueueExecuteCommandListsSerializer(ExecuteCommandLists));

        ID3D12CommandQueueSignalCommand CommandQueueSignal;
        CommandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
        CommandQueueSignal.m_Object.Key = m_CommandQueueCopyKey;
        CommandQueueSignal.m_pFence.Key = m_FenceKey;
        CommandQueueSignal.m_Value.Value = ++m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(CommandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.Key = m_StateService.GetUniqueCommandKey();
        getCompletedValue.m_Object.Key = m_FenceKey;
        getCompletedValue.m_Result.Value = m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(
            ID3D12FenceGetCompletedValueSerializer(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.Key = m_StateService.GetUniqueCommandKey();
        commandAllocatorReset.m_Object.Key = m_CommandAllocatorCopyKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandAllocatorResetSerializer(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand CommandListReset;
        CommandListReset.Key = m_StateService.GetUniqueCommandKey();
        CommandListReset.m_Object.Key = m_CommandListCopyKey;
        CommandListReset.m_pAllocator.Key = m_CommandAllocatorCopyKey;
        CommandListReset.m_pInitialState.Key = 0;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListResetSerializer(CommandListReset));
      }

      for (auto& it : state->Buffers) {
        if (!it.second->IsMappable && restoredBuffers.find(it.first) != restoredBuffers.end()) {
          ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
          barrierCommand.Key = m_StateService.GetUniqueCommandKey();
          barrierCommand.m_Object.Key = m_CommandListDirectKey;
          barrierCommand.m_NumBarriers.Value = 1;
          D3D12_RESOURCE_BARRIER barrier{};
          barrierCommand.m_pBarriers.Value = &barrier;
          barrierCommand.m_pBarriers.Size = 1;
          barrierCommand.m_pBarriers.ResourceKeys.resize(1);
          barrierCommand.m_pBarriers.ResourceAfterKeys.resize(1);
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
          barrier.Transition.StateAfter = it.second->CurrentState;
          barrierCommand.m_pBarriers.ResourceKeys[0] = it.first;
          m_StateService.GetRecorder().Record(
              ID3D12GraphicsCommandListResourceBarrierSerializer(barrierCommand));
        }
      }

      state->Desc->ScratchOpacityMicromapArrayDataKey = scratchResourceKey;
      state->Desc->ScratchOpacityMicromapArrayDataOffset = 0;

      {
        NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand build;
        build.Key = state->CommandKey;
        build.m_pCommandList.Key = m_CommandListDirectKey;
        build.m_pParams.Value = state->Desc->Value;
        build.m_pParams.DestOpacityMicromapArrayDataKey =
            state->Desc->DestOpacityMicromapArrayDataKey;
        build.m_pParams.DestOpacityMicromapArrayDataOffset =
            state->Desc->DestOpacityMicromapArrayDataOffset;
        build.m_pParams.InputBufferKey = state->Desc->InputBufferKey;
        build.m_pParams.InputBufferOffset = state->Desc->InputBufferOffset;
        build.m_pParams.PerOMMDescsKey = state->Desc->PerOMMDescsKey;
        build.m_pParams.PerOMMDescsOffset = state->Desc->PerOMMDescsOffset;
        build.m_pParams.ScratchOpacityMicromapArrayDataKey =
            state->Desc->ScratchOpacityMicromapArrayDataKey;
        build.m_pParams.ScratchOpacityMicromapArrayDataOffset =
            state->Desc->ScratchOpacityMicromapArrayDataOffset;
        build.m_pParams.Value->numPostbuildInfoDescs = 0;
        build.m_pParams.Value->pPostbuildInfoDescs = nullptr;
        m_Recorder.Record(NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer(build));
      }

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.Key = m_StateService.GetUniqueCommandKey();
        commandListClose.m_Object.Key = m_CommandListDirectKey;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListCloseSerializer(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand ExecuteCommandLists;
        ExecuteCommandLists.Key = m_StateService.GetUniqueCommandKey();
        ExecuteCommandLists.m_Object.Key = m_CommandQueueDirectKey;
        ExecuteCommandLists.m_NumCommandLists.Value = 1;
        ExecuteCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
        ExecuteCommandLists.m_ppCommandLists.Size = 1;
        ExecuteCommandLists.m_ppCommandLists.Keys.resize(1);
        ExecuteCommandLists.m_ppCommandLists.Keys[0] = m_CommandListDirectKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandQueueExecuteCommandListsSerializer(ExecuteCommandLists));

        ID3D12CommandQueueSignalCommand CommandQueueSignal;
        CommandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
        CommandQueueSignal.m_Object.Key = m_CommandQueueDirectKey;
        CommandQueueSignal.m_pFence.Key = m_FenceKey;
        CommandQueueSignal.m_Value.Value = ++m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(CommandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.Key = m_StateService.GetUniqueCommandKey();
        getCompletedValue.m_Object.Key = m_FenceKey;
        getCompletedValue.m_Result.Value = m_RecordedFenceValue;
        m_StateService.GetRecorder().Record(
            ID3D12FenceGetCompletedValueSerializer(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.Key = m_StateService.GetUniqueCommandKey();
        commandAllocatorReset.m_Object.Key = m_CommandAllocatorDirectKey;
        m_StateService.GetRecorder().Record(
            ID3D12CommandAllocatorResetSerializer(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand CommandListReset;
        CommandListReset.Key = m_StateService.GetUniqueCommandKey();
        CommandListReset.m_Object.Key = m_CommandListDirectKey;
        CommandListReset.m_pAllocator.Key = m_CommandAllocatorDirectKey;
        CommandListReset.m_pInitialState.Key = 0;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListResetSerializer(CommandListReset));
      }
      RecordEvict(residencyKeys);
    } else {
      GITS_ASSERT(0 && "unknown state");
    }
    delete itState.second;
  }

  {
    IUnknownReleaseCommand releaseScratchResource{};
    releaseScratchResource.Key = m_StateService.GetUniqueCommandKey();
    releaseScratchResource.m_Object.Key = scratchResourceKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseScratchResource));
  }
  {
    IUnknownReleaseCommand releaseFence{};
    releaseFence.Key = m_StateService.GetUniqueCommandKey();
    releaseFence.m_Object.Key = m_FenceKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseFence));
  }
  {
    IUnknownReleaseCommand releaseCommandList{};
    releaseCommandList.Key = m_StateService.GetUniqueCommandKey();
    releaseCommandList.m_Object.Key = m_CommandListDirectKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandList));

    IUnknownReleaseCommand releaseCommandAllocator{};
    releaseCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
    releaseCommandAllocator.m_Object.Key = m_CommandAllocatorDirectKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandAllocator));

    IUnknownReleaseCommand releaseCommandQueue{};
    releaseCommandQueue.Key = m_StateService.GetUniqueCommandKey();
    releaseCommandQueue.m_Object.Key = m_CommandQueueDirectKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandQueue));
  }
  {
    IUnknownReleaseCommand releaseCommandList{};
    releaseCommandList.Key = m_StateService.GetUniqueCommandKey();
    releaseCommandList.m_Object.Key = m_CommandListCopyKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandList));

    IUnknownReleaseCommand releaseCommandAllocator{};
    releaseCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
    releaseCommandAllocator.m_Object.Key = m_CommandAllocatorCopyKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandAllocator));

    IUnknownReleaseCommand releaseCommandQueue{};
    releaseCommandQueue.Key = m_StateService.GetUniqueCommandKey();
    releaseCommandQueue.m_Object.Key = m_CommandQueueCopyKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandQueue));
  }
  if (m_UploadBufferKey) {
    IUnknownReleaseCommand releaseUploadBuffer{};
    releaseUploadBuffer.Key = m_StateService.GetUniqueCommandKey();
    releaseUploadBuffer.m_Object.Key = m_UploadBufferKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseUploadBuffer));
  }

  m_Restored = true;
}

void AccelerationStructuresBuildService::ExecuteCommandLists(
    ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (m_Restored) {
    return;
  }
  for (unsigned commandListKey : c.m_ppCommandLists.Keys) {
    auto itStates = m_StatesByCommandList.find(commandListKey);
    if (itStates != m_StatesByCommandList.end()) {
      for (RaytracingAccelerationStructureState* state : itStates->second) {
        StoreState(state);
      }
      m_StatesByCommandList.erase(itStates);
    }
  }
  m_BufferContentRestore.executeCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                             c.m_ppCommandLists.Value, c.m_NumCommandLists.Value);
}

void AccelerationStructuresBuildService::CommandQueueWait(ID3D12CommandQueueWaitCommand& c) {
  m_BufferContentRestore.commandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void AccelerationStructuresBuildService::CommandQueueSignal(ID3D12CommandQueueSignalCommand& c) {
  m_BufferContentRestore.commandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void AccelerationStructuresBuildService::FenceSignal(unsigned key,
                                                     unsigned fenceKey,
                                                     UINT64 fenceValue) {
  m_BufferContentRestore.fenceSignal(key, fenceKey, fenceValue);
}

void AccelerationStructuresBuildService::StoreState(RaytracingAccelerationStructureState* state) {

  unsigned stateId = ++m_StateUniqueId;
  m_StatesById[stateId] = state;
  if (state->SourceKey) {
    unsigned sourceId = GetState(state->SourceKey, state->SourceOffset);
    GITS_ASSERT(sourceId);

    // remove intermediate update
    if (state->Kind == RaytracingAccelerationStructureState::StateKind::Build) {
      BuildRaytracingAccelerationStructureState* buildState =
          static_cast<BuildRaytracingAccelerationStructureState*>(state);
      if (buildState->Update) {
        auto itPrimarySource = m_StateSourceByDest.find(sourceId);
        if (itPrimarySource != m_StateSourceByDest.end()) {
          sourceId = itPrimarySource->second;
          auto itPrimarySourceState = m_StatesById.find(sourceId);
          GITS_ASSERT(itPrimarySourceState != m_StatesById.end());
          buildState->SourceKey = itPrimarySourceState->second->DestKey;
          buildState->SourceOffset = itPrimarySourceState->second->DestOffset;
          buildState->Desc->SourceAccelerationStructureKey = buildState->SourceKey;
          buildState->Desc->SourceAccelerationStructureOffset = buildState->SourceOffset;
        }
      }
    } else if (state->Kind == RaytracingAccelerationStructureState::StateKind::NvAPIBuild) {
      NvAPIBuildRaytracingAccelerationStructureExState* buildState =
          static_cast<NvAPIBuildRaytracingAccelerationStructureExState*>(state);
      if (buildState->Update) {
        auto itPrimarySource = m_StateSourceByDest.find(sourceId);
        if (itPrimarySource != m_StateSourceByDest.end()) {
          sourceId = itPrimarySource->second;
          auto itPrimarySourceState = m_StatesById.find(sourceId);
          GITS_ASSERT(itPrimarySourceState != m_StatesById.end());
          buildState->SourceKey = itPrimarySourceState->second->DestKey;
          buildState->SourceOffset = itPrimarySourceState->second->DestOffset;
          buildState->Desc->SourceAccelerationStructureKey = buildState->SourceKey;
          buildState->Desc->SourceAccelerationStructureOffset = buildState->SourceOffset;
        }
      }
    }

    m_StateSourceByDest[stateId] = sourceId;
    m_StateDestsBySource[sourceId].insert(stateId);
  }

  // remove previous state if not a source for any AS
  unsigned prevStateId = GetState(state->DestKey, state->DestOffset);
  if (prevStateId) {
    auto itDests = m_StateDestsBySource.find(prevStateId);
    if (itDests == m_StateDestsBySource.end()) {
      RemoveState(prevStateId);
    }
  }

  m_StateByKeyOffset[{state->DestKey, state->DestOffset}].insert(stateId);
}

unsigned AccelerationStructuresBuildService::GetState(unsigned key, unsigned offset) {
  auto itStates = m_StateByKeyOffset.find({key, offset});
  if (itStates == m_StateByKeyOffset.end() || itStates->second.empty()) {
    return 0;
  }
  return *itStates->second.rbegin();
}

void AccelerationStructuresBuildService::RemoveState(unsigned stateId, bool removeSource) {
  auto itState = m_StatesById.find(stateId);
  GITS_ASSERT(itState != m_StatesById.end());
  if (itState->second->Kind == RaytracingAccelerationStructureState::StateKind::NvAPIOMM) {
    return;
  } else if (itState->second->Kind == RaytracingAccelerationStructureState::StateKind::Build &&
             static_cast<BuildRaytracingAccelerationStructureState*>(itState->second)
                     ->Desc->Value->Inputs.Type ==
                 D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    return;
  }

  if (itState->second->FoundInAnalysis) {
    return;
  }

  // remove state sources chain
  auto itSource = m_StateSourceByDest.find(stateId);
  if (itSource != m_StateSourceByDest.end()) {
    auto itDests = m_StateDestsBySource.find(itSource->second);
    GITS_ASSERT(itDests != m_StateDestsBySource.end());
    itDests->second.erase(stateId);
    if (itDests->second.empty()) {
      if (removeSource) {
        RemoveState(itSource->second, removeSource);
        m_StateDestsBySource.erase(itDests);
      } else {
        m_SourcesWithoutDestinations.insert(itSource->second);
      }
    }
    m_StateSourceByDest.erase(itSource);
  }

  // remove state
  if (itState->second->Kind != RaytracingAccelerationStructureState::StateKind::Copy) {
    m_BufferContentRestore.RemoveBuild(itState->second->CommandKey);
  }
  auto it = m_StateByKeyOffset.find({itState->second->DestKey, itState->second->DestOffset});
  GITS_ASSERT(it != m_StateByKeyOffset.end());
  it->second.erase(stateId);
  if (it->second.empty()) {
    m_StateByKeyOffset.erase(it);
  }
  m_StatesById.erase(itState);
}

void AccelerationStructuresBuildService::InitUploadBuffer() {
  size_t maxPerBuildUploadSize = 0;
  for (auto& itState : m_StatesById) {
    std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
        m_BufferContentRestore.GetRestoreInfos(itState.second->CommandKey);
    size_t uploadSize = 0;
    for (const AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
      if (!info.IsMappable) {
        uploadSize += info.BufferData->size();
      }
    }

    if (uploadSize > maxPerBuildUploadSize) {
      maxPerBuildUploadSize = uploadSize;
    }
  }

  if (maxPerBuildUploadSize == 0) {
    return;
  }

  m_UploadBufferSize = maxPerBuildUploadSize;

  D3D12_HEAP_PROPERTIES heapPropertiesUpload{};
  heapPropertiesUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapPropertiesUpload.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapPropertiesUpload.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapPropertiesUpload.CreationNodeMask = 1;
  heapPropertiesUpload.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = m_UploadBufferSize;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  m_UploadBufferKey = m_StateService.GetUniqueObjectKey();

  ID3D12DeviceCreateCommittedResourceCommand createUploadResource;
  createUploadResource.Key = m_StateService.GetUniqueCommandKey();
  createUploadResource.m_Object.Key = m_DeviceKey;
  createUploadResource.m_pHeapProperties.Value = &heapPropertiesUpload;
  createUploadResource.m_HeapFlags.Value = D3D12_HEAP_FLAG_NONE;
  createUploadResource.m_pDesc.Value = &resourceDesc;
  createUploadResource.m_InitialResourceState.Value = D3D12_RESOURCE_STATE_GENERIC_READ;
  createUploadResource.m_pOptimizedClearValue.Value = nullptr;
  createUploadResource.m_riidResource.Value = IID_ID3D12Resource;
  createUploadResource.m_ppvResource.Key = m_UploadBufferKey;
  m_Recorder.Record(ID3D12DeviceCreateCommittedResourceSerializer(createUploadResource));
}

size_t AccelerationStructuresBuildService::RestoreBuffer(
    const AccelerationStructuresBufferContentRestore::BufferRestoreInfo& restoreInfo,
    size_t uploadBufferOffset) {
  if (restoreInfo.IsMappable) {
    ID3D12ResourceMapCommand mapCommand;
    mapCommand.Key = m_StateService.GetUniqueCommandKey();
    mapCommand.m_Object.Key = restoreInfo.BufferKey;
    mapCommand.m_Subresource.Value = 0;
    mapCommand.m_pReadRange.Value = nullptr;
    mapCommand.m_ppData.CaptureValue = m_StateService.GetUniqueFakePointer();
    mapCommand.m_ppData.Value = &mapCommand.m_ppData.CaptureValue;
    m_Recorder.Record(ID3D12ResourceMapSerializer(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.Key = m_StateService.GetUniqueCommandKey();
    metaCommand.m_resource.Key = restoreInfo.BufferKey;
    metaCommand.m_mappedAddress.Value = mapCommand.m_ppData.CaptureValue;
    metaCommand.m_offset.Value = restoreInfo.Offset;
    metaCommand.m_data.Value = const_cast<char*>(restoreInfo.BufferData->data());
    metaCommand.m_data.Size = restoreInfo.BufferData->size();
    m_Recorder.Record(MappedDataMetaSerializer(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.Key = m_StateService.GetUniqueCommandKey();
    unmapCommand.m_Object.Key = restoreInfo.BufferKey;
    unmapCommand.m_Subresource.Value = 0;
    unmapCommand.m_pWrittenRange.Value = nullptr;
    m_Recorder.Record(ID3D12ResourceUnmapSerializer(unmapCommand));

    return 0;
  } else {
    GITS_ASSERT(uploadBufferOffset + restoreInfo.BufferData->size() <= m_UploadBufferSize);

    ID3D12ResourceMapCommand mapCommand;
    mapCommand.Key = m_StateService.GetUniqueCommandKey();
    mapCommand.m_Object.Key = m_UploadBufferKey;
    mapCommand.m_Subresource.Value = 0;
    mapCommand.m_pReadRange.Value = nullptr;
    mapCommand.m_ppData.CaptureValue = m_StateService.GetUniqueFakePointer();
    mapCommand.m_ppData.Value = &mapCommand.m_ppData.CaptureValue;
    m_Recorder.Record(ID3D12ResourceMapSerializer(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.Key = m_StateService.GetUniqueCommandKey();
    metaCommand.m_resource.Key = m_UploadBufferKey;
    metaCommand.m_mappedAddress.Value = mapCommand.m_ppData.CaptureValue;
    metaCommand.m_offset.Value = uploadBufferOffset;
    metaCommand.m_data.Value = const_cast<char*>(restoreInfo.BufferData->data());
    metaCommand.m_data.Size = restoreInfo.BufferData->size();
    m_Recorder.Record(MappedDataMetaSerializer(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.Key = m_StateService.GetUniqueCommandKey();
    unmapCommand.m_Object.Key = m_UploadBufferKey;
    unmapCommand.m_Subresource.Value = 0;
    unmapCommand.m_pWrittenRange.Value = nullptr;
    m_Recorder.Record(ID3D12ResourceUnmapSerializer(unmapCommand));

    ID3D12GraphicsCommandListCopyBufferRegionCommand copyBufferRegion;
    copyBufferRegion.Key = m_StateService.GetUniqueCommandKey();
    copyBufferRegion.m_Object.Key = m_CommandListCopyKey;
    copyBufferRegion.m_pDstBuffer.Key = restoreInfo.BufferKey;
    copyBufferRegion.m_DstOffset.Value = restoreInfo.Offset;
    copyBufferRegion.m_pSrcBuffer.Key = m_UploadBufferKey;
    copyBufferRegion.m_SrcOffset.Value = uploadBufferOffset;
    copyBufferRegion.m_NumBytes.Value = restoreInfo.BufferData->size();
    m_Recorder.Record(ID3D12GraphicsCommandListCopyBufferRegionSerializer(copyBufferRegion));

    return restoreInfo.BufferData->size();
  }
}

void AccelerationStructuresBuildService::Optimize() {
  if (!Configurator::Get().directx.features.subcapture.optimize &&
      !Configurator::Get().directx.features.subcapture.optimizeRaytracing) {
    return;
  }
  std::vector<unsigned> removedStates;
  for (auto& [stateId, state] : m_StatesById) {
    if (!m_StateService.GetAnalyzerResults().RestoreBlas(
            std::make_pair(state->DestKey, state->DestOffset)) &&
        !m_StateService.GetAnalyzerResults().RestoreTlas(state->CommandKey)) {
      auto itDests = m_StateDestsBySource.find(stateId);
      if (itDests == m_StateDestsBySource.end() || itDests->second.empty()) {
        removedStates.push_back(stateId);
      }
    } else {
      state->FoundInAnalysis = true;
    }
  }
  for (unsigned stateId : removedStates) {
    RemoveState(stateId, true);
  }
}

void AccelerationStructuresBuildService::RemoveSourcesWithoutDestinations() {
  for (unsigned source : m_SourcesWithoutDestinations) {
    auto itDests = m_StateDestsBySource.find(source);
    GITS_ASSERT(itDests != m_StateDestsBySource.end());
    if (itDests->second.empty()) {
      RemoveState(source, true);
      m_StateDestsBySource.erase(itDests);
    }
  }
}

void AccelerationStructuresBuildService::CompleteSourcesFromAnalysis() {
  std::set<std::pair<unsigned, unsigned>>& sources =
      m_StateService.GetAnalyzerResults().GetAsSources();
  for (auto& keyOffset : sources) {
    auto itStates = m_StateByKeyOffset.find(keyOffset);
    if (itStates != m_StateByKeyOffset.end() && !itStates->second.empty()) {
      unsigned lastState = *itStates->second.rbegin();
      m_StateDestsBySource[lastState].insert(UINT_MAX);
    }
  }
}

void AccelerationStructuresBuildService::StoreBuffer(
    unsigned inputKey,
    unsigned inputOffset,
    unsigned size,
    unsigned commandKey,
    ID3D12GraphicsCommandList* commandList,
    BufferBackedRaytracingAccelerationStructureState* state) {
  m_StateService.KeepState(inputKey);
  ResourceState* bufferState = static_cast<ResourceState*>(m_StateService.GetState(inputKey));
  bufferState->CurrentState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  D3D12_RESOURCE_STATES trackedState =
      m_ResourceStateTracker.GetResourceState(commandList, inputKey, 0);
  if (trackedState == D3D12_RESOURCE_STATE_GENERIC_READ ||
      trackedState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
    bufferState->CurrentState = trackedState;
  } else if (trackedState != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
    if (bufferState->DenyShaderResource) {
      bufferState->CurrentState = D3D12_RESOURCE_STATE_COMMON;
    } else {
      CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
          m_GpuAddressService.GetResourceInfoByCaptureAddress(bufferState->GpuVirtualAddress +
                                                              inputOffset);
      if (resourceInfo && resourceInfo->Overlapping()) {
        bufferState->CurrentState = trackedState;
        static bool logged = false;
        if (!logged) {
          LOG_WARNING << "State restore - state of overlapped resource different than expected";
          logged = true;
        }
      }
      if (commandList->GetType() == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        bufferState->CurrentState &= ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        if (bufferState->CurrentState != trackedState) {
          static bool logged = false;
          if (!logged) {
            LOG_WARNING
                << "State restore - state of overlapped resource adjusted for compute Command list";
            logged = true;
          }
        }
      }
    }
  }
  m_BufferContentRestore.StoreBuffer(commandList, static_cast<ID3D12Resource*>(bufferState->Object),
                                     inputKey, inputOffset, size, bufferState->CurrentState,
                                     commandKey, bufferState->IsMappable);
  state->Buffers[inputKey] = bufferState;
  ReservedResourcesService::TiledResource* tiledResource =
      m_ReservedResourcesService.GetTiledResource(inputKey);
  if (tiledResource) {
    auto it = state->TiledResources.find(inputKey);
    if (it == state->TiledResources.end()) {
      state->TiledResources[inputKey] = *tiledResource;
    }
    for (ReservedResourcesService::Tile& tile : tiledResource->Tiles) {
      if (tile.HeapKey) {
        m_StateService.KeepState(tile.HeapKey);
      }
    }
  }
}

std::vector<AccelerationStructuresBuildService::Interval> AccelerationStructuresBuildService::
    MergeIntervals(const std::vector<Interval>& intervals) {
  std::vector<Interval> intervalsCopy = intervals;
  std::sort(intervalsCopy.begin(), intervalsCopy.end(),
            [](const auto& a, const auto& b) { return a.Start < b.Start; });

  std::vector<Interval> merged;
  for (auto interval : intervalsCopy) {
    if (merged.empty() || merged.back().End < interval.Start) {
      merged.push_back(interval);
    } else {
      merged.back().End = std::max(merged.back().End, interval.End);
    }
  }

  return merged;
}

void AccelerationStructuresBuildService::InsertIfNotResident(unsigned ResourceKey,
                                                             std::set<unsigned>& residencyKeys) {
  if (!ResourceKey) {
    return;
  }

  auto residencyKey = GetResidencyKeyForNotResidentResource(ResourceKey);
  if (residencyKey.has_value() && residencyKey.value() != 0) {
    residencyKeys.insert(residencyKey.value());
  }
}

std::optional<unsigned> AccelerationStructuresBuildService::GetResidencyKeyForNotResidentResource(
    unsigned key) {
  ObjectState* state = m_StateService.GetState(key);
  if (!state) {
    return std::nullopt;
  }

  switch (state->CreationCommand->GetId()) {
  case CommandId::ID_ID3D12DEVICE_CREATECOMMITTEDRESOURCE: {
    auto* Command =
        static_cast<ID3D12DeviceCreateCommittedResourceCommand*>(state->CreationCommand.get());
    if (Command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      return key;
    }
  } break;
  case CommandId::ID_ID3D12DEVICE4_CREATECOMMITTEDRESOURCE1: {
    auto* Command =
        static_cast<ID3D12Device4CreateCommittedResource1Command*>(state->CreationCommand.get());
    if (Command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      return key;
    }
  } break;
  case CommandId::ID_ID3D12DEVICE8_CREATECOMMITTEDRESOURCE2: {
    auto* Command =
        static_cast<ID3D12Device8CreateCommittedResource2Command*>(state->CreationCommand.get());
    if (Command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      return key;
    }
  } break;
  case CommandId::ID_ID3D12DEVICE10_CREATECOMMITTEDRESOURCE3: {
    auto* Command =
        static_cast<ID3D12Device10CreateCommittedResource3Command*>(state->CreationCommand.get());
    if (Command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      return key;
    }
  } break;
  case CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE: {
    auto* Command =
        static_cast<INTC_D3D12_CreateCommittedResourceCommand*>(state->CreationCommand.get());
    if (Command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      return key;
    }
  } break;
  case CommandId::ID_ID3D12DEVICE_CREATEPLACEDRESOURCE:
  case CommandId::ID_ID3D12DEVICE8_CREATEPLACEDRESOURCE1:
  case CommandId::ID_ID3D12DEVICE10_CREATEPLACEDRESOURCE2:
  case CommandId::INTC_D3D12_CREATEPLACEDRESOURCE: {
    unsigned heapKey = static_cast<ResourceState*>(state)->HeapKey;
    ObjectState* heapState = m_StateService.GetState(heapKey);
    if (!heapState) {
      return std::nullopt;
    }
    switch (heapState->CreationCommand->GetId()) {
    case CommandId::ID_ID3D12DEVICE_CREATEHEAP: {
      auto* Command = static_cast<ID3D12DeviceCreateHeapCommand*>(heapState->CreationCommand.get());
      if (Command->m_pDesc.Value->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
        return heapKey;
      }
    } break;
    case CommandId::ID_ID3D12DEVICE4_CREATEHEAP1: {
      auto* Command =
          static_cast<ID3D12Device4CreateHeap1Command*>(heapState->CreationCommand.get());
      if (Command->m_pDesc.Value->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
        return heapKey;
      }
    } break;
    case CommandId::INTC_D3D12_CREATEHEAP: {
      auto* Command = static_cast<INTC_D3D12_CreateHeapCommand*>(heapState->CreationCommand.get());
      if (Command->m_pDesc.Value->pD3D12Desc->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
        return heapKey;
      }
    } break;
    default:
      return std::nullopt;
    }
  } break;
  }
  return std::nullopt;
}

void AccelerationStructuresBuildService::RecordMakeResident(const std::set<unsigned>& keys) {
  if (keys.empty()) {
    return;
  }

  ID3D12DeviceMakeResidentCommand MakeResident;
  MakeResident.Key = m_StateService.GetUniqueCommandKey();
  MakeResident.m_Object.Key = m_DeviceKey;
  MakeResident.m_NumObjects.Value = keys.size();
  ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
  MakeResident.m_ppObjects.Value = &fakePtr;
  MakeResident.m_ppObjects.Size = keys.size();
  for (unsigned key : keys) {
    MakeResident.m_ppObjects.Keys.push_back(key);
  }
  m_StateService.GetRecorder().Record(ID3D12DeviceMakeResidentSerializer(MakeResident));
}

void AccelerationStructuresBuildService::RecordEvict(const std::set<unsigned>& keys) {
  if (keys.empty()) {
    return;
  }

  ID3D12DeviceEvictCommand Evict;
  Evict.Key = m_StateService.GetUniqueCommandKey();
  Evict.m_Object.Key = m_DeviceKey;
  Evict.m_NumObjects.Value = keys.size();
  ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
  Evict.m_ppObjects.Value = &fakePtr;
  Evict.m_ppObjects.Size = keys.size();
  for (unsigned key : keys) {
    Evict.m_ppObjects.Keys.push_back(key);
  }
  m_StateService.GetRecorder().Record(ID3D12DeviceEvictSerializer(Evict));
}

} // namespace DirectX
} // namespace gits
