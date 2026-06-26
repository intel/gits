// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresBuildService.h"
#include "stateTrackingService.h"
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
    StateTrackingService& m_StateService,
    SubcaptureRecorder& m_Recorder,
    ReservedResourcesService& reservedResourcesService,
    ResourceStateTracker& resourceStateTracker,
    CapturePlayerGpuAddressService& gpuAddressService)
    : m_StateService(m_StateService),
      m_Recorder(m_Recorder),
      m_ReservedResourcesService(reservedResourcesService),
      m_BufferContentRestore(m_StateService),
      m_ResourceStateTracker(resourceStateTracker),
      m_GpuAddressService(gpuAddressService),
      m_OptimizationService(m_StateService, m_BufferContentRestore) {
  m_SerializeMode = Configurator::Get().directx.features.subcapture.serializeAccelerationStructures;
  m_RestoreTlas = Configurator::Get().directx.features.subcapture.restoreTLASes;
  m_Optimize = Configurator::Get().directx.features.subcapture.optimize;
}

void AccelerationStructuresBuildService::BuildAccelerationStructure(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  {
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
  command->CommandListKey = m_CommandListDirectKeys.CommandListKey;
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

  InputBufferService inputBufferService(m_StateService, m_BufferContentRestore,
                                        m_ReservedResourcesService, m_ResourceStateTracker,
                                        m_GpuAddressService);
  // get input buffer regions
  {
    unsigned inputIndex = 0;
    if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL &&
        inputs.InstanceDescs) {
      unsigned size = inputs.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
      if (inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        size = inputs.NumDescs * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
      }
      inputBufferService.AddBufferRegion(c.m_pDesc.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                               c.m_pDesc.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
          if (desc.Triangles.IndexBuffer && desc.Triangles.IndexCount) {
            unsigned size = desc.Triangles.IndexCount *
                            (desc.Triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
            inputBufferService.AddBufferRegion(c.m_pDesc.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                               c.m_pDesc.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
        } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
          if (desc.AABBs.AABBs.StartAddress && desc.AABBs.AABBCount) {
            unsigned size = desc.AABBs.AABBCount * desc.AABBs.AABBs.StrideInBytes;
            inputBufferService.AddBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                               c.m_pDesc.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
        } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (desc.OmmTriangles.pTriangles) {
            auto& triangles = *desc.OmmTriangles.pTriangles;
            if (triangles.Transform3x4) {
              unsigned size = sizeof(float) * 3 * 4;
              inputBufferService.AddBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                                 c.m_pDesc.InputOffsets[inputIndex], size);
            }
            ++inputIndex;
            if (triangles.IndexBuffer && triangles.IndexCount) {
              unsigned size =
                  triangles.IndexCount * (triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
              inputBufferService.AddBufferRegion(c.m_pDesc.InputKeys[inputIndex],
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
              inputBufferService.AddBufferRegion(c.m_pDesc.InputKeys[inputIndex],
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
                inputBufferService.AddBufferRegion(c.m_pDesc.InputKeys[inputIndex],
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
          inputBufferService.AddBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                             c.m_pDesc.InputOffsets[inputIndex], totalInputSize);
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
          inputBufferService.AddBufferRegion(c.m_pDesc.InputKeys[inputIndex],
                                             c.m_pDesc.InputOffsets[inputIndex], size);
        }
        ++inputIndex;
      }
    }
  }

  inputBufferService.StoreBuffers(c.Key, c.m_Object.Value, command);
  m_OptimizationService.AddCommand(c.m_Object.Key, command);
}

void AccelerationStructuresBuildService::CopyAccelerationStructure(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (m_SerializeMode) {
    auto it = m_TlasesKeyOffsets.find(std::make_pair(c.m_DestAccelerationStructureData.InterfaceKey,
                                                     c.m_DestAccelerationStructureData.Offset));
    if (it == m_TlasesKeyOffsets.end()) {
      return;
    }
  }
  CopyRaytracingAccelerationStructureCommand* command =
      new CopyRaytracingAccelerationStructureCommand();
  command->CommandKey = c.Key;
  command->CommandListKey = m_CommandListDirectKeys.CommandListKey;
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
}

void AccelerationStructuresBuildService::NvapiBuildAccelerationStructureEx(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  const NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX* buildDesc =
      c.m_pParams.Value->pDesc;
  if (!m_RestoreTlas &&
      buildDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }

  if (m_Restored) {
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
  command->CommandListKey = m_CommandListDirectKeys.CommandListKey;
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

  InputBufferService inputBufferService(m_StateService, m_BufferContentRestore,
                                        m_ReservedResourcesService, m_ResourceStateTracker,
                                        m_GpuAddressService);
  // get input buffer regions
  {
    unsigned inputIndex = 0;
    if (inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL && inputs.numDescs) {
      if (inputs.numDescs) {
        unsigned size = inputs.numDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
        inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                               c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
          if (desc.triangles.IndexBuffer && desc.triangles.IndexCount) {
            unsigned size = desc.triangles.IndexCount *
                            (desc.triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                               c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
        } else if (desc.type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
          if (desc.aabbs.AABBs.StartAddress && desc.aabbs.AABBCount) {
            unsigned size = desc.aabbs.AABBCount * desc.aabbs.AABBs.StrideInBytes;
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                               c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
        } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
          if (desc.ommTriangles.triangles.Transform3x4) {
            unsigned size = sizeof(float) * 3 * 4;
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                               c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
          if (desc.ommTriangles.triangles.IndexBuffer && desc.ommTriangles.triangles.IndexCount) {
            unsigned size =
                desc.ommTriangles.triangles.IndexCount *
                (desc.ommTriangles.triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
              inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                 c.m_pParams.InputOffsets[inputIndex], size);
            }
          }
          ++inputIndex;
          ++inputIndex;
        } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
          if (desc.dmmTriangles.triangles.Transform3x4) {
            unsigned size = sizeof(float) * 3 * 4;
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                               c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
          if (desc.dmmTriangles.triangles.IndexBuffer && desc.dmmTriangles.triangles.IndexCount) {
            unsigned size =
                desc.dmmTriangles.triangles.IndexCount *
                (desc.dmmTriangles.triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
              inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                                 c.m_pParams.InputOffsets[inputIndex], size);
            }
          }
          ++inputIndex;
          if (desc.dmmTriangles.dmmAttachment.trianglePrimitiveFlagsBuffer.StartAddress) {
            unsigned size = desc.dmmTriangles.triangles.VertexCount;
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
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
            inputBufferService.AddBufferRegion(c.m_pParams.InputKeys[inputIndex],
                                               c.m_pParams.InputOffsets[inputIndex], size);
          }
          ++inputIndex;
        }
      }
    }
  }

  inputBufferService.StoreBuffers(c.Key, c.m_pCommandList.Value, command);
  m_OptimizationService.AddCommand(c.m_pCommandList.Key, command);
}

void AccelerationStructuresBuildService::NvapiBuildOpacityMicromapArray(
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (m_Restored) {
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
  command->CommandListKey = m_CommandListDirectKeys.CommandListKey;
  command->Type = RaytracingAccelerationStructureCommand::CommandType::NvAPIOMM;
  command->DestKey = c.m_pParams.DestOpacityMicromapArrayDataKey;
  command->DestOffset = c.m_pParams.DestOpacityMicromapArrayDataOffset;

  command->Desc.reset(
      new PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>(c.m_pParams));

  m_StateService.KeepState(c.m_pParams.DestOpacityMicromapArrayDataKey);

  InputBufferService inputBufferService(m_StateService, m_BufferContentRestore,
                                        m_ReservedResourcesService, m_ResourceStateTracker,
                                        m_GpuAddressService);
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
      inputBufferService.AddBufferRegion(c.m_pParams.InputBufferKey, c.m_pParams.InputBufferOffset,
                                         inputSize);
    }
    if (buildDesc->inputs.perOMMDescs.StartAddress) {
      unsigned stride = buildDesc->inputs.perOMMDescs.StrideInBytes;
      if (!stride) {
        stride = sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT);
      }
      inputBufferService.AddBufferRegion(c.m_pParams.PerOMMDescsKey, c.m_pParams.PerOMMDescsOffset,
                                         stride * ommCount);
    }
  }

  inputBufferService.StoreBuffers(c.Key, c.m_pCommandList.Value, command);
  m_OptimizationService.AddCommand(c.m_pCommandList.Key, command);
}

void AccelerationStructuresBuildService::RestoreAccelerationStructures() {
  m_OptimizationService.ProcessCommands();
  if (m_OptimizationService.GetCommands().empty()) {
    m_OptimizationService.Cleanup();
    return;
  }

  m_BufferContentRestore.WaitUntilDumped();

  InitUploadBuffer();
  {
    m_CommandListCopyKeys.CommandQueueKey = m_StateService.GetUniqueObjectKey();
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    ID3D12DeviceCreateCommandQueueCommand createCommandQueue;
    createCommandQueue.Key = m_StateService.GetUniqueCommandKey();
    createCommandQueue.m_Object.Key = m_DeviceKey;
    createCommandQueue.m_pDesc.Value = &commandQueueDesc;
    createCommandQueue.m_riid.Value = IID_ID3D12CommandQueue;
    createCommandQueue.m_ppCommandQueue.Key = m_CommandListCopyKeys.CommandQueueKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandQueueSerializer(createCommandQueue));

    m_CommandListCopyKeys.CommandAllocatorKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
    createCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
    createCommandAllocator.m_Object.Key = m_DeviceKey;
    createCommandAllocator.m_type.Value = D3D12_COMMAND_LIST_TYPE_COPY;
    createCommandAllocator.m_riid.Value = IID_ID3D12CommandAllocator;
    createCommandAllocator.m_ppCommandAllocator.Key = m_CommandListCopyKeys.CommandAllocatorKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandAllocatorSerializer(createCommandAllocator));

    m_CommandListCopyKeys.CommandListKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandListCommand createCommandList;
    createCommandList.Key = m_StateService.GetUniqueCommandKey();
    createCommandList.m_Object.Key = m_DeviceKey;
    createCommandList.m_nodeMask.Value = 0;
    createCommandList.m_pCommandAllocator.Key = createCommandAllocator.m_ppCommandAllocator.Key;
    createCommandList.m_type.Value = D3D12_COMMAND_LIST_TYPE_COPY;
    createCommandList.m_pInitialState.Value = nullptr;
    createCommandList.m_riid.Value = IID_ID3D12CommandList;
    createCommandList.m_ppCommandList.Key = m_CommandListCopyKeys.CommandListKey;
    m_StateService.GetRecorder().Record(ID3D12DeviceCreateCommandListSerializer(createCommandList));
  }
  {
    m_CommandListDirectKeys.CommandQueueKey = m_StateService.GetUniqueObjectKey();
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ID3D12DeviceCreateCommandQueueCommand createCommandQueue;
    createCommandQueue.Key = m_StateService.GetUniqueCommandKey();
    createCommandQueue.m_Object.Key = m_DeviceKey;
    createCommandQueue.m_pDesc.Value = &commandQueueDesc;
    createCommandQueue.m_riid.Value = IID_ID3D12CommandQueue;
    createCommandQueue.m_ppCommandQueue.Key = m_CommandListDirectKeys.CommandQueueKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandQueueSerializer(createCommandQueue));

    m_CommandListDirectKeys.CommandAllocatorKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
    createCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
    createCommandAllocator.m_Object.Key = m_DeviceKey;
    createCommandAllocator.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
    createCommandAllocator.m_riid.Value = IID_ID3D12CommandAllocator;
    createCommandAllocator.m_ppCommandAllocator.Key = m_CommandListDirectKeys.CommandAllocatorKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandAllocatorSerializer(createCommandAllocator));

    m_CommandListDirectKeys.CommandListKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandListCommand createCommandList;
    createCommandList.Key = m_StateService.GetUniqueCommandKey();
    createCommandList.m_Object.Key = m_DeviceKey;
    createCommandList.m_nodeMask.Value = 0;
    createCommandList.m_pCommandAllocator.Key = createCommandAllocator.m_ppCommandAllocator.Key;
    createCommandList.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
    createCommandList.m_pInitialState.Value = nullptr;
    createCommandList.m_riid.Value = IID_ID3D12CommandList;
    createCommandList.m_ppCommandList.Key = m_CommandListDirectKeys.CommandListKey;
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
      releaseCommandList.m_Object.Key = m_CommandListDirectKeys.CommandListKey;
      m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandList));

      IUnknownReleaseCommand releaseCommandAllocator{};
      releaseCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
      releaseCommandAllocator.m_Object.Key = m_CommandListDirectKeys.CommandAllocatorKey;
      m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandAllocator));

      IUnknownReleaseCommand releaseCommandQueue{};
      releaseCommandQueue.Key = m_StateService.GetUniqueCommandKey();
      releaseCommandQueue.m_Object.Key = m_CommandListDirectKeys.CommandQueueKey;
      m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandQueue));
    }
    {
      IUnknownReleaseCommand releaseCommandList{};
      releaseCommandList.Key = m_StateService.GetUniqueCommandKey();
      releaseCommandList.m_Object.Key = m_CommandListCopyKeys.CommandListKey;
      m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandList));

      IUnknownReleaseCommand releaseCommandAllocator{};
      releaseCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
      releaseCommandAllocator.m_Object.Key = m_CommandListCopyKeys.CommandAllocatorKey;
      m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandAllocator));

      IUnknownReleaseCommand releaseCommandQueue{};
      releaseCommandQueue.Key = m_StateService.GetUniqueCommandKey();
      releaseCommandQueue.m_Object.Key = m_CommandListCopyKeys.CommandQueueKey;
      m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandQueue));
    }
    if (m_UploadBufferKey) {
      IUnknownReleaseCommand releaseUploadBuffer{};
      releaseUploadBuffer.Key = m_StateService.GetUniqueCommandKey();
      releaseUploadBuffer.m_Object.Key = m_UploadBufferKey;
      m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseUploadBuffer));
    }
  }

  m_Restored = true;
}

