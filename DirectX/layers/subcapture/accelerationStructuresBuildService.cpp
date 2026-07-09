// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresBuildService.h"
#include "stateTrackingService.h"
#include "analyzerResults.h"
#include "resourceStateEnhanced.h"
#include "arguments.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
#include "reservedResourcesService.h"
#include "resourceResidencyService.h"
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
      m_ResourceStateTracker(resourceStateTracker),
      m_GpuAddressService(gpuAddressService),
      m_OptimizationService(stateService),
      m_InputBuffersService(stateService,
                            reservedResourcesService,
                            resourceStateTracker,
                            gpuAddressService,
                            recorder),
      m_BufferReleaseService(recorder) {
  m_SerializeMode = Configurator::Get().directx.features.subcapture.serializeAccelerationStructures;
  m_RestoreTlas = Configurator::Get().directx.features.subcapture.restoreTLASes;
  m_Optimize = Configurator::Get().directx.features.subcapture.optimize;
}

void AccelerationStructuresBuildService::BuildAccelerationStructure(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (m_Restored) {
    return;
  }
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
  } else if (!m_StateService.GetAnalyzerResults().RestoreBlas(c.Key)) {
    return;
  }

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = c.m_pDesc.Value->Inputs;

  // get scratch space size
  {
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
  }

  BuildRaytracingAccelerationStructureCommand* command =
      new BuildRaytracingAccelerationStructureCommand();
  command->CommandKey = c.Key;
  command->CommandListKey = m_CommandListKey;
  command->Type = RaytracingAccelerationStructureCommand::CommandType::Build;
  command->DestKey = c.m_pDesc.DestAccelerationStructureKey;
  command->DestOffset = c.m_pDesc.DestAccelerationStructureOffset;
  command->SourceKey = c.m_pDesc.SourceAccelerationStructureKey;
  command->SourceOffset = c.m_pDesc.SourceAccelerationStructureOffset;
  command->Update = c.m_pDesc.Value->Inputs.Flags &
                    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  command->TlasBuild =
      c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

  if (m_SerializeMode &&
      c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    m_TlasesKeyOffsets.insert(std::make_pair(command->DestKey, command->DestOffset));
  }

  command->Desc.reset(
      new PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>(c.m_pDesc));

  m_StateService.KeepState(c.m_pDesc.DestAccelerationStructureKey);

  // get input buffer regions
  {
    unsigned inputIndex = 0;
    if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL &&
        inputs.InstanceDescs) {
      unsigned size = inputs.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
      if (inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        size = inputs.NumDescs * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
      }
      m_InputBuffersService.StoreBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                              c.m_pDesc.InputOffsets[inputIndex], size);
    } else if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
      for (unsigned i = 0; i < inputs.NumDescs; ++i) {
        D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
            c.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
                ? c.m_pDesc.Value->Inputs.pGeometryDescs[i]
                : *c.m_pDesc.Value->Inputs.ppGeometryDescs[i]);
        if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
          if (desc.Triangles.Transform3x4) {
            unsigned size = sizeof(float) * 3 * 4;
            m_InputBuffersService.StoreBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                                    c.m_pDesc.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
          if (desc.Triangles.IndexBuffer && desc.Triangles.IndexCount) {
            unsigned size = desc.Triangles.IndexCount *
                            (desc.Triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
            m_InputBuffersService.StoreBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                                    c.m_pDesc.InputOffsets[inputIndex], size);
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
            m_InputBuffersService.StoreBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                                    c.m_pDesc.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
        } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
          if (desc.AABBs.AABBs.StartAddress && desc.AABBs.AABBCount) {
            unsigned size = desc.AABBs.AABBCount * desc.AABBs.AABBs.StrideInBytes;
            m_InputBuffersService.StoreBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                                    c.m_pDesc.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
        } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (desc.OmmTriangles.pTriangles) {
            auto& triangles = *desc.OmmTriangles.pTriangles;
            if (triangles.Transform3x4) {
              unsigned size = sizeof(float) * 3 * 4;
              m_InputBuffersService.StoreBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                                      c.m_pDesc.InputOffsets[inputIndex], size);
            }
            ++inputIndex;
            if (triangles.IndexBuffer && triangles.IndexCount) {
              unsigned size =
                  triangles.IndexCount * (triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
              m_InputBuffersService.StoreBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                                      c.m_pDesc.InputOffsets[inputIndex], size);
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
              m_InputBuffersService.StoreBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                                      c.m_pDesc.InputOffsets[inputIndex], size);
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
                m_InputBuffersService.StoreBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                                        c.m_pDesc.InputOffsets[inputIndex], size);
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
          } else if (histogramEntry.Format ==
                     D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT_OC1_2_STATE) {
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
          m_InputBuffersService.StoreBufferRegion(
              c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex], totalInputSize);
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
          m_InputBuffersService.StoreBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                                  c.m_pDesc.InputOffsets[inputIndex], size);
        }
        ++inputIndex;
      }
    }
  }

  m_InputBuffersService.StoreBuffers(c.Key, c.m_Object.Value);
  m_OptimizationService.AddCommand(c.m_Object.Key, command);

  m_BufferReleaseService.AddBuffer(c.m_pDesc.DestAccelerationStructureKey);
  m_BufferReleaseService.AddBuffer(c.m_pDesc.SourceAccelerationStructureKey);
  for (unsigned key : c.m_pDesc.InputKeys) {
    m_BufferReleaseService.AddBuffer(key);
  }
}

void AccelerationStructuresBuildService::CopyAccelerationStructure(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (m_Restored) {
    return;
  }
  if (m_SerializeMode) {
    auto it = m_TlasesKeyOffsets.find(std::make_pair(c.m_DestAccelerationStructureData.InterfaceKey,
                                                     c.m_DestAccelerationStructureData.Offset));
    if (it == m_TlasesKeyOffsets.end()) {
      return;
    }
  }
  if (!m_StateService.GetAnalyzerResults().RestoreBlas(c.Key)) {
    return;
  }

  CopyRaytracingAccelerationStructureCommand* command =
      new CopyRaytracingAccelerationStructureCommand();
  command->CommandKey = c.Key;
  command->CommandListKey = m_CommandListKey;
  command->Type = RaytracingAccelerationStructureCommand::CommandType::Copy;
  command->DestAccelerationStructureData = c.m_DestAccelerationStructureData.Value;
  command->DestKey = c.m_DestAccelerationStructureData.InterfaceKey;
  command->DestOffset = c.m_DestAccelerationStructureData.Offset;
  command->SourceAccelerationStructureData = c.m_SourceAccelerationStructureData.Value;
  command->SourceKey = c.m_SourceAccelerationStructureData.InterfaceKey;
  command->SourceOffset = c.m_SourceAccelerationStructureData.Offset;
  command->Mode = c.m_Mode.Value;

  m_StateService.KeepState(c.m_DestAccelerationStructureData.InterfaceKey);
  m_OptimizationService.AddCommand(c.m_Object.Key, command);

  m_BufferReleaseService.AddBuffer(c.m_DestAccelerationStructureData.InterfaceKey);
  m_BufferReleaseService.AddBuffer(c.m_SourceAccelerationStructureData.InterfaceKey);
}