void AccelerationStructuresBuildService::ExecuteCommandLists(
    ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (m_Restored) {
    return;
  }
  m_OptimizationService.OnExecute(c.m_ppCommandLists.Keys);
  m_BufferContentRestore.ExecuteCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                             c.m_ppCommandLists.Value, c.m_NumCommandLists.Value);
}

void AccelerationStructuresBuildService::CommandQueueWait(ID3D12CommandQueueWaitCommand& c) {
  m_BufferContentRestore.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void AccelerationStructuresBuildService::CommandQueueSignal(ID3D12CommandQueueSignalCommand& c) {
  m_BufferContentRestore.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void AccelerationStructuresBuildService::FenceSignal(unsigned key,
                                                     unsigned fenceKey,
                                                     UINT64 fenceValue) {
  m_BufferContentRestore.FenceSignal(key, fenceKey, fenceValue);
}

void AccelerationStructuresBuildService::InitUploadBuffer() {
  size_t maxPerBuildUploadSize = 0;
  for (OptimizationService::CommandNode* node : m_OptimizationService.GetCommands()) {
    std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
        m_BufferContentRestore.GetRestoreInfos(node->Command->CommandKey);
    size_t uploadSize = 0;
    for (AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
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

void AccelerationStructuresBuildService::RestoreCommand(
    BuildRaytracingAccelerationStructureCommand* command) {
  std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
      m_BufferContentRestore.GetRestoreInfos(command->CommandKey);

  ResourceResidencyService residencyService(m_StateService, m_DeviceKey);
  for (const auto& info : restoreInfos) {
    residencyService.AddResource(info.BufferKey);
  }
  residencyService.AddResource(command->Desc->DestAccelerationStructureKey);
  residencyService.AddResource(command->Desc->SourceAccelerationStructureKey);
  residencyService.RecordMakeResident();

  std::unordered_set<unsigned> restoredBuffers;
  size_t uploadBufferOffset{};
  for (AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
    auto itHash = m_BufferHashesByKeyOffset.find(std::pair(info.BufferKey, info.Offset));
    if (itHash != m_BufferHashesByKeyOffset.end() && itHash->second == info.BufferHash) {
      continue;
    }
    m_BufferHashesByKeyOffset[std::pair(info.BufferKey, info.Offset)] = info.BufferHash;
    restoredBuffers.insert(info.BufferKey);

    for (auto& itTiledResource : command->TiledResources) {
      auto it = m_TiledResourceUpdatesRestored.find(info.BufferKey);
      if (it == m_TiledResourceUpdatesRestored.end() ||
          it->second.find(itTiledResource.second.UpdateId) == it->second.end()) {
        m_ReservedResourcesService.UpdateTileMappings(
            itTiledResource.second, m_CommandListCopyKeys.CommandQueueKey, nullptr);
        m_TiledResourceUpdatesRestored[info.BufferKey].insert(itTiledResource.second.UpdateId);
      }
    }

    uploadBufferOffset += RestoreBuffer(info, uploadBufferOffset);
  }

  RecordExecuteCommandLists(m_CommandListCopyKeys);

  for (auto& it : command->Buffers) {
    if (!it.second->IsMappable && restoredBuffers.find(it.first) != restoredBuffers.end()) {
      ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
      barrierCommand.Key = m_StateService.GetUniqueCommandKey();
      barrierCommand.m_Object.Key = m_CommandListDirectKeys.CommandListKey;
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

  command->Desc->ScratchAccelerationStructureKey = m_ScratchResourceKey;
  command->Desc->ScratchAccelerationStructureOffset = 0;

  {
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand build;
    build.Key = command->CommandKey;
    build.m_Object.Key = m_CommandListDirectKeys.CommandListKey;
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

  RecordExecuteCommandLists(m_CommandListDirectKeys);
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
  copy.m_Object.Key = m_CommandListDirectKeys.CommandListKey;
  copy.m_DestAccelerationStructureData.Value = command->DestAccelerationStructureData;
  copy.m_DestAccelerationStructureData.InterfaceKey = command->DestKey;
  copy.m_DestAccelerationStructureData.Offset = command->DestOffset;
  copy.m_SourceAccelerationStructureData.Value = command->SourceAccelerationStructureData;
  copy.m_SourceAccelerationStructureData.InterfaceKey = command->SourceKey;
  copy.m_SourceAccelerationStructureData.Offset = command->SourceOffset;
  copy.m_Mode.Value = command->Mode;
  m_StateService.GetRecorder().Record(
      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureSerializer(copy));

  RecordExecuteCommandLists(m_CommandListDirectKeys);
  residencyService.RecordEvict();
}

void AccelerationStructuresBuildService::RestoreCommand(
    NvAPIBuildRaytracingAccelerationStructureExCommand* command) {
  std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
      m_BufferContentRestore.GetRestoreInfos(command->CommandKey);

  ResourceResidencyService residencyService(m_StateService, m_DeviceKey);
  for (const auto& info : restoreInfos) {
    residencyService.AddResource(info.BufferKey);
  }
  residencyService.AddResource(command->Desc->DestAccelerationStructureKey);
  residencyService.AddResource(command->Desc->SourceAccelerationStructureKey);
  residencyService.RecordMakeResident();

  std::unordered_set<unsigned> restoredBuffers;
  size_t uploadBufferOffset{};
  for (AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
    auto itHash = m_BufferHashesByKeyOffset.find(std::pair(info.BufferKey, info.Offset));
    if (itHash != m_BufferHashesByKeyOffset.end() && itHash->second == info.BufferHash) {
      continue;
    }
    m_BufferHashesByKeyOffset[std::pair(info.BufferKey, info.Offset)] = info.BufferHash;
    restoredBuffers.insert(info.BufferKey);

    for (auto& itTiledResource : command->TiledResources) {
      auto it = m_TiledResourceUpdatesRestored.find(info.BufferKey);
      if (it == m_TiledResourceUpdatesRestored.end() ||
          it->second.find(itTiledResource.second.UpdateId) == it->second.end()) {
        m_ReservedResourcesService.UpdateTileMappings(
            itTiledResource.second, m_CommandListCopyKeys.CommandQueueKey, nullptr);
        m_TiledResourceUpdatesRestored[info.BufferKey].insert(itTiledResource.second.UpdateId);
      }
    }

    uploadBufferOffset += RestoreBuffer(info, uploadBufferOffset);
  }

  RecordExecuteCommandLists(m_CommandListCopyKeys);

  for (auto& it : command->Buffers) {
    if (!it.second->IsMappable && restoredBuffers.find(it.first) != restoredBuffers.end()) {
      ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
      barrierCommand.Key = m_StateService.GetUniqueCommandKey();
      barrierCommand.m_Object.Key = m_CommandListDirectKeys.CommandListKey;
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

  command->Desc->ScratchAccelerationStructureKey = m_ScratchResourceKey;
  command->Desc->ScratchAccelerationStructureOffset = 0;

  {
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand build;
    build.Key = command->CommandKey;
    build.m_pCommandList.Key = m_CommandListDirectKeys.CommandListKey;
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

  RecordExecuteCommandLists(m_CommandListDirectKeys);
  residencyService.RecordEvict();
}

void AccelerationStructuresBuildService::RestoreCommand(
    NvAPIBuildRaytracingOpacityMicromapArrayCommand* command) {
  std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
      m_BufferContentRestore.GetRestoreInfos(command->CommandKey);

  ResourceResidencyService residencyService(m_StateService, m_DeviceKey);
  for (const auto& info : restoreInfos) {
    residencyService.AddResource(info.BufferKey);
  }
  residencyService.AddResource(command->Desc->DestOpacityMicromapArrayDataKey);
  residencyService.AddResource(command->Desc->InputBufferKey);
  residencyService.AddResource(command->Desc->PerOMMDescsKey);
  for (unsigned key : command->Desc->DestPostBuildBufferKeys) {
    residencyService.AddResource(key);
  }
  residencyService.RecordMakeResident();

  std::unordered_set<unsigned> restoredBuffers;
  size_t uploadBufferOffset{};
  for (AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
    auto itHash = m_BufferHashesByKeyOffset.find(std::pair(info.BufferKey, info.Offset));
    if (itHash != m_BufferHashesByKeyOffset.end() && itHash->second == info.BufferHash) {
      continue;
    }
    m_BufferHashesByKeyOffset[std::pair(info.BufferKey, info.Offset)] = info.BufferHash;
    restoredBuffers.insert(info.BufferKey);

    for (auto& itTiledResource : command->TiledResources) {
      auto it = m_TiledResourceUpdatesRestored.find(info.BufferKey);
      if (it == m_TiledResourceUpdatesRestored.end() ||
          it->second.find(itTiledResource.second.UpdateId) == it->second.end()) {
        m_ReservedResourcesService.UpdateTileMappings(
            itTiledResource.second, m_CommandListCopyKeys.CommandQueueKey, nullptr);
        m_TiledResourceUpdatesRestored[info.BufferKey].insert(itTiledResource.second.UpdateId);
      }
    }

    uploadBufferOffset += RestoreBuffer(info, uploadBufferOffset);
  }

  RecordExecuteCommandLists(m_CommandListCopyKeys);

  for (auto& it : command->Buffers) {
    if (!it.second->IsMappable && restoredBuffers.find(it.first) != restoredBuffers.end()) {
      ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
      barrierCommand.Key = m_StateService.GetUniqueCommandKey();
      barrierCommand.m_Object.Key = m_CommandListDirectKeys.CommandListKey;
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

  command->Desc->ScratchOpacityMicromapArrayDataKey = m_ScratchResourceKey;
  command->Desc->ScratchOpacityMicromapArrayDataOffset = 0;

  {
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand build;
    build.Key = command->CommandKey;
    build.m_pCommandList.Key = m_CommandListDirectKeys.CommandListKey;
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

  RecordExecuteCommandLists(m_CommandListDirectKeys);
  residencyService.RecordEvict();
}

void AccelerationStructuresBuildService::RecordExecuteCommandLists(const CommandListKeys& keys) {
  ID3D12GraphicsCommandListCloseCommand commandListClose;
  commandListClose.Key = m_StateService.GetUniqueCommandKey();
  commandListClose.m_Object.Key = keys.CommandListKey;
  m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListCloseSerializer(commandListClose));

  ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
  executeCommandLists.Key = m_StateService.GetUniqueCommandKey();
  executeCommandLists.m_Object.Key = keys.CommandQueueKey;
  executeCommandLists.m_NumCommandLists.Value = 1;
  executeCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
  executeCommandLists.m_ppCommandLists.Size = 1;
  executeCommandLists.m_ppCommandLists.Keys.resize(1);
  executeCommandLists.m_ppCommandLists.Keys[0] = keys.CommandListKey;
  m_StateService.GetRecorder().Record(
      ID3D12CommandQueueExecuteCommandListsSerializer(executeCommandLists));

  ID3D12CommandQueueSignalCommand commandQueueSignal;
  commandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
  commandQueueSignal.m_Object.Key = keys.CommandQueueKey;
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
  commandAllocatorReset.m_Object.Key = keys.CommandAllocatorKey;
  m_StateService.GetRecorder().Record(ID3D12CommandAllocatorResetSerializer(commandAllocatorReset));

  ID3D12GraphicsCommandListResetCommand commandListReset;
  commandListReset.Key = m_StateService.GetUniqueCommandKey();
  commandListReset.m_Object.Key = keys.CommandListKey;
  commandListReset.m_pAllocator.Key = keys.CommandAllocatorKey;
  commandListReset.m_pInitialState.Key = 0;
  m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListResetSerializer(commandListReset));
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
    copyBufferRegion.m_Object.Key = m_CommandListCopyKeys.CommandListKey;
    copyBufferRegion.m_pDstBuffer.Key = restoreInfo.BufferKey;
    copyBufferRegion.m_DstOffset.Value = restoreInfo.Offset;
    copyBufferRegion.m_pSrcBuffer.Key = m_UploadBufferKey;
    copyBufferRegion.m_SrcOffset.Value = uploadBufferOffset;
    copyBufferRegion.m_NumBytes.Value = restoreInfo.BufferData->size();
    m_Recorder.Record(ID3D12GraphicsCommandListCopyBufferRegionSerializer(copyBufferRegion));

    return restoreInfo.BufferData->size();
  }
}

void AccelerationStructuresBuildService::InputBufferService::AddBufferRegion(unsigned key,
                                                                             unsigned offset,
                                                                             unsigned size) {
  m_BufferRegionsByInputKey[key].emplace_back(offset, offset + size);
}

void AccelerationStructuresBuildService::InputBufferService::StoreBuffers(
    unsigned commandKey,
    ID3D12GraphicsCommandList* commandList,
    RaytracingAccelerationStructureCommand* command) {
  for (auto& [inputKey, bufferRegions] : m_BufferRegionsByInputKey) {

    std::sort(bufferRegions.begin(), bufferRegions.end(),
              [](const BufferRegion& a, const BufferRegion& b) { return a.Start < b.Start; });

    std::vector<BufferRegion> bufferRegionsMerged;
    for (BufferRegion bufferRegion : bufferRegions) {
      if (bufferRegionsMerged.empty() || bufferRegionsMerged.back().End < bufferRegion.Start) {
        bufferRegionsMerged.push_back(bufferRegion);
      } else {
        bufferRegionsMerged.back().End = std::max(bufferRegionsMerged.back().End, bufferRegion.End);
      }
    }

    for (BufferRegion bufferRegion : bufferRegionsMerged) {
      StoreBuffer(inputKey, bufferRegion.Start, bufferRegion.End - bufferRegion.Start, commandKey,
                  commandList, command);
    }
  }
}

void AccelerationStructuresBuildService::InputBufferService::StoreBuffer(
    unsigned inputKey,
    unsigned inputOffset,
    unsigned size,
    unsigned commandKey,
    ID3D12GraphicsCommandList* commandList,
    RaytracingAccelerationStructureCommand* command) {
  m_StateService.KeepState(inputKey);
  ResourceState* bufferState = static_cast<ResourceState*>(m_StateService.GetState(inputKey));
  bufferState->CurrentState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  if (bufferState->DenyShaderResource) {
    bufferState->CurrentState = D3D12_RESOURCE_STATE_COMMON;
  }

  BarrierState currentState = GetAdjustedCurrentState(
      m_ResourceStateTracker, m_GpuAddressService, commandList,
      bufferState->GpuVirtualAddress + inputOffset,
      static_cast<ID3D12Resource*>(bufferState->Object), inputKey, bufferState->CurrentState);

  m_BufferContentRestore.StoreBuffer(commandList, static_cast<ID3D12Resource*>(bufferState->Object),
                                     inputKey, inputOffset, size, currentState, commandKey,
                                     bufferState->IsMappable);
  command->Buffers[inputKey] = bufferState;
  ReservedResourcesService::TiledResource* tiledResource =
      m_ReservedResourcesService.GetTiledResource(inputKey);
  if (tiledResource) {
    auto it = command->TiledResources.find(inputKey);
    if (it == command->TiledResources.end()) {
      command->TiledResources[inputKey] = *tiledResource;
    }
    for (ReservedResourcesService::Tile& tile : tiledResource->Tiles) {
      if (tile.HeapKey) {
        m_StateService.KeepState(tile.HeapKey);
      }
    }
  }
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

  CommandNode* sourceNode{};
  if (node->Command->SourceKey) {
    auto it = m_CommandByKeyOffset.find({node->Command->SourceKey, node->Command->SourceOffset});
    GITS_ASSERT(it != m_CommandByKeyOffset.end() && it->second);
    sourceNode = it->second;
  }

  // skip intermediate update build command
  if (node->Command->Update) {
    GITS_ASSERT(sourceNode);
    if (sourceNode->Source) {
      sourceNode = sourceNode->Source;
      node->Command->SourceKey = sourceNode->Command->DestKey;
      node->Command->SourceOffset = sourceNode->Command->DestOffset;
      switch (node->Command->Type) {
      case RaytracingAccelerationStructureCommand::CommandType::Build: {
        auto* build =
            static_cast<BuildRaytracingAccelerationStructureCommand*>(node->Command.get());
        build->Desc->SourceAccelerationStructureKey = sourceNode->Command->DestKey;
        build->Desc->SourceAccelerationStructureOffset = sourceNode->Command->DestOffset;
      } break;
      case RaytracingAccelerationStructureCommand::CommandType::NvAPIBuild: {
        auto* build =
            static_cast<NvAPIBuildRaytracingAccelerationStructureExCommand*>(node->Command.get());
        build->Desc->SourceAccelerationStructureKey = sourceNode->Command->DestKey;
        build->Desc->SourceAccelerationStructureOffset = sourceNode->Command->DestOffset;
      } break;
      }
    }
  }

  // handle previous command in the same location
  CommandNode* previousNode{};
  auto it = m_CommandByKeyOffset.find({node->Command->DestKey, node->Command->DestOffset});
  if (it != m_CommandByKeyOffset.end()) {
    previousNode = it->second;
  }
  m_CommandByKeyOffset[{node->Command->DestKey, node->Command->DestOffset}] = node;
  if (previousNode && previousNode != sourceNode && previousNode->Destinations.empty()) {
    m_BufferContentRestore.RemoveBuild(previousNode->Command->CommandKey);
    if (previousNode->Source) {
      previousNode->Source->Destinations.erase(previousNode);
    }
    m_CommandById.erase(previousNode->Id);
  }

  if (sourceNode) {
    sourceNode->Destinations.insert(node);
    node->Source = sourceNode;
  }
}

void AccelerationStructuresBuildService::OptimizationService::ProcessCommands() {
  if (Configurator::Get().directx.features.subcapture.optimize) {
    for (auto& [keyOffset, node] : m_CommandByKeyOffset) {
      if (m_StateService.GetAnalyzerResults().RestoreBlas(keyOffset) || node->Command->TlasBuild) {
        node->Restore = true;
      }
    }

    // mark source nodes for restoration
    for (auto& it : m_CommandById) {
      CommandNode* node = it.second.get();
      if (node->Restore) {
        while (node->Source) {
          node->Source->Restore = true;
          node = node->Source;
        }
      } else {
        if (node->Command->Type == RaytracingAccelerationStructureCommand::CommandType::NvAPIOMM) {
          node->Restore = true;
        }
      }
    }
  }

  // prepare sorted commands
  for (auto& it : m_CommandById) {
    if (it.second->Restore || !Configurator::Get().directx.features.subcapture.optimize) {
      m_RestoreCommands.push_back(it.second.get());
    }
  }
  std::sort(m_RestoreCommands.begin(), m_RestoreCommands.end(),
            [](const CommandNode* a, const CommandNode* b) { return a->Id < b->Id; });
}

void AccelerationStructuresBuildService::OptimizationService::Cleanup() {
  m_CommandsByCommandList.clear();
  m_CommandByKeyOffset.clear();
  m_CommandById.clear();
  m_RestoreCommands.clear();
}

} // namespace DirectX
} // namespace gits