void AccelerationStructuresBuildService::NvapiBuildAccelerationStructureEx(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (m_Restored) {
    return;
  }
  const NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX* buildDesc =
      c.m_pParams.Value->pDesc;
  if (!m_RestoreTlas &&
      buildDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }
  if (!m_StateService.GetAnalyzerResults().RestoreBlas(c.Key)) {
    return;
  }

  NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS_EX inputs = buildDesc->inputs;

  // get scratch space size
  {
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
  }

  NvAPIBuildRaytracingAccelerationStructureExCommand* command =
      new NvAPIBuildRaytracingAccelerationStructureExCommand();
  command->CommandKey = c.Key;
  command->CommandListKey = m_CommandListKey;
  command->Type = RaytracingAccelerationStructureCommand::CommandType::NvAPIBuild;
  command->DestKey = c.m_pParams.DestAccelerationStructureKey;
  command->DestOffset = c.m_pParams.DestAccelerationStructureOffset;
  command->SourceKey = c.m_pParams.SourceAccelerationStructureKey;
  command->SourceOffset = c.m_pParams.SourceAccelerationStructureOffset;
  command->Update = buildDesc->inputs.flags &
                    NVAPI_D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE_EX;
  command->TlasBuild = c.m_pParams.Value->pDesc->inputs.type ==
                       D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

  if (m_SerializeMode &&
      buildDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    m_TlasesKeyOffsets.insert(std::make_pair(command->DestKey, command->DestOffset));
  }

  command->Desc.reset(
      new PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>(c.m_pParams));

  m_StateService.KeepState(c.m_pParams.DestAccelerationStructureKey);

  // get input buffer regions
  {
    unsigned inputIndex = 0;
    if (inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL && inputs.numDescs) {
      if (inputs.numDescs) {
        unsigned size = inputs.numDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
        m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                c.m_pParams.InputOffsets[inputIndex], size);
      }
    } else {
      for (unsigned i = 0; i < inputs.numDescs; ++i) {
        const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
            buildDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
                ? *(const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(buildDesc->inputs
                                                                                .pGeometryDescs) +
                                                                    buildDesc->inputs
                                                                            .geometryDescStrideInBytes *
                                                                        i)
                : *buildDesc->inputs.ppGeometryDescs[i];
        if (desc.type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
          if (desc.triangles.Transform3x4) {
            unsigned size = sizeof(float) * 3 * 4;
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
          if (desc.triangles.IndexBuffer && desc.triangles.IndexCount) {
            unsigned size = desc.triangles.IndexCount *
                            (desc.triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
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
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
        } else if (desc.type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
          if (desc.aabbs.AABBs.StartAddress && desc.aabbs.AABBCount) {
            unsigned size = desc.aabbs.AABBCount * desc.aabbs.AABBs.StrideInBytes;
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
        } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
          if (desc.ommTriangles.triangles.Transform3x4) {
            unsigned size = sizeof(float) * 3 * 4;
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
          if (desc.ommTriangles.triangles.IndexBuffer && desc.ommTriangles.triangles.IndexCount) {
            unsigned size =
                desc.ommTriangles.triangles.IndexCount *
                (desc.ommTriangles.triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
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
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
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
              m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                      c.m_pParams.InputOffsets[inputIndex], size);
            }
          }
          ++inputIndex;
          ++inputIndex;
        } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
          if (desc.dmmTriangles.triangles.Transform3x4) {
            unsigned size = sizeof(float) * 3 * 4;
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
          if (desc.dmmTriangles.triangles.IndexBuffer && desc.dmmTriangles.triangles.IndexCount) {
            unsigned size =
                desc.dmmTriangles.triangles.IndexCount *
                (desc.dmmTriangles.triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
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
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
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
              m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                      c.m_pParams.InputOffsets[inputIndex], size);
            }
          }
          ++inputIndex;
          if (desc.dmmTriangles.dmmAttachment.trianglePrimitiveFlagsBuffer.StartAddress) {
            unsigned size = desc.dmmTriangles.triangles.VertexCount;
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
          if (desc.dmmTriangles.dmmAttachment.vertexBiasAndScaleBuffer.StartAddress) {
            unsigned stride =
                desc.dmmTriangles.dmmAttachment.vertexBiasAndScaleBuffer.StrideInBytes;
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
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
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
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
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
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
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
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
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
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
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
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
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
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
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
            m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                    c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
        }
      }
    }
  }

  m_InputBuffersService.StoreBuffers(c.Key, c.m_pCommandList.Value);
  m_OptimizationService.AddCommand(c.m_pCommandList.Key, command);

  m_BufferReleaseService.AddBuffer(c.m_pParams.DestAccelerationStructureKey);
  m_BufferReleaseService.AddBuffer(c.m_pParams.SourceAccelerationStructureKey);
  for (unsigned key : c.m_pParams.InputKeys) {
    m_BufferReleaseService.AddBuffer(key);
  }
}

void AccelerationStructuresBuildService::NvapiBuildOpacityMicromapArray(
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (m_Restored) {
    return;
  }
  if (!m_StateService.GetAnalyzerResults().RestoreBlas(c.Key)) {
    return;
  }

  const NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC* buildDesc =
      c.m_pParams.Value->pDesc;

  NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_INPUTS inputs = buildDesc->inputs;

  // get scratch space size
  {
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
  }

  NvAPIBuildRaytracingOpacityMicromapArrayCommand* command =
      new NvAPIBuildRaytracingOpacityMicromapArrayCommand();
  command->CommandKey = c.Key;
  command->CommandListKey = m_CommandListKey;
  command->Type = RaytracingAccelerationStructureCommand::CommandType::NvAPIOMM;
  command->DestKey = c.m_pParams.DestOpacityMicromapArrayDataKey;
  command->DestOffset = c.m_pParams.DestOpacityMicromapArrayDataOffset;

  command->Desc.reset(
      new PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>(c.m_pParams));

  m_StateService.KeepState(c.m_pParams.DestOpacityMicromapArrayDataKey);

  // get input buffer regions
  {
    unsigned ommCount{};
    size_t inputSize{};
    for (unsigned i = 0; i < buildDesc->inputs.numOMMUsageCounts; ++i) {
      const auto& usage = buildDesc->inputs.pOMMUsageCounts[i];
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

    if (buildDesc->inputs.inputBuffer) {
      m_InputBuffersService.StoreBufferRegion(c.m_pParams.InputBufferKey,
                                              c.m_pParams.InputBufferOffset, inputSize);
    }
    if (buildDesc->inputs.perOMMDescs.StartAddress) {
      unsigned stride = buildDesc->inputs.perOMMDescs.StrideInBytes;
      if (!stride) {
        stride = sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT);
      }
      m_InputBuffersService.StoreBufferRegion(c.m_pParams.PerOMMDescsKey,
                                              c.m_pParams.PerOMMDescsOffset, stride * ommCount);
    }
  }

  m_InputBuffersService.StoreBuffers(c.Key, c.m_pCommandList.Value);
  m_OptimizationService.AddCommand(c.m_pCommandList.Key, command);

  m_BufferReleaseService.AddBuffer(c.m_pParams.DestOpacityMicromapArrayDataKey);
  m_BufferReleaseService.AddBuffer(c.m_pParams.InputBufferKey);
  m_BufferReleaseService.AddBuffer(c.m_pParams.PerOMMDescsKey);
}

void AccelerationStructuresBuildService::RestoreAccelerationStructures() {
  m_OptimizationService.ProcessCommands();
  if (m_OptimizationService.GetCommands().empty()) {
    m_OptimizationService.Cleanup();
    return;
  }

  std::vector<unsigned> commandKeys;
  commandKeys.reserve(m_OptimizationService.GetCommands().size());
  for (OptimizationService::CommandNode* node : m_OptimizationService.GetCommands()) {
    commandKeys.push_back(node->Command->CommandKey);
  }
  m_InputBuffersService.RestoreBuffersInitialization(commandKeys, m_DeviceKey);

  {
    m_CommandQueueKey = m_StateService.GetUniqueObjectKey();
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ID3D12DeviceCreateCommandQueueCommand createCommandQueue;
    createCommandQueue.Key = m_StateService.GetUniqueCommandKey();
    createCommandQueue.m_Object.Key = m_DeviceKey;
    createCommandQueue.m_pDesc.Value = &commandQueueDesc;
    createCommandQueue.m_riid.Value = IID_ID3D12CommandQueue;
    createCommandQueue.m_ppCommandQueue.Key = m_CommandQueueKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandQueueSerializer(createCommandQueue));

    m_CommandAllocatorKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
    createCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
    createCommandAllocator.m_Object.Key = m_DeviceKey;
    createCommandAllocator.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
    createCommandAllocator.m_riid.Value = IID_ID3D12CommandAllocator;
    createCommandAllocator.m_ppCommandAllocator.Key = m_CommandAllocatorKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandAllocatorSerializer(createCommandAllocator));

    m_CommandListKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandListCommand createCommandList;
    createCommandList.Key = m_StateService.GetUniqueCommandKey();
    createCommandList.m_Object.Key = m_DeviceKey;
    createCommandList.m_nodeMask.Value = 0;
    createCommandList.m_pCommandAllocator.Key = createCommandAllocator.m_ppCommandAllocator.Key;
    createCommandList.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
    createCommandList.m_pInitialState.Value = nullptr;
    createCommandList.m_riid.Value = IID_ID3D12CommandList;
    createCommandList.m_ppCommandList.Key = m_CommandListKey;
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

  {
    m_ScratchResourceKey = m_StateService.GetUniqueObjectKey();

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
    createScratch.m_ppvResource.Key = m_ScratchResourceKey;
    m_Recorder.Record(ID3D12DeviceCreateCommittedResourceSerializer(createScratch));

    ID3D12ResourceGetGPUVirtualAddressCommand getAddress{};
    getAddress.Key = m_StateService.GetUniqueCommandKey();
    getAddress.m_Object.Key = m_ScratchResourceKey;
    m_Recorder.Record(ID3D12ResourceGetGPUVirtualAddressSerializer(getAddress));
  }

  // restoring RTAS
  for (OptimizationService::CommandNode* node : m_OptimizationService.GetCommands()) {
    if (node->Command->Type == RaytracingAccelerationStructureCommand::CommandType::Build) {
      BuildRaytracingAccelerationStructureCommand* build =
          static_cast<BuildRaytracingAccelerationStructureCommand*>(node->Command.get());
      RestoreCommand(build);
    } else if (node->Command->Type == RaytracingAccelerationStructureCommand::CommandType::Copy) {
      CopyRaytracingAccelerationStructureCommand* copy =
          static_cast<CopyRaytracingAccelerationStructureCommand*>(node->Command.get());
      RestoreCommand(copy);
    } else if (node->Command->Type ==
               RaytracingAccelerationStructureCommand::CommandType::NvAPIBuild) {
      NvAPIBuildRaytracingAccelerationStructureExCommand* build =
          static_cast<NvAPIBuildRaytracingAccelerationStructureExCommand*>(node->Command.get());
      RestoreCommand(build);
    } else if (node->Command->Type ==
               RaytracingAccelerationStructureCommand::CommandType::NvAPIOMM) {
      NvAPIBuildRaytracingOpacityMicromapArrayCommand* build =
          static_cast<NvAPIBuildRaytracingOpacityMicromapArrayCommand*>(node->Command.get());
      RestoreCommand(build);
    } else {
      GITS_ASSERT(0 && "unknown command");
    }

    m_BufferReleaseService.ProcessReleases(node->Command->CommandKey);
  }

  // cleanup
  {
    m_OptimizationService.Cleanup();
    {
      IUnknownReleaseCommand releaseScratchResource{};
      releaseScratchResource.Key = m_StateService.GetUniqueCommandKey();
      releaseScratchResource.m_Object.Key = m_ScratchResourceKey;
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
      releaseCommandList.m_Object.Key = m_CommandListKey;
      m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandList));

      IUnknownReleaseCommand releaseCommandAllocator{};
      releaseCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
      releaseCommandAllocator.m_Object.Key = m_CommandAllocatorKey;
      m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandAllocator));

      IUnknownReleaseCommand releaseCommandQueue{};
      releaseCommandQueue.Key = m_StateService.GetUniqueCommandKey();
      releaseCommandQueue.m_Object.Key = m_CommandQueueKey;
      m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandQueue));
    }
    m_InputBuffersService.RestoreBuffersCleanup();
  }

  m_Restored = true;
}

void AccelerationStructuresBuildService::ExecuteCommandLists(
    ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (m_Restored) {
    return;
  }
  m_OptimizationService.OnExecute(c.m_ppCommandLists.Keys);
  m_InputBuffersService.ExecuteCommandLists(c);
}

void AccelerationStructuresBuildService::CommandQueueWait(ID3D12CommandQueueWaitCommand& c) {
  m_InputBuffersService.CommandQueueWait(c);
}

void AccelerationStructuresBuildService::CommandQueueSignal(ID3D12CommandQueueSignalCommand& c) {
  m_InputBuffersService.CommandQueueSignal(c);
}

void AccelerationStructuresBuildService::FenceSignal(unsigned key,
                                                     unsigned fenceKey,
                                                     UINT64 fenceValue) {
  m_InputBuffersService.FenceSignal(key, fenceKey, fenceValue);
}

void AccelerationStructuresBuildService::DestroyResource(unsigned commandKey,
                                                         unsigned resourceKey) {
  m_BufferReleaseService.AddRelease(commandKey, resourceKey);
}

void AccelerationStructuresBuildService::RestoreCommand(
    BuildRaytracingAccelerationStructureCommand* command) {

  ResourceResidencyService residencyService(m_StateService, m_DeviceKey);
  m_InputBuffersService.MakeBuffersResident(command->CommandKey, residencyService);
  residencyService.AddResource(command->Desc->DestAccelerationStructureKey);
  residencyService.AddResource(command->Desc->SourceAccelerationStructureKey);
  residencyService.RecordMakeResident();

  m_InputBuffersService.RestoreBuffers(command->CommandKey, m_CommandListKey);

  command->Desc->ScratchAccelerationStructureKey = m_ScratchResourceKey;
  command->Desc->ScratchAccelerationStructureOffset = 0;

  {
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand build;
    build.Key = command->CommandKey;
    build.m_Object.Key = m_CommandListKey;
    build.m_pDesc.Value = command->Desc->Value;
    build.m_pDesc.DestAccelerationStructureKey = command->Desc->DestAccelerationStructureKey;
    build.m_pDesc.DestAccelerationStructureOffset = command->Desc->DestAccelerationStructureOffset;
    build.m_pDesc.SourceAccelerationStructureKey = command->Desc->SourceAccelerationStructureKey;
    build.m_pDesc.SourceAccelerationStructureOffset =
        command->Desc->SourceAccelerationStructureOffset;
    build.m_pDesc.ScratchAccelerationStructureKey = command->Desc->ScratchAccelerationStructureKey;
    build.m_pDesc.ScratchAccelerationStructureOffset =
        command->Desc->ScratchAccelerationStructureOffset;
    build.m_pDesc.InputKeys = command->Desc->InputKeys;
    build.m_pDesc.InputOffsets = command->Desc->InputOffsets;
    build.m_NumPostbuildInfoDescs.Value = 0;
    build.m_pPostbuildInfoDescs.Value = nullptr;
    m_Recorder.Record(
        ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureSerializer(build));
  }

  RecordExecuteCommandLists();
  residencyService.RecordEvict();
}

void AccelerationStructuresBuildService::RestoreCommand(
    CopyRaytracingAccelerationStructureCommand* command) {
  ResourceResidencyService residencyService(m_StateService, m_DeviceKey);
  residencyService.AddResource(command->DestKey);
  residencyService.AddResource(command->SourceKey);
  residencyService.RecordMakeResident();

  ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand copy;
  copy.Key = command->CommandKey;
  copy.m_Object.Key = m_CommandListKey;
  copy.m_DestAccelerationStructureData.Value = command->DestAccelerationStructureData;
  copy.m_DestAccelerationStructureData.InterfaceKey = command->DestKey;
  copy.m_DestAccelerationStructureData.Offset = command->DestOffset;
  copy.m_SourceAccelerationStructureData.Value = command->SourceAccelerationStructureData;
  copy.m_SourceAccelerationStructureData.InterfaceKey = command->SourceKey;
  copy.m_SourceAccelerationStructureData.Offset = command->SourceOffset;
  copy.m_Mode.Value = command->Mode;
  m_StateService.GetRecorder().Record(
      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureSerializer(copy));

  RecordExecuteCommandLists();
  residencyService.RecordEvict();
}

void AccelerationStructuresBuildService::RestoreCommand(
    NvAPIBuildRaytracingAccelerationStructureExCommand* command) {

  ResourceResidencyService residencyService(m_StateService, m_DeviceKey);
  m_InputBuffersService.MakeBuffersResident(command->CommandKey, residencyService);
  residencyService.AddResource(command->Desc->DestAccelerationStructureKey);
  residencyService.AddResource(command->Desc->SourceAccelerationStructureKey);
  residencyService.RecordMakeResident();

  m_InputBuffersService.RestoreBuffers(command->CommandKey, m_CommandListKey);

  command->Desc->ScratchAccelerationStructureKey = m_ScratchResourceKey;
  command->Desc->ScratchAccelerationStructureOffset = 0;

  {
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand build;
    build.Key = command->CommandKey;
    build.m_pCommandList.Key = m_CommandListKey;
    build.m_pParams.Value = command->Desc->Value;
    build.m_pParams.DestAccelerationStructureKey = command->Desc->DestAccelerationStructureKey;
    build.m_pParams.DestAccelerationStructureOffset =
        command->Desc->DestAccelerationStructureOffset;
    build.m_pParams.SourceAccelerationStructureKey = command->Desc->SourceAccelerationStructureKey;
    build.m_pParams.SourceAccelerationStructureOffset =
        command->Desc->SourceAccelerationStructureOffset;
    build.m_pParams.ScratchAccelerationStructureKey =
        command->Desc->ScratchAccelerationStructureKey;
    build.m_pParams.ScratchAccelerationStructureOffset =
        command->Desc->ScratchAccelerationStructureOffset;
    build.m_pParams.InputKeys = command->Desc->InputKeys;
    build.m_pParams.InputOffsets = command->Desc->InputOffsets;
    build.m_pParams.Value->numPostbuildInfoDescs = 0;
    build.m_pParams.Value->pPostbuildInfoDescs = nullptr;
    m_Recorder.Record(NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer(build));
  }

  RecordExecuteCommandLists();
  residencyService.RecordEvict();
}

void AccelerationStructuresBuildService::RestoreCommand(
    NvAPIBuildRaytracingOpacityMicromapArrayCommand* command) {

  ResourceResidencyService residencyService(m_StateService, m_DeviceKey);
  m_InputBuffersService.MakeBuffersResident(command->CommandKey, residencyService);
  residencyService.AddResource(command->Desc->DestOpacityMicromapArrayDataKey);
  residencyService.AddResource(command->Desc->InputBufferKey);
  residencyService.AddResource(command->Desc->PerOMMDescsKey);
  for (unsigned key : command->Desc->DestPostBuildBufferKeys) {
    residencyService.AddResource(key);
  }
  residencyService.RecordMakeResident();

  m_InputBuffersService.RestoreBuffers(command->CommandKey, m_CommandListKey);

  command->Desc->ScratchOpacityMicromapArrayDataKey = m_ScratchResourceKey;
  command->Desc->ScratchOpacityMicromapArrayDataOffset = 0;

  {
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand build;
    build.Key = command->CommandKey;
    build.m_pCommandList.Key = m_CommandListKey;
    build.m_pParams.Value = command->Desc->Value;
    build.m_pParams.DestOpacityMicromapArrayDataKey =
        command->Desc->DestOpacityMicromapArrayDataKey;
    build.m_pParams.DestOpacityMicromapArrayDataOffset =
        command->Desc->DestOpacityMicromapArrayDataOffset;
    build.m_pParams.InputBufferKey = command->Desc->InputBufferKey;
    build.m_pParams.InputBufferOffset = command->Desc->InputBufferOffset;
    build.m_pParams.PerOMMDescsKey = command->Desc->PerOMMDescsKey;
    build.m_pParams.PerOMMDescsOffset = command->Desc->PerOMMDescsOffset;
    build.m_pParams.ScratchOpacityMicromapArrayDataKey =
        command->Desc->ScratchOpacityMicromapArrayDataKey;
    build.m_pParams.ScratchOpacityMicromapArrayDataOffset =
        command->Desc->ScratchOpacityMicromapArrayDataOffset;
    build.m_pParams.Value->numPostbuildInfoDescs = 0;
    build.m_pParams.Value->pPostbuildInfoDescs = nullptr;
    m_Recorder.Record(NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer(build));
  }

  RecordExecuteCommandLists();
  residencyService.RecordEvict();
}

void AccelerationStructuresBuildService::RecordExecuteCommandLists() {
  ID3D12GraphicsCommandListCloseCommand commandListClose;
  commandListClose.Key = m_StateService.GetUniqueCommandKey();
  commandListClose.m_Object.Key = m_CommandListKey;
  m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListCloseSerializer(commandListClose));

  ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
  executeCommandLists.Key = m_StateService.GetUniqueCommandKey();
  executeCommandLists.m_Object.Key = m_CommandQueueKey;
  executeCommandLists.m_NumCommandLists.Value = 1;
  executeCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
  executeCommandLists.m_ppCommandLists.Size = 1;
  executeCommandLists.m_ppCommandLists.Keys.resize(1);
  executeCommandLists.m_ppCommandLists.Keys[0] = m_CommandListKey;
  m_StateService.GetRecorder().Record(
      ID3D12CommandQueueExecuteCommandListsSerializer(executeCommandLists));

  ID3D12CommandQueueSignalCommand commandQueueSignal;
  commandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
  commandQueueSignal.m_Object.Key = m_CommandQueueKey;
  commandQueueSignal.m_pFence.Key = m_FenceKey;
  commandQueueSignal.m_Value.Value = ++m_RecordedFenceValue;
  m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(commandQueueSignal));

  ID3D12FenceGetCompletedValueCommand getCompletedValue;
  getCompletedValue.Key = m_StateService.GetUniqueCommandKey();
  getCompletedValue.m_Object.Key = m_FenceKey;
  getCompletedValue.m_Result.Value = m_RecordedFenceValue;
  m_StateService.GetRecorder().Record(ID3D12FenceGetCompletedValueSerializer(getCompletedValue));

  ID3D12CommandAllocatorResetCommand commandAllocatorReset;
  commandAllocatorReset.Key = m_StateService.GetUniqueCommandKey();
  commandAllocatorReset.m_Object.Key = m_CommandAllocatorKey;
  m_StateService.GetRecorder().Record(ID3D12CommandAllocatorResetSerializer(commandAllocatorReset));

  ID3D12GraphicsCommandListResetCommand commandListReset;
  commandListReset.Key = m_StateService.GetUniqueCommandKey();
  commandListReset.m_Object.Key = m_CommandListKey;
  commandListReset.m_pAllocator.Key = m_CommandAllocatorKey;
  commandListReset.m_pInitialState.Key = 0;
  m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListResetSerializer(commandListReset));
}

void AccelerationStructuresBuildService::OptimizationService::OnExecute(
    std::vector<unsigned>& commandListKeys) {
  for (unsigned commandListKey : commandListKeys) {
    auto itStates = m_CommandsByCommandList.find(commandListKey);
    if (itStates != m_CommandsByCommandList.end()) {
      for (auto& command : itStates->second) {
        StoreCommand(command);
      }
      m_CommandsByCommandList.erase(itStates);
    }
  }
}

void AccelerationStructuresBuildService::OptimizationService::StoreCommand(
    std::unique_ptr<RaytracingAccelerationStructureCommand>& command) {
  CommandNode* node = new CommandNode{};
  node->Command.swap(command);
  node->Id = ++m_CommandUniqueId;
  m_CommandById[node->Id].reset(node);
  m_CommandByBuildKey[node->Command->CommandKey] = node->Command.get();

  // skip intermediate update build command
  if (node->Command->Update) {
    unsigned sourceKey =
        m_StateService.GetAnalyzerResults().GetBlasSourceBuild(node->Command->CommandKey);
    if (sourceKey) {
      auto it = m_CommandByBuildKey.find(sourceKey);
      GITS_ASSERT(it != m_CommandByBuildKey.end());
      RaytracingAccelerationStructureCommand* source = it->second;
      GITS_ASSERT(source);
      switch (node->Command->Type) {
      case RaytracingAccelerationStructureCommand::CommandType::Build: {
        auto* build =
            static_cast<BuildRaytracingAccelerationStructureCommand*>(node->Command.get());
        build->Desc->SourceAccelerationStructureKey = source->DestKey;
        build->Desc->SourceAccelerationStructureOffset = source->DestOffset;
      } break;
      case RaytracingAccelerationStructureCommand::CommandType::NvAPIBuild: {
        auto* build =
            static_cast<NvAPIBuildRaytracingAccelerationStructureExCommand*>(node->Command.get());
        build->Desc->SourceAccelerationStructureKey = source->DestKey;
        build->Desc->SourceAccelerationStructureOffset = source->DestOffset;
      } break;
      }
    }
  }
}

void AccelerationStructuresBuildService::OptimizationService::ProcessCommands() {
  for (auto& it : m_CommandById) {
    m_RestoreCommands.push_back(it.second.get());
  }
  std::sort(m_RestoreCommands.begin(), m_RestoreCommands.end(),
            [](const CommandNode* a, const CommandNode* b) { return a->Id < b->Id; });
}

void AccelerationStructuresBuildService::OptimizationService::Cleanup() {
  m_CommandsByCommandList.clear();
  m_CommandById.clear();
  m_CommandByBuildKey.clear();
  m_RestoreCommands.clear();
}

void AccelerationStructuresBuildService::BufferReleaseService::AddBuffer(unsigned key) {
  if (key) {
    m_Buffers.insert(key);
  }
}

void AccelerationStructuresBuildService::BufferReleaseService::AddRelease(unsigned commandKey,
                                                                          unsigned bufferKey) {
  auto it = m_Buffers.find(bufferKey);
  if (it != m_Buffers.end()) {
    m_Releases[commandKey] = bufferKey;
    m_Buffers.erase(it);
  }
}

void AccelerationStructuresBuildService::BufferReleaseService::ProcessReleases(
    unsigned commandKey) {
  auto endIt = m_Releases.lower_bound(commandKey);
  for (auto it = m_Releases.begin(); it != endIt;) {
    IUnknownReleaseCommand release{};
    release.Key = it->first;
    release.m_Object.Key = it->second;
    m_Recorder.Record(IUnknownReleaseSerializer(release));
    it = m_Releases.erase(it);
  }
}

} // namespace DirectX
} // namespace gits
