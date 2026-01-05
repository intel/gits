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
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"
#include "reservedResourcesService.h"
#include "gits.h"
#include "configurationLib.h"
#include "nvapi.h"

namespace gits {
namespace DirectX {

AccelerationStructuresBuildService::AccelerationStructuresBuildService(
    StateTrackingService& stateService,
    SubcaptureRecorder& recorder,
    ReservedResourcesService& reservedResourcesService,
    ResourceStateTracker& resourceStateTracker,
    CapturePlayerGpuAddressService& gpuAddressService)
    : stateService_(stateService),
      recorder_(recorder),
      reservedResourcesService_(reservedResourcesService),
      bufferContentRestore_(stateService),
      resourceStateTracker_(resourceStateTracker),
      gpuAddressService_(gpuAddressService) {
  serializeMode_ = Configurator::Get().directx.features.subcapture.serializeAccelerationStructures;
  restoreTLASes_ = Configurator::Get().directx.features.subcapture.restoreTLASes;
  optimize_ = Configurator::Get().directx.features.subcapture.optimize;
}

void AccelerationStructuresBuildService::buildAccelerationStructure(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (serializeMode_ &&
      c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    return;
  }
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    if (optimize_) {
      if (!stateService_.getAnalyzerResults().restoreTlas(c.key)) {
        return;
      }
    } else {
      if (!recorder_.commandListSubcapture() && !restoreTLASes_) {
        return;
      }
    }
  }
  if (restored_) {
    return;
  }

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = c.pDesc_.value->Inputs;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info{};
  Microsoft::WRL::ComPtr<ID3D12Device5> device;
  HRESULT hr = c.object_.value->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
  if (info.ScratchDataSizeInBytes > maxBuildScratchSpace_) {
    maxBuildScratchSpace_ = info.ScratchDataSizeInBytes;
  }
  if (info.UpdateScratchDataSizeInBytes > maxBuildScratchSpace_) {
    maxBuildScratchSpace_ = info.UpdateScratchDataSizeInBytes;
  }

  BuildRaytracingAccelerationStructureState* state =
      new BuildRaytracingAccelerationStructureState();
  state->commandKey = c.key;
  state->commandListKey = commandListDirectKey_;
  state->stateType = RaytracingAccelerationStructureState::Build;
  state->destKey = c.pDesc_.destAccelerationStructureKey;
  state->destOffset = c.pDesc_.destAccelerationStructureOffset;
  state->sourceKey = c.pDesc_.sourceAccelerationStructureKey;
  state->sourceOffset = c.pDesc_.sourceAccelerationStructureOffset;
  state->update = c.pDesc_.value->Inputs.Flags &
                  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

  if (serializeMode_ &&
      c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    tlases_.insert(std::make_pair(state->destKey, state->destOffset));
  }

  state->desc.reset(
      new PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>(c.pDesc_));

  auto storeBuffer = [&](unsigned inputIndex, unsigned size, D3D12_GPU_VIRTUAL_ADDRESS address) {
    unsigned inputKey = c.pDesc_.inputKeys[inputIndex];
    unsigned inputOffset = c.pDesc_.inputOffsets[inputIndex];
    stateService_.keepState(inputKey);
    ResourceState* bufferState = static_cast<ResourceState*>(stateService_.getState(inputKey));
    D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_STATES trackedState =
        resourceStateTracker_.getResourceState(c.object_.value, inputKey, 0);
    if (trackedState == D3D12_RESOURCE_STATE_GENERIC_READ ||
        trackedState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
      resourceState = trackedState;
    } else if (trackedState != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
      CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
          gpuAddressService_.getResourceInfoByCaptureAddress(address);
      if (resourceInfo && resourceInfo->overlapping()) {
        resourceState = trackedState;
        static bool logged = false;
        if (!logged) {
          LOG_WARNING << "State restore - state of overlapped resource different than expected";
          logged = true;
        }
      }
    }
    bufferContentRestore_.storeBuffer(
        c.object_.value, static_cast<ID3D12Resource*>(bufferState->object), inputKey, inputOffset,
        size, resourceState, c.key, bufferState->isMappable);
    state->buffers[inputKey] = bufferState;
    ReservedResourcesService::TiledResource* tiledResource =
        reservedResourcesService_.getTiledResource(inputKey);
    if (tiledResource) {
      auto it = state->tiledResources.find(inputKey);
      if (it == state->tiledResources.end()) {
        state->tiledResources[inputKey] = *tiledResource;
      }
      for (ReservedResourcesService::Tile& tile : tiledResource->tiles) {
        if (tile.heapKey) {
          stateService_.keepState(tile.heapKey);
        }
      }
    }
  };

  stateService_.keepState(c.pDesc_.destAccelerationStructureKey);

  unsigned inputIndex = 0;
  if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL && inputs.NumDescs) {
    if (inputs.NumDescs) {
      unsigned size = inputs.NumDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
      if (inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        size = inputs.NumDescs * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
      }
      storeBuffer(inputIndex, size, inputs.InstanceDescs);
    }
  } else if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          c.pDesc_.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? c.pDesc_.value->Inputs.pGeometryDescs[i]
              : *c.pDesc_.value->Inputs.ppGeometryDescs[i]);
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        if (desc.Triangles.Transform3x4) {
          unsigned size = sizeof(float) * 3 * 4;
          storeBuffer(inputIndex, size, desc.Triangles.Transform3x4);
        }
        ++inputIndex;
        if (desc.Triangles.IndexBuffer && desc.Triangles.IndexCount) {
          unsigned size = desc.Triangles.IndexCount *
                          (desc.Triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
          storeBuffer(inputIndex, size, desc.Triangles.IndexBuffer);
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
          storeBuffer(inputIndex, size, desc.Triangles.VertexBuffer.StartAddress);
        }
        ++inputIndex;
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        if (desc.AABBs.AABBs.StartAddress && desc.AABBs.AABBCount) {
          unsigned size = desc.AABBs.AABBCount * desc.AABBs.AABBs.StrideInBytes;
          storeBuffer(inputIndex, size, desc.AABBs.AABBs.StartAddress);
        }
        ++inputIndex;
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
        if (desc.OmmTriangles.pTriangles) {
          auto& triangles = *desc.OmmTriangles.pTriangles;
          if (triangles.Transform3x4) {
            unsigned size = sizeof(float) * 3 * 4;
            storeBuffer(inputIndex, size, triangles.Transform3x4);
          }
          ++inputIndex;
          if (triangles.IndexBuffer && triangles.IndexCount) {
            unsigned size =
                triangles.IndexCount * (triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
            storeBuffer(inputIndex, size, triangles.IndexBuffer);
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
            storeBuffer(inputIndex, size, triangles.VertexBuffer.StartAddress);
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
              storeBuffer(inputIndex, size, ommLinkage.OpacityMicromapIndexBuffer.StartAddress);
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
        storeBuffer(inputIndex, totalInputSize, ommDesc.InputBuffer);
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
        storeBuffer(inputIndex, size, ommDesc.PerOmmDescs.StartAddress);
      }
      ++inputIndex;
    }
  }

  statesByCommandList_[c.object_.key].emplace_back(state);
}

void AccelerationStructuresBuildService::copyAccelerationStructure(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (serializeMode_) {
    auto it = tlases_.find(std::make_pair(c.DestAccelerationStructureData_.interfaceKey,
                                          c.DestAccelerationStructureData_.offset));
    if (it == tlases_.end()) {
      return;
    }
  }
  CopyRaytracingAccelerationStructureState* state = new CopyRaytracingAccelerationStructureState();
  state->commandKey = c.key;
  state->commandListKey = commandListDirectKey_;
  state->stateType = RaytracingAccelerationStructureState::Copy;
  state->destAccelerationStructureData = c.DestAccelerationStructureData_.value;
  state->destKey = c.DestAccelerationStructureData_.interfaceKey;
  state->destOffset = c.DestAccelerationStructureData_.offset;
  state->sourceAccelerationStructureData = c.SourceAccelerationStructureData_.value;
  state->sourceKey = c.SourceAccelerationStructureData_.interfaceKey;
  state->sourceOffset = c.SourceAccelerationStructureData_.offset;
  state->mode = c.Mode_.value;

  stateService_.keepState(c.DestAccelerationStructureData_.interfaceKey);

  statesByCommandList_[c.object_.key].emplace_back(state);
}

void AccelerationStructuresBuildService::nvapiBuildAccelerationStructureEx(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  const NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX* pDesc = c.pParams.value->pDesc;
  if (!restoreTLASes_ &&
      pDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }

  if (restored_) {
    return;
  }

  NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS_EX inputs = pDesc->inputs;

  Microsoft::WRL::ComPtr<ID3D12Device5> device;
  HRESULT hr = c.pCommandList_.value->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  NVAPI_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_EX_PARAMS params{};
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info{};
  params.version = NVAPI_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_EX_PARAMS_VER;
  params.pDesc = &inputs;
  params.pInfo = &info;
  NvAPI_D3D12_GetRaytracingAccelerationStructurePrebuildInfoEx(device.Get(), &params);
  if (info.ScratchDataSizeInBytes > maxBuildScratchSpace_) {
    maxBuildScratchSpace_ = info.ScratchDataSizeInBytes;
  }
  if (info.UpdateScratchDataSizeInBytes > maxBuildScratchSpace_) {
    maxBuildScratchSpace_ = info.UpdateScratchDataSizeInBytes;
  }

  NvAPIBuildRaytracingAccelerationStructureExState* state =
      new NvAPIBuildRaytracingAccelerationStructureExState();
  state->commandKey = c.key;
  state->commandListKey = commandListDirectKey_;
  state->stateType = RaytracingAccelerationStructureState::NvAPIBuild;
  state->destKey = c.pParams.destAccelerationStructureKey;
  state->destOffset = c.pParams.destAccelerationStructureOffset;
  state->sourceKey = c.pParams.sourceAccelerationStructureKey;
  state->sourceOffset = c.pParams.sourceAccelerationStructureOffset;
  state->update = pDesc->inputs.flags &
                  NVAPI_D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE_EX;

  if (serializeMode_ &&
      pDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    tlases_.insert(std::make_pair(state->destKey, state->destOffset));
  }

  state->desc.reset(
      new PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>(c.pParams));

  auto storeBuffer = [&](unsigned inputIndex, unsigned size, D3D12_GPU_VIRTUAL_ADDRESS address) {
    unsigned inputKey = c.pParams.inputKeys[inputIndex];
    unsigned inputOffset = c.pParams.inputOffsets[inputIndex];
    stateService_.keepState(inputKey);
    ResourceState* bufferState = static_cast<ResourceState*>(stateService_.getState(inputKey));
    D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_STATES trackedState =
        resourceStateTracker_.getResourceState(c.pCommandList_.value, inputKey, 0);
    if (trackedState == D3D12_RESOURCE_STATE_GENERIC_READ ||
        trackedState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
      resourceState = trackedState;
    } else if (trackedState != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
      CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
          gpuAddressService_.getResourceInfoByCaptureAddress(address);
      if (resourceInfo && resourceInfo->overlapping()) {
        resourceState = trackedState;
        static bool logged = false;
        if (!logged) {
          LOG_WARNING << "State restore - state of overlapped resource different than expected";
          logged = true;
        }
      }
    }
    bufferContentRestore_.storeBuffer(
        c.pCommandList_.value, static_cast<ID3D12Resource*>(bufferState->object), inputKey,
        inputOffset, size, resourceState, c.key, bufferState->isMappable);
    state->buffers[inputKey] = bufferState;
    ReservedResourcesService::TiledResource* tiledResource =
        reservedResourcesService_.getTiledResource(inputKey);
    if (tiledResource) {
      auto it = state->tiledResources.find(inputKey);
      if (it == state->tiledResources.end()) {
        state->tiledResources[inputKey] = *tiledResource;
      }
      for (ReservedResourcesService::Tile& tile : tiledResource->tiles) {
        if (tile.heapKey) {
          stateService_.keepState(tile.heapKey);
        }
      }
    }
  };

  stateService_.keepState(c.pParams.destAccelerationStructureKey);

  unsigned inputIndex = 0;
  if (inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL && inputs.numDescs) {
    if (inputs.numDescs) {
      unsigned size = inputs.numDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
      storeBuffer(inputIndex, size, inputs.instanceDescs);
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
          storeBuffer(inputIndex, size, desc.triangles.Transform3x4);
        }
        ++inputIndex;
        if (desc.triangles.IndexBuffer && desc.triangles.IndexCount) {
          unsigned size = desc.triangles.IndexCount *
                          (desc.triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
          storeBuffer(inputIndex, size, desc.triangles.IndexBuffer);
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
          storeBuffer(inputIndex, size, desc.triangles.VertexBuffer.StartAddress);
        }
        ++inputIndex;
      } else if (desc.type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        if (desc.aabbs.AABBs.StartAddress && desc.aabbs.AABBCount) {
          unsigned size = desc.aabbs.AABBCount * desc.aabbs.AABBs.StrideInBytes;
          storeBuffer(inputIndex, size, desc.aabbs.AABBs.StartAddress);
        }
        ++inputIndex;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
        if (desc.ommTriangles.triangles.Transform3x4) {
          unsigned size = sizeof(float) * 3 * 4;
          storeBuffer(inputIndex, size, desc.ommTriangles.triangles.Transform3x4);
        }
        ++inputIndex;
        if (desc.ommTriangles.triangles.IndexBuffer && desc.ommTriangles.triangles.IndexCount) {
          unsigned size = desc.ommTriangles.triangles.IndexCount *
                          (desc.ommTriangles.triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
          storeBuffer(inputIndex, size, desc.ommTriangles.triangles.IndexBuffer);
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
          storeBuffer(inputIndex, size, desc.ommTriangles.triangles.VertexBuffer.StartAddress);
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
            storeBuffer(inputIndex, size,
                        desc.ommTriangles.ommAttachment.opacityMicromapIndexBuffer.StartAddress);
          }
        }
        ++inputIndex;
        ++inputIndex;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
        if (desc.dmmTriangles.triangles.Transform3x4) {
          unsigned size = sizeof(float) * 3 * 4;
          storeBuffer(inputIndex, size, desc.dmmTriangles.triangles.Transform3x4);
        }
        ++inputIndex;
        if (desc.dmmTriangles.triangles.IndexBuffer && desc.dmmTriangles.triangles.IndexCount) {
          unsigned size = desc.dmmTriangles.triangles.IndexCount *
                          (desc.dmmTriangles.triangles.IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
          storeBuffer(inputIndex, size, desc.dmmTriangles.triangles.IndexBuffer);
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
          storeBuffer(inputIndex, size, desc.dmmTriangles.triangles.VertexBuffer.StartAddress);
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
            storeBuffer(inputIndex, size,
                        desc.dmmTriangles.dmmAttachment.triangleMicromapIndexBuffer.StartAddress);
          }
        }
        ++inputIndex;
        if (desc.dmmTriangles.dmmAttachment.trianglePrimitiveFlagsBuffer.StartAddress) {
          unsigned size = desc.dmmTriangles.triangles.VertexCount;
          storeBuffer(inputIndex, size,
                      desc.dmmTriangles.dmmAttachment.trianglePrimitiveFlagsBuffer.StartAddress);
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
          storeBuffer(inputIndex, size,
                      desc.dmmTriangles.dmmAttachment.vertexBiasAndScaleBuffer.StartAddress);
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
          storeBuffer(inputIndex, size,
                      desc.dmmTriangles.dmmAttachment.vertexDisplacementVectorBuffer.StartAddress);
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
          storeBuffer(inputIndex, size, desc.spheres.vertexPositionBuffer.StartAddress);
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
          storeBuffer(inputIndex, size, desc.spheres.vertexRadiusBuffer.StartAddress);
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
          storeBuffer(inputIndex, size, desc.spheres.indexBuffer.StartAddress);
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
          storeBuffer(inputIndex, size, desc.lss.vertexPositionBuffer.StartAddress);
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
          storeBuffer(inputIndex, size, desc.lss.vertexRadiusBuffer.StartAddress);
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
          storeBuffer(inputIndex, size, desc.lss.indexBuffer.StartAddress);
        }
        ++inputIndex;
      }
    }
  }

  statesByCommandList_[c.pCommandList_.key].emplace_back(state);
}

void AccelerationStructuresBuildService::nvapiBuildOpacityMicromapArray(
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (restored_) {
    return;
  }

  const NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC* pDesc = c.pParams.value->pDesc;

  NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_INPUTS inputs = pDesc->inputs;

  Microsoft::WRL::ComPtr<ID3D12Device5> device;
  HRESULT hr = c.pCommandList_.value->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  NVAPI_GET_RAYTRACING_OPACITY_MICROMAP_ARRAY_PREBUILD_INFO_PARAMS params{};
  NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_PREBUILD_INFO info{};
  params.version = NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS_VER;
  params.pDesc = &inputs;
  params.pInfo = &info;
  NvAPI_D3D12_GetRaytracingOpacityMicromapArrayPrebuildInfo(device.Get(), &params);
  if (info.scratchDataSizeInBytes > maxBuildScratchSpace_) {
    maxBuildScratchSpace_ = info.scratchDataSizeInBytes;
  }

  NvAPIBuildRaytracingOpacityMicromapArrayState* state =
      new NvAPIBuildRaytracingOpacityMicromapArrayState();
  state->commandKey = c.key;
  state->commandListKey = commandListDirectKey_;
  state->stateType = RaytracingAccelerationStructureState::NvAPIOMM;
  state->destKey = c.pParams.destOpacityMicromapArrayDataKey;
  state->destOffset = c.pParams.destOpacityMicromapArrayDataOffset;

  state->desc.reset(
      new PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>(c.pParams));

  auto storeBuffer = [&](unsigned inputKey, unsigned inputOffset, unsigned size,
                         D3D12_GPU_VIRTUAL_ADDRESS address) {
    stateService_.keepState(inputKey);
    ResourceState* bufferState = static_cast<ResourceState*>(stateService_.getState(inputKey));
    D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_STATES trackedState =
        resourceStateTracker_.getResourceState(c.pCommandList_.value, inputKey, 0);
    if (trackedState == D3D12_RESOURCE_STATE_GENERIC_READ ||
        trackedState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
      resourceState = trackedState;
    } else if (trackedState != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
      CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
          gpuAddressService_.getResourceInfoByCaptureAddress(address);
      if (resourceInfo && resourceInfo->overlapping()) {
        resourceState = trackedState;
        static bool logged = false;
        if (!logged) {
          LOG_WARNING << "State restore - state of overlapped resource different than expected";
          logged = true;
        }
      }
    }
    bufferContentRestore_.storeBuffer(
        c.pCommandList_.value, static_cast<ID3D12Resource*>(bufferState->object), inputKey,
        inputOffset, size, resourceState, c.key, bufferState->isMappable);
    state->buffers[inputKey] = bufferState;
    ReservedResourcesService::TiledResource* tiledResource =
        reservedResourcesService_.getTiledResource(inputKey);
    if (tiledResource) {
      auto it = state->tiledResources.find(inputKey);
      if (it == state->tiledResources.end()) {
        state->tiledResources[inputKey] = *tiledResource;
      }
      for (ReservedResourcesService::Tile& tile : tiledResource->tiles) {
        if (tile.heapKey) {
          stateService_.keepState(tile.heapKey);
        }
      }
    }
  };

  stateService_.keepState(c.pParams.destOpacityMicromapArrayDataKey);

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
      storeBuffer(c.pParams.inputBufferKey, c.pParams.inputBufferOffset, inputSize,
                  pDesc->inputs.inputBuffer);
    }
    if (pDesc->inputs.perOMMDescs.StartAddress) {
      unsigned stride = pDesc->inputs.perOMMDescs.StrideInBytes;
      if (!stride) {
        stride = sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT);
      }
      storeBuffer(c.pParams.perOMMDescsKey, c.pParams.perOMMDescsOffset, stride * ommCount,
                  pDesc->inputs.perOMMDescs.StartAddress);
    }
  }

  statesByCommandList_[c.pCommandList_.key].emplace_back(state);
}

void AccelerationStructuresBuildService::restoreAccelerationStructures() {
  if (statesById_.empty()) {
    return;
  }

  completeSourcesFromAnalysis();
  removeSourcesWithoutDestinations();
  optimize();

  bufferContentRestore_.waitUntilDumped();

  initUploadBuffer();

  {
    commandQueueCopyKey_ = stateService_.getUniqueObjectKey();
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    ID3D12DeviceCreateCommandQueueCommand createCommandQueue;
    createCommandQueue.key = stateService_.getUniqueCommandKey();
    createCommandQueue.object_.key = deviceKey_;
    createCommandQueue.pDesc_.value = &commandQueueDesc;
    createCommandQueue.riid_.value = IID_ID3D12CommandQueue;
    createCommandQueue.ppCommandQueue_.key = commandQueueCopyKey_;
    stateService_.getRecorder().record(
        new ID3D12DeviceCreateCommandQueueWriter(createCommandQueue));

    commandAllocatorCopyKey_ = stateService_.getUniqueObjectKey();
    ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
    createCommandAllocator.key = stateService_.getUniqueCommandKey();
    createCommandAllocator.object_.key = deviceKey_;
    createCommandAllocator.type_.value = D3D12_COMMAND_LIST_TYPE_COPY;
    createCommandAllocator.riid_.value = IID_ID3D12CommandAllocator;
    createCommandAllocator.ppCommandAllocator_.key = commandAllocatorCopyKey_;
    stateService_.getRecorder().record(
        new ID3D12DeviceCreateCommandAllocatorWriter(createCommandAllocator));

    commandListCopyKey_ = stateService_.getUniqueObjectKey();
    ID3D12DeviceCreateCommandListCommand createCommandList;
    createCommandList.key = stateService_.getUniqueCommandKey();
    createCommandList.object_.key = deviceKey_;
    createCommandList.nodeMask_.value = 0;
    createCommandList.pCommandAllocator_.key = createCommandAllocator.ppCommandAllocator_.key;
    createCommandList.type_.value = D3D12_COMMAND_LIST_TYPE_COPY;
    createCommandList.pInitialState_.value = nullptr;
    createCommandList.riid_.value = IID_ID3D12CommandList;
    createCommandList.ppCommandList_.key = commandListCopyKey_;
    stateService_.getRecorder().record(new ID3D12DeviceCreateCommandListWriter(createCommandList));
  }
  {
    commandQueueDirectKey_ = stateService_.getUniqueObjectKey();
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ID3D12DeviceCreateCommandQueueCommand createCommandQueue;
    createCommandQueue.key = stateService_.getUniqueCommandKey();
    createCommandQueue.object_.key = deviceKey_;
    createCommandQueue.pDesc_.value = &commandQueueDesc;
    createCommandQueue.riid_.value = IID_ID3D12CommandQueue;
    createCommandQueue.ppCommandQueue_.key = commandQueueDirectKey_;
    stateService_.getRecorder().record(
        new ID3D12DeviceCreateCommandQueueWriter(createCommandQueue));

    commandAllocatorDirectKey_ = stateService_.getUniqueObjectKey();
    ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
    createCommandAllocator.key = stateService_.getUniqueCommandKey();
    createCommandAllocator.object_.key = deviceKey_;
    createCommandAllocator.type_.value = D3D12_COMMAND_LIST_TYPE_DIRECT;
    createCommandAllocator.riid_.value = IID_ID3D12CommandAllocator;
    createCommandAllocator.ppCommandAllocator_.key = commandAllocatorDirectKey_;
    stateService_.getRecorder().record(
        new ID3D12DeviceCreateCommandAllocatorWriter(createCommandAllocator));

    commandListDirectKey_ = stateService_.getUniqueObjectKey();
    ID3D12DeviceCreateCommandListCommand createCommandList;
    createCommandList.key = stateService_.getUniqueCommandKey();
    createCommandList.object_.key = deviceKey_;
    createCommandList.nodeMask_.value = 0;
    createCommandList.pCommandAllocator_.key = createCommandAllocator.ppCommandAllocator_.key;
    createCommandList.type_.value = D3D12_COMMAND_LIST_TYPE_DIRECT;
    createCommandList.pInitialState_.value = nullptr;
    createCommandList.riid_.value = IID_ID3D12CommandList;
    createCommandList.ppCommandList_.key = commandListDirectKey_;
    stateService_.getRecorder().record(new ID3D12DeviceCreateCommandListWriter(createCommandList));
  }
  {
    fenceKey_ = stateService_.getUniqueObjectKey();
    ID3D12DeviceCreateFenceCommand createFence;
    createFence.key = stateService_.getUniqueCommandKey();
    createFence.object_.key = deviceKey_;
    createFence.InitialValue_.value = 0;
    createFence.Flags_.value = D3D12_FENCE_FLAG_NONE;
    createFence.riid_.value = IID_ID3D12Fence;
    createFence.ppFence_.key = fenceKey_;
    stateService_.getRecorder().record(new ID3D12DeviceCreateFenceWriter(createFence));
  }

  unsigned scratchResourceKey = stateService_.getUniqueObjectKey();

  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
  heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProperties.CreationNodeMask = 1;
  heapProperties.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = maxBuildScratchSpace_;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  ID3D12DeviceCreateCommittedResourceCommand createScratch;
  createScratch.key = stateService_.getUniqueCommandKey();
  createScratch.object_.key = deviceKey_;
  createScratch.pHeapProperties_.value = &heapProperties;
  createScratch.HeapFlags_.value = D3D12_HEAP_FLAG_NONE;
  createScratch.pDesc_.value = &resourceDesc;
  createScratch.InitialResourceState_.value = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  createScratch.pOptimizedClearValue_.value = nullptr;
  createScratch.riidResource_.value = IID_ID3D12Resource;
  createScratch.ppvResource_.key = scratchResourceKey;
  recorder_.record(new ID3D12DeviceCreateCommittedResourceWriter(createScratch));

  ID3D12ResourceGetGPUVirtualAddressCommand getAddress{};
  getAddress.key = stateService_.getUniqueCommandKey();
  getAddress.object_.key = scratchResourceKey;
  recorder_.record(new ID3D12ResourceGetGPUVirtualAddressWriter(getAddress));

  std::map<std::pair<unsigned, unsigned>, uint64_t> bufferHashesByKeyOffset;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> tiledResourceUpdatesRestored;
  for (auto& itState : statesById_) {
    if (itState.second->stateType == RaytracingAccelerationStructureState::Build) {
      BuildRaytracingAccelerationStructureState* state =
          static_cast<BuildRaytracingAccelerationStructureState*>(itState.second);

      std::unordered_set<unsigned> restoredBuffers;
      size_t uploadBufferOffset{};

      std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
          bufferContentRestore_.getRestoreInfos(state->commandKey);
      for (AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
        auto itHash = bufferHashesByKeyOffset.find(std::pair(info.bufferKey, info.offset));
        if (itHash != bufferHashesByKeyOffset.end() && itHash->second == info.bufferHash) {
          continue;
        }
        bufferHashesByKeyOffset[std::pair(info.bufferKey, info.offset)] = info.bufferHash;
        restoredBuffers.insert(info.bufferKey);

        for (auto& itTiledResource : state->tiledResources) {
          auto it = tiledResourceUpdatesRestored.find(info.bufferKey);
          if (it == tiledResourceUpdatesRestored.end() ||
              it->second.find(itTiledResource.second.updateId) == it->second.end()) {
            reservedResourcesService_.updateTileMappings(itTiledResource.second,
                                                         commandQueueCopyKey_, nullptr);
            tiledResourceUpdatesRestored[info.bufferKey].insert(itTiledResource.second.updateId);
          }
        }

        uploadBufferOffset += restoreBuffer(info, uploadBufferOffset);
      }

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.key = stateService_.getUniqueCommandKey();
        commandListClose.object_.key = commandListCopyKey_;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListCloseWriter(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
        executeCommandLists.key = stateService_.getUniqueCommandKey();
        executeCommandLists.object_.key = commandQueueCopyKey_;
        executeCommandLists.NumCommandLists_.value = 1;
        executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
        executeCommandLists.ppCommandLists_.size = 1;
        executeCommandLists.ppCommandLists_.keys.resize(1);
        executeCommandLists.ppCommandLists_.keys[0] = commandListCopyKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

        ID3D12CommandQueueSignalCommand commandQueueSignal;
        commandQueueSignal.key = stateService_.getUniqueCommandKey();
        commandQueueSignal.object_.key = commandQueueCopyKey_;
        commandQueueSignal.pFence_.key = fenceKey_;
        commandQueueSignal.Value_.value = ++recordedFenceValue_;
        stateService_.getRecorder().record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.key = stateService_.getUniqueCommandKey();
        getCompletedValue.object_.key = fenceKey_;
        getCompletedValue.result_.value = recordedFenceValue_;
        stateService_.getRecorder().record(
            new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.key = stateService_.getUniqueCommandKey();
        commandAllocatorReset.object_.key = commandAllocatorCopyKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandAllocatorResetWriter(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand commandListReset;
        commandListReset.key = stateService_.getUniqueCommandKey();
        commandListReset.object_.key = commandListCopyKey_;
        commandListReset.pAllocator_.key = commandAllocatorCopyKey_;
        commandListReset.pInitialState_.key = 0;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListResetWriter(commandListReset));
      }

      for (auto& it : state->buffers) {
        if (!it.second->isMappable && restoredBuffers.find(it.first) != restoredBuffers.end()) {
          ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
          barrierCommand.key = stateService_.getUniqueCommandKey();
          barrierCommand.object_.key = commandListDirectKey_;
          barrierCommand.NumBarriers_.value = 1;
          D3D12_RESOURCE_BARRIER barrier{};
          barrierCommand.pBarriers_.value = &barrier;
          barrierCommand.pBarriers_.size = 1;
          barrierCommand.pBarriers_.resourceKeys.resize(1);
          barrierCommand.pBarriers_.resourceAfterKeys.resize(1);
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
          barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
          barrierCommand.pBarriers_.resourceKeys[0] = it.first;
          stateService_.getRecorder().record(
              new ID3D12GraphicsCommandListResourceBarrierWriter(barrierCommand));
        }
      }

      state->desc->scratchAccelerationStructureKey = scratchResourceKey;
      state->desc->scratchAccelerationStructureOffset = 0;

      {
        ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand build;
        build.key = state->commandKey;
        build.object_.key = commandListDirectKey_;
        build.pDesc_.value = state->desc->value;
        build.pDesc_.destAccelerationStructureKey = state->desc->destAccelerationStructureKey;
        build.pDesc_.destAccelerationStructureOffset = state->desc->destAccelerationStructureOffset;
        build.pDesc_.sourceAccelerationStructureKey = state->desc->sourceAccelerationStructureKey;
        build.pDesc_.sourceAccelerationStructureOffset =
            state->desc->sourceAccelerationStructureOffset;
        build.pDesc_.scratchAccelerationStructureKey = state->desc->scratchAccelerationStructureKey;
        build.pDesc_.scratchAccelerationStructureOffset =
            state->desc->scratchAccelerationStructureOffset;
        build.pDesc_.inputKeys = state->desc->inputKeys;
        build.pDesc_.inputOffsets = state->desc->inputOffsets;
        build.NumPostbuildInfoDescs_.value = 0;
        build.pPostbuildInfoDescs_.value = nullptr;
        recorder_.record(
            new ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureWriter(build));
      }

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.key = stateService_.getUniqueCommandKey();
        commandListClose.object_.key = commandListDirectKey_;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListCloseWriter(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
        executeCommandLists.key = stateService_.getUniqueCommandKey();
        executeCommandLists.object_.key = commandQueueDirectKey_;
        executeCommandLists.NumCommandLists_.value = 1;
        executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
        executeCommandLists.ppCommandLists_.size = 1;
        executeCommandLists.ppCommandLists_.keys.resize(1);
        executeCommandLists.ppCommandLists_.keys[0] = commandListDirectKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

        ID3D12CommandQueueSignalCommand commandQueueSignal;
        commandQueueSignal.key = stateService_.getUniqueCommandKey();
        commandQueueSignal.object_.key = commandQueueDirectKey_;
        commandQueueSignal.pFence_.key = fenceKey_;
        commandQueueSignal.Value_.value = ++recordedFenceValue_;
        stateService_.getRecorder().record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.key = stateService_.getUniqueCommandKey();
        getCompletedValue.object_.key = fenceKey_;
        getCompletedValue.result_.value = recordedFenceValue_;
        stateService_.getRecorder().record(
            new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.key = stateService_.getUniqueCommandKey();
        commandAllocatorReset.object_.key = commandAllocatorDirectKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandAllocatorResetWriter(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand commandListReset;
        commandListReset.key = stateService_.getUniqueCommandKey();
        commandListReset.object_.key = commandListDirectKey_;
        commandListReset.pAllocator_.key = commandAllocatorDirectKey_;
        commandListReset.pInitialState_.key = 0;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListResetWriter(commandListReset));
      }
    } else if (itState.second->stateType == RaytracingAccelerationStructureState::Copy) {
      CopyRaytracingAccelerationStructureState* state =
          static_cast<CopyRaytracingAccelerationStructureState*>(itState.second);

      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand copy;
      copy.key = state->commandKey;
      copy.object_.key = commandListDirectKey_;
      copy.DestAccelerationStructureData_.value = state->destAccelerationStructureData;
      copy.DestAccelerationStructureData_.interfaceKey = state->destKey;
      copy.DestAccelerationStructureData_.offset = state->destOffset;
      copy.SourceAccelerationStructureData_.value = state->sourceAccelerationStructureData;
      copy.SourceAccelerationStructureData_.interfaceKey = state->sourceKey;
      copy.SourceAccelerationStructureData_.offset = state->sourceOffset;
      copy.Mode_.value = state->mode;
      stateService_.getRecorder().record(
          new ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureWriter(copy));

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.key = stateService_.getUniqueCommandKey();
        commandListClose.object_.key = commandListDirectKey_;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListCloseWriter(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
        executeCommandLists.key = stateService_.getUniqueCommandKey();
        executeCommandLists.object_.key = commandQueueDirectKey_;
        executeCommandLists.NumCommandLists_.value = 1;
        executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
        executeCommandLists.ppCommandLists_.size = 1;
        executeCommandLists.ppCommandLists_.keys.resize(1);
        executeCommandLists.ppCommandLists_.keys[0] = commandListDirectKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

        ID3D12CommandQueueSignalCommand commandQueueSignal;
        commandQueueSignal.key = stateService_.getUniqueCommandKey();
        commandQueueSignal.object_.key = commandQueueDirectKey_;
        commandQueueSignal.pFence_.key = fenceKey_;
        commandQueueSignal.Value_.value = ++recordedFenceValue_;
        stateService_.getRecorder().record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.key = stateService_.getUniqueCommandKey();
        getCompletedValue.object_.key = fenceKey_;
        getCompletedValue.result_.value = recordedFenceValue_;
        stateService_.getRecorder().record(
            new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.key = stateService_.getUniqueCommandKey();
        commandAllocatorReset.object_.key = commandAllocatorDirectKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandAllocatorResetWriter(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand commandListReset;
        commandListReset.key = stateService_.getUniqueCommandKey();
        commandListReset.object_.key = commandListDirectKey_;
        commandListReset.pAllocator_.key = commandAllocatorDirectKey_;
        commandListReset.pInitialState_.key = 0;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListResetWriter(commandListReset));
      }
    } else if (itState.second->stateType == RaytracingAccelerationStructureState::NvAPIBuild) {
      NvAPIBuildRaytracingAccelerationStructureExState* state =
          static_cast<NvAPIBuildRaytracingAccelerationStructureExState*>(itState.second);

      std::unordered_set<unsigned> restoredBuffers;
      size_t uploadBufferOffset{};

      std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
          bufferContentRestore_.getRestoreInfos(state->commandKey);
      for (AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
        auto itHash = bufferHashesByKeyOffset.find(std::pair(info.bufferKey, info.offset));
        if (itHash != bufferHashesByKeyOffset.end() && itHash->second == info.bufferHash) {
          continue;
        }
        bufferHashesByKeyOffset[std::pair(info.bufferKey, info.offset)] = info.bufferHash;
        restoredBuffers.insert(info.bufferKey);

        for (auto& itTiledResource : state->tiledResources) {
          auto it = tiledResourceUpdatesRestored.find(info.bufferKey);
          if (it == tiledResourceUpdatesRestored.end() ||
              it->second.find(itTiledResource.second.updateId) == it->second.end()) {
            reservedResourcesService_.updateTileMappings(itTiledResource.second,
                                                         commandQueueCopyKey_, nullptr);
            tiledResourceUpdatesRestored[info.bufferKey].insert(itTiledResource.second.updateId);
          }
        }

        uploadBufferOffset += restoreBuffer(info, uploadBufferOffset);
      }

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.key = stateService_.getUniqueCommandKey();
        commandListClose.object_.key = commandListCopyKey_;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListCloseWriter(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
        executeCommandLists.key = stateService_.getUniqueCommandKey();
        executeCommandLists.object_.key = commandQueueCopyKey_;
        executeCommandLists.NumCommandLists_.value = 1;
        executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
        executeCommandLists.ppCommandLists_.size = 1;
        executeCommandLists.ppCommandLists_.keys.resize(1);
        executeCommandLists.ppCommandLists_.keys[0] = commandListCopyKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

        ID3D12CommandQueueSignalCommand commandQueueSignal;
        commandQueueSignal.key = stateService_.getUniqueCommandKey();
        commandQueueSignal.object_.key = commandQueueCopyKey_;
        commandQueueSignal.pFence_.key = fenceKey_;
        commandQueueSignal.Value_.value = ++recordedFenceValue_;
        stateService_.getRecorder().record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.key = stateService_.getUniqueCommandKey();
        getCompletedValue.object_.key = fenceKey_;
        getCompletedValue.result_.value = recordedFenceValue_;
        stateService_.getRecorder().record(
            new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.key = stateService_.getUniqueCommandKey();
        commandAllocatorReset.object_.key = commandAllocatorCopyKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandAllocatorResetWriter(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand commandListReset;
        commandListReset.key = stateService_.getUniqueCommandKey();
        commandListReset.object_.key = commandListCopyKey_;
        commandListReset.pAllocator_.key = commandAllocatorCopyKey_;
        commandListReset.pInitialState_.key = 0;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListResetWriter(commandListReset));
      }

      for (auto& it : state->buffers) {
        if (!it.second->isMappable && restoredBuffers.find(it.first) != restoredBuffers.end()) {
          ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
          barrierCommand.key = stateService_.getUniqueCommandKey();
          barrierCommand.object_.key = commandListDirectKey_;
          barrierCommand.NumBarriers_.value = 1;
          D3D12_RESOURCE_BARRIER barrier{};
          barrierCommand.pBarriers_.value = &barrier;
          barrierCommand.pBarriers_.size = 1;
          barrierCommand.pBarriers_.resourceKeys.resize(1);
          barrierCommand.pBarriers_.resourceAfterKeys.resize(1);
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
          barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
          barrierCommand.pBarriers_.resourceKeys[0] = it.first;
          stateService_.getRecorder().record(
              new ID3D12GraphicsCommandListResourceBarrierWriter(barrierCommand));
        }
      }

      state->desc->scratchAccelerationStructureKey = scratchResourceKey;
      state->desc->scratchAccelerationStructureOffset = 0;

      {
        NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand build;
        build.key = state->commandKey;
        build.pCommandList_.key = commandListDirectKey_;
        build.pParams.value = state->desc->value;
        build.pParams.destAccelerationStructureKey = state->desc->destAccelerationStructureKey;
        build.pParams.destAccelerationStructureOffset =
            state->desc->destAccelerationStructureOffset;
        build.pParams.sourceAccelerationStructureKey = state->desc->sourceAccelerationStructureKey;
        build.pParams.sourceAccelerationStructureOffset =
            state->desc->sourceAccelerationStructureOffset;
        build.pParams.scratchAccelerationStructureKey =
            state->desc->scratchAccelerationStructureKey;
        build.pParams.scratchAccelerationStructureOffset =
            state->desc->scratchAccelerationStructureOffset;
        build.pParams.inputKeys = state->desc->inputKeys;
        build.pParams.inputOffsets = state->desc->inputOffsets;
        build.pParams.value->numPostbuildInfoDescs = 0;
        build.pParams.value->pPostbuildInfoDescs = nullptr;
        recorder_.record(new NvAPI_D3D12_BuildRaytracingAccelerationStructureExWriter(build));
      }

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.key = stateService_.getUniqueCommandKey();
        commandListClose.object_.key = commandListDirectKey_;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListCloseWriter(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
        executeCommandLists.key = stateService_.getUniqueCommandKey();
        executeCommandLists.object_.key = commandQueueDirectKey_;
        executeCommandLists.NumCommandLists_.value = 1;
        executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
        executeCommandLists.ppCommandLists_.size = 1;
        executeCommandLists.ppCommandLists_.keys.resize(1);
        executeCommandLists.ppCommandLists_.keys[0] = commandListDirectKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

        ID3D12CommandQueueSignalCommand commandQueueSignal;
        commandQueueSignal.key = stateService_.getUniqueCommandKey();
        commandQueueSignal.object_.key = commandQueueDirectKey_;
        commandQueueSignal.pFence_.key = fenceKey_;
        commandQueueSignal.Value_.value = ++recordedFenceValue_;
        stateService_.getRecorder().record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.key = stateService_.getUniqueCommandKey();
        getCompletedValue.object_.key = fenceKey_;
        getCompletedValue.result_.value = recordedFenceValue_;
        stateService_.getRecorder().record(
            new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.key = stateService_.getUniqueCommandKey();
        commandAllocatorReset.object_.key = commandAllocatorDirectKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandAllocatorResetWriter(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand commandListReset;
        commandListReset.key = stateService_.getUniqueCommandKey();
        commandListReset.object_.key = commandListDirectKey_;
        commandListReset.pAllocator_.key = commandAllocatorDirectKey_;
        commandListReset.pInitialState_.key = 0;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListResetWriter(commandListReset));
      }
    } else if (itState.second->stateType == RaytracingAccelerationStructureState::NvAPIOMM) {
      NvAPIBuildRaytracingOpacityMicromapArrayState* state =
          static_cast<NvAPIBuildRaytracingOpacityMicromapArrayState*>(itState.second);

      std::unordered_set<unsigned> restoredBuffers;
      size_t uploadBufferOffset{};

      std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
          bufferContentRestore_.getRestoreInfos(state->commandKey);
      for (AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
        auto itHash = bufferHashesByKeyOffset.find(std::pair(info.bufferKey, info.offset));
        if (itHash != bufferHashesByKeyOffset.end() && itHash->second == info.bufferHash) {
          continue;
        }
        bufferHashesByKeyOffset[std::pair(info.bufferKey, info.offset)] = info.bufferHash;
        restoredBuffers.insert(info.bufferKey);

        for (auto& itTiledResource : state->tiledResources) {
          auto it = tiledResourceUpdatesRestored.find(info.bufferKey);
          if (it == tiledResourceUpdatesRestored.end() ||
              it->second.find(itTiledResource.second.updateId) == it->second.end()) {
            reservedResourcesService_.updateTileMappings(itTiledResource.second,
                                                         commandQueueCopyKey_, nullptr);
            tiledResourceUpdatesRestored[info.bufferKey].insert(itTiledResource.second.updateId);
          }
        }

        uploadBufferOffset += restoreBuffer(info, uploadBufferOffset);
      }

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.key = stateService_.getUniqueCommandKey();
        commandListClose.object_.key = commandListCopyKey_;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListCloseWriter(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
        executeCommandLists.key = stateService_.getUniqueCommandKey();
        executeCommandLists.object_.key = commandQueueCopyKey_;
        executeCommandLists.NumCommandLists_.value = 1;
        executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
        executeCommandLists.ppCommandLists_.size = 1;
        executeCommandLists.ppCommandLists_.keys.resize(1);
        executeCommandLists.ppCommandLists_.keys[0] = commandListCopyKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

        ID3D12CommandQueueSignalCommand commandQueueSignal;
        commandQueueSignal.key = stateService_.getUniqueCommandKey();
        commandQueueSignal.object_.key = commandQueueCopyKey_;
        commandQueueSignal.pFence_.key = fenceKey_;
        commandQueueSignal.Value_.value = ++recordedFenceValue_;
        stateService_.getRecorder().record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.key = stateService_.getUniqueCommandKey();
        getCompletedValue.object_.key = fenceKey_;
        getCompletedValue.result_.value = recordedFenceValue_;
        stateService_.getRecorder().record(
            new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.key = stateService_.getUniqueCommandKey();
        commandAllocatorReset.object_.key = commandAllocatorCopyKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandAllocatorResetWriter(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand commandListReset;
        commandListReset.key = stateService_.getUniqueCommandKey();
        commandListReset.object_.key = commandListCopyKey_;
        commandListReset.pAllocator_.key = commandAllocatorCopyKey_;
        commandListReset.pInitialState_.key = 0;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListResetWriter(commandListReset));
      }

      for (auto& it : state->buffers) {
        if (!it.second->isMappable && restoredBuffers.find(it.first) != restoredBuffers.end()) {
          ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
          barrierCommand.key = stateService_.getUniqueCommandKey();
          barrierCommand.object_.key = commandListDirectKey_;
          barrierCommand.NumBarriers_.value = 1;
          D3D12_RESOURCE_BARRIER barrier{};
          barrierCommand.pBarriers_.value = &barrier;
          barrierCommand.pBarriers_.size = 1;
          barrierCommand.pBarriers_.resourceKeys.resize(1);
          barrierCommand.pBarriers_.resourceAfterKeys.resize(1);
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
          barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
          barrierCommand.pBarriers_.resourceKeys[0] = it.first;
          stateService_.getRecorder().record(
              new ID3D12GraphicsCommandListResourceBarrierWriter(barrierCommand));
        }
      }

      state->desc->scratchOpacityMicromapArrayDataKey = scratchResourceKey;
      state->desc->scratchOpacityMicromapArrayDataOffset = 0;

      {
        NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand build;
        build.key = state->commandKey;
        build.pCommandList_.key = commandListDirectKey_;
        build.pParams.value = state->desc->value;
        build.pParams.destOpacityMicromapArrayDataKey =
            state->desc->destOpacityMicromapArrayDataKey;
        build.pParams.destOpacityMicromapArrayDataOffset =
            state->desc->destOpacityMicromapArrayDataOffset;
        build.pParams.inputBufferKey = state->desc->inputBufferKey;
        build.pParams.inputBufferOffset = state->desc->inputBufferOffset;
        build.pParams.perOMMDescsKey = state->desc->perOMMDescsKey;
        build.pParams.perOMMDescsOffset = state->desc->perOMMDescsOffset;
        build.pParams.scratchOpacityMicromapArrayDataKey =
            state->desc->scratchOpacityMicromapArrayDataKey;
        build.pParams.scratchOpacityMicromapArrayDataOffset =
            state->desc->scratchOpacityMicromapArrayDataOffset;
        build.pParams.value->numPostbuildInfoDescs = 0;
        build.pParams.value->pPostbuildInfoDescs = nullptr;
        recorder_.record(new NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayWriter(build));
      }

      {
        ID3D12GraphicsCommandListCloseCommand commandListClose;
        commandListClose.key = stateService_.getUniqueCommandKey();
        commandListClose.object_.key = commandListDirectKey_;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListCloseWriter(commandListClose));

        ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
        executeCommandLists.key = stateService_.getUniqueCommandKey();
        executeCommandLists.object_.key = commandQueueDirectKey_;
        executeCommandLists.NumCommandLists_.value = 1;
        executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
        executeCommandLists.ppCommandLists_.size = 1;
        executeCommandLists.ppCommandLists_.keys.resize(1);
        executeCommandLists.ppCommandLists_.keys[0] = commandListDirectKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

        ID3D12CommandQueueSignalCommand commandQueueSignal;
        commandQueueSignal.key = stateService_.getUniqueCommandKey();
        commandQueueSignal.object_.key = commandQueueDirectKey_;
        commandQueueSignal.pFence_.key = fenceKey_;
        commandQueueSignal.Value_.value = ++recordedFenceValue_;
        stateService_.getRecorder().record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

        ID3D12FenceGetCompletedValueCommand getCompletedValue;
        getCompletedValue.key = stateService_.getUniqueCommandKey();
        getCompletedValue.object_.key = fenceKey_;
        getCompletedValue.result_.value = recordedFenceValue_;
        stateService_.getRecorder().record(
            new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

        ID3D12CommandAllocatorResetCommand commandAllocatorReset;
        commandAllocatorReset.key = stateService_.getUniqueCommandKey();
        commandAllocatorReset.object_.key = commandAllocatorDirectKey_;
        stateService_.getRecorder().record(
            new ID3D12CommandAllocatorResetWriter(commandAllocatorReset));

        ID3D12GraphicsCommandListResetCommand commandListReset;
        commandListReset.key = stateService_.getUniqueCommandKey();
        commandListReset.object_.key = commandListDirectKey_;
        commandListReset.pAllocator_.key = commandAllocatorDirectKey_;
        commandListReset.pInitialState_.key = 0;
        stateService_.getRecorder().record(
            new ID3D12GraphicsCommandListResetWriter(commandListReset));
      }
    } else {
      GITS_ASSERT(0 && "unknown state");
    }
    delete itState.second;
  }

  {
    IUnknownReleaseCommand releaseScratchResource{};
    releaseScratchResource.key = stateService_.getUniqueCommandKey();
    releaseScratchResource.object_.key = scratchResourceKey;
    stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseScratchResource));
  }
  {
    IUnknownReleaseCommand releaseFence{};
    releaseFence.key = stateService_.getUniqueCommandKey();
    releaseFence.object_.key = fenceKey_;
    stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseFence));
  }
  {
    IUnknownReleaseCommand releaseCommandList{};
    releaseCommandList.key = stateService_.getUniqueCommandKey();
    releaseCommandList.object_.key = commandListDirectKey_;
    stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseCommandList));

    IUnknownReleaseCommand releaseCommandAllocator{};
    releaseCommandAllocator.key = stateService_.getUniqueCommandKey();
    releaseCommandAllocator.object_.key = commandAllocatorDirectKey_;
    stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseCommandAllocator));

    IUnknownReleaseCommand releaseCommandQueue{};
    releaseCommandQueue.key = stateService_.getUniqueCommandKey();
    releaseCommandQueue.object_.key = commandQueueDirectKey_;
    stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseCommandQueue));
  }
  {
    IUnknownReleaseCommand releaseCommandList{};
    releaseCommandList.key = stateService_.getUniqueCommandKey();
    releaseCommandList.object_.key = commandListCopyKey_;
    stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseCommandList));

    IUnknownReleaseCommand releaseCommandAllocator{};
    releaseCommandAllocator.key = stateService_.getUniqueCommandKey();
    releaseCommandAllocator.object_.key = commandAllocatorCopyKey_;
    stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseCommandAllocator));

    IUnknownReleaseCommand releaseCommandQueue{};
    releaseCommandQueue.key = stateService_.getUniqueCommandKey();
    releaseCommandQueue.object_.key = commandQueueCopyKey_;
    stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseCommandQueue));
  }
  if (uploadBufferKey_) {
    IUnknownReleaseCommand releaseUploadBuffer{};
    releaseUploadBuffer.key = stateService_.getUniqueCommandKey();
    releaseUploadBuffer.object_.key = uploadBufferKey_;
    stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseUploadBuffer));
  }

  restored_ = true;
}

void AccelerationStructuresBuildService::executeCommandLists(
    ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (restored_) {
    return;
  }
  for (unsigned commandListKey : c.ppCommandLists_.keys) {
    auto itStates = statesByCommandList_.find(commandListKey);
    if (itStates != statesByCommandList_.end()) {
      for (RaytracingAccelerationStructureState* state : itStates->second) {
        storeState(state);
      }
      statesByCommandList_.erase(itStates);
    }
  }
  bufferContentRestore_.executeCommandLists(c.key, c.object_.key, c.object_.value,
                                            c.ppCommandLists_.value, c.NumCommandLists_.value);
}

void AccelerationStructuresBuildService::commandQueueWait(ID3D12CommandQueueWaitCommand& c) {
  bufferContentRestore_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void AccelerationStructuresBuildService::commandQueueSignal(ID3D12CommandQueueSignalCommand& c) {
  bufferContentRestore_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void AccelerationStructuresBuildService::fenceSignal(unsigned key,
                                                     unsigned fenceKey,
                                                     UINT64 fenceValue) {
  bufferContentRestore_.fenceSignal(key, fenceKey, fenceValue);
}

void AccelerationStructuresBuildService::storeState(RaytracingAccelerationStructureState* state) {

  unsigned stateId = ++stateUniqueId_;
  statesById_[stateId] = state;
  if (state->sourceKey) {
    unsigned sourceId = getState(state->sourceKey, state->sourceOffset);
    GITS_ASSERT(sourceId);

    // remove intermediate update
    if (state->stateType == RaytracingAccelerationStructureState::Build) {
      BuildRaytracingAccelerationStructureState* buildState =
          static_cast<BuildRaytracingAccelerationStructureState*>(state);
      if (buildState->update) {
        auto itPrimarySource = stateSourceByDest_.find(sourceId);
        if (itPrimarySource != stateSourceByDest_.end()) {
          sourceId = itPrimarySource->second;
          auto itPrimarySourceState = statesById_.find(sourceId);
          GITS_ASSERT(itPrimarySourceState != statesById_.end());
          buildState->sourceKey = itPrimarySourceState->second->destKey;
          buildState->sourceOffset = itPrimarySourceState->second->destOffset;
          buildState->desc->sourceAccelerationStructureKey = buildState->sourceKey;
          buildState->desc->sourceAccelerationStructureOffset = buildState->sourceOffset;
        }
      }
    } else if (state->stateType == RaytracingAccelerationStructureState::NvAPIBuild) {
      NvAPIBuildRaytracingAccelerationStructureExState* buildState =
          static_cast<NvAPIBuildRaytracingAccelerationStructureExState*>(state);
      if (buildState->update) {
        auto itPrimarySource = stateSourceByDest_.find(sourceId);
        if (itPrimarySource != stateSourceByDest_.end()) {
          sourceId = itPrimarySource->second;
          auto itPrimarySourceState = statesById_.find(sourceId);
          GITS_ASSERT(itPrimarySourceState != statesById_.end());
          buildState->sourceKey = itPrimarySourceState->second->destKey;
          buildState->sourceOffset = itPrimarySourceState->second->destOffset;
          buildState->desc->sourceAccelerationStructureKey = buildState->sourceKey;
          buildState->desc->sourceAccelerationStructureOffset = buildState->sourceOffset;
        }
      }
    }

    stateSourceByDest_[stateId] = sourceId;
    stateDestsBySource_[sourceId].insert(stateId);
  }

  // remove previous state if not a source for any AS
  unsigned prevStateId = getState(state->destKey, state->destOffset);
  if (prevStateId) {
    auto itDests = stateDestsBySource_.find(prevStateId);
    if (itDests == stateDestsBySource_.end()) {
      removeState(prevStateId);
    }
  }

  stateByKeyOffset_[{state->destKey, state->destOffset}].insert(stateId);
}

unsigned AccelerationStructuresBuildService::getState(unsigned key, unsigned offset) {
  auto itStates = stateByKeyOffset_.find({key, offset});
  if (itStates == stateByKeyOffset_.end() || itStates->second.empty()) {
    return 0;
  }
  return *itStates->second.rbegin();
}

void AccelerationStructuresBuildService::removeState(unsigned stateId, bool removeSource) {
  auto itState = statesById_.find(stateId);
  GITS_ASSERT(itState != statesById_.end());
  if (itState->second->stateType == RaytracingAccelerationStructureState::NvAPIOMM) {
    return;
  }

  // remove state sources chain
  auto itSource = stateSourceByDest_.find(stateId);
  if (itSource != stateSourceByDest_.end()) {
    auto itDests = stateDestsBySource_.find(itSource->second);
    GITS_ASSERT(itDests != stateDestsBySource_.end());
    itDests->second.erase(stateId);
    if (itDests->second.empty()) {
      if (removeSource) {
        removeState(itSource->second, removeSource);
        stateDestsBySource_.erase(itDests);
      } else {
        sourcesWithoutDestinations_.insert(itSource->second);
      }
    }
    stateSourceByDest_.erase(itSource);
  }

  // remove state
  if (itState->second->stateType != RaytracingAccelerationStructureState::Copy) {
    bufferContentRestore_.removeBuild(itState->second->commandKey);
  }
  auto it = stateByKeyOffset_.find({itState->second->destKey, itState->second->destOffset});
  GITS_ASSERT(it != stateByKeyOffset_.end());
  it->second.erase(stateId);
  if (it->second.empty()) {
    stateByKeyOffset_.erase(it);
  }
  statesById_.erase(itState);
}

void AccelerationStructuresBuildService::initUploadBuffer() {
  size_t maxPerBuildUploadSize = 0;
  for (auto& itState : statesById_) {
    std::vector<AccelerationStructuresBufferContentRestore::BufferRestoreInfo>& restoreInfos =
        bufferContentRestore_.getRestoreInfos(itState.second->commandKey);
    size_t uploadSize = 0;
    for (const AccelerationStructuresBufferContentRestore::BufferRestoreInfo& info : restoreInfos) {
      if (!info.isMappable) {
        uploadSize += info.bufferData->size();
      }
    }

    if (uploadSize > maxPerBuildUploadSize) {
      maxPerBuildUploadSize = uploadSize;
    }
  }

  if (maxPerBuildUploadSize == 0) {
    return;
  }

  uploadBufferSize_ = maxPerBuildUploadSize;

  D3D12_HEAP_PROPERTIES heapPropertiesUpload{};
  heapPropertiesUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapPropertiesUpload.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapPropertiesUpload.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapPropertiesUpload.CreationNodeMask = 1;
  heapPropertiesUpload.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = uploadBufferSize_;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  uploadBufferKey_ = stateService_.getUniqueObjectKey();

  ID3D12DeviceCreateCommittedResourceCommand createUploadResource;
  createUploadResource.key = stateService_.getUniqueCommandKey();
  createUploadResource.object_.key = deviceKey_;
  createUploadResource.pHeapProperties_.value = &heapPropertiesUpload;
  createUploadResource.HeapFlags_.value = D3D12_HEAP_FLAG_NONE;
  createUploadResource.pDesc_.value = &resourceDesc;
  createUploadResource.InitialResourceState_.value = D3D12_RESOURCE_STATE_GENERIC_READ;
  createUploadResource.pOptimizedClearValue_.value = nullptr;
  createUploadResource.riidResource_.value = IID_ID3D12Resource;
  createUploadResource.ppvResource_.key = uploadBufferKey_;
  recorder_.record(new ID3D12DeviceCreateCommittedResourceWriter(createUploadResource));
}

size_t AccelerationStructuresBuildService::restoreBuffer(
    const AccelerationStructuresBufferContentRestore::BufferRestoreInfo& restoreInfo,
    size_t uploadBufferOffset) {
  if (restoreInfo.isMappable) {
    ID3D12ResourceMapCommand mapCommand;
    mapCommand.key = stateService_.getUniqueCommandKey();
    mapCommand.object_.key = restoreInfo.bufferKey;
    mapCommand.Subresource_.value = 0;
    mapCommand.pReadRange_.value = nullptr;
    mapCommand.ppData_.captureValue = stateService_.getUniqueFakePointer();
    mapCommand.ppData_.value = &mapCommand.ppData_.captureValue;
    recorder_.record(new ID3D12ResourceMapWriter(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.key = stateService_.getUniqueCommandKey();
    metaCommand.resource_.key = restoreInfo.bufferKey;
    metaCommand.mappedAddress_.value = mapCommand.ppData_.captureValue;
    metaCommand.offset_.value = restoreInfo.offset;
    metaCommand.data_.value = const_cast<char*>(restoreInfo.bufferData->data());
    metaCommand.data_.size = restoreInfo.bufferData->size();
    recorder_.record(new MappedDataMetaWriter(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.key = stateService_.getUniqueCommandKey();
    unmapCommand.object_.key = restoreInfo.bufferKey;
    unmapCommand.Subresource_.value = 0;
    unmapCommand.pWrittenRange_.value = nullptr;
    recorder_.record(new ID3D12ResourceUnmapWriter(unmapCommand));

    return 0;
  } else {
    GITS_ASSERT(uploadBufferOffset + restoreInfo.bufferData->size() <= uploadBufferSize_);

    ID3D12ResourceMapCommand mapCommand;
    mapCommand.key = stateService_.getUniqueCommandKey();
    mapCommand.object_.key = uploadBufferKey_;
    mapCommand.Subresource_.value = 0;
    mapCommand.pReadRange_.value = nullptr;
    mapCommand.ppData_.captureValue = stateService_.getUniqueFakePointer();
    mapCommand.ppData_.value = &mapCommand.ppData_.captureValue;
    recorder_.record(new ID3D12ResourceMapWriter(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.key = stateService_.getUniqueCommandKey();
    metaCommand.resource_.key = uploadBufferKey_;
    metaCommand.mappedAddress_.value = mapCommand.ppData_.captureValue;
    metaCommand.offset_.value = uploadBufferOffset;
    metaCommand.data_.value = const_cast<char*>(restoreInfo.bufferData->data());
    metaCommand.data_.size = restoreInfo.bufferData->size();
    recorder_.record(new MappedDataMetaWriter(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.key = stateService_.getUniqueCommandKey();
    unmapCommand.object_.key = uploadBufferKey_;
    unmapCommand.Subresource_.value = 0;
    unmapCommand.pWrittenRange_.value = nullptr;
    recorder_.record(new ID3D12ResourceUnmapWriter(unmapCommand));

    ID3D12GraphicsCommandListCopyBufferRegionCommand copyBufferRegion;
    copyBufferRegion.key = stateService_.getUniqueCommandKey();
    copyBufferRegion.object_.key = commandListCopyKey_;
    copyBufferRegion.pDstBuffer_.key = restoreInfo.bufferKey;
    copyBufferRegion.DstOffset_.value = restoreInfo.offset;
    copyBufferRegion.pSrcBuffer_.key = uploadBufferKey_;
    copyBufferRegion.SrcOffset_.value = uploadBufferOffset;
    copyBufferRegion.NumBytes_.value = restoreInfo.bufferData->size();
    recorder_.record(new ID3D12GraphicsCommandListCopyBufferRegionWriter(copyBufferRegion));

    return restoreInfo.bufferData->size();
  }
}

void AccelerationStructuresBuildService::optimize() {
  if (!Configurator::Get().directx.features.subcapture.optimize &&
      !Configurator::Get().directx.features.subcapture.optimizeRaytracing) {
    return;
  }
  std::vector<unsigned> removedStates;
  for (auto& [stateId, state] : statesById_) {
    auto itDests = stateDestsBySource_.find(stateId);
    if (itDests == stateDestsBySource_.end() || itDests->second.empty()) {
      if (!stateService_.getAnalyzerResults().restoreBlas(
              std::make_pair(state->destKey, state->destOffset)) &&
          !stateService_.getAnalyzerResults().restoreTlas(state->commandKey)) {
        removedStates.push_back(stateId);
      }
    }
  }
  for (unsigned stateId : removedStates) {
    removeState(stateId, true);
  }
}

void AccelerationStructuresBuildService::removeSourcesWithoutDestinations() {
  for (unsigned source : sourcesWithoutDestinations_) {
    auto itDests = stateDestsBySource_.find(source);
    GITS_ASSERT(itDests != stateDestsBySource_.end());
    if (itDests->second.empty()) {
      removeState(source, true);
      stateDestsBySource_.erase(itDests);
    }
  }
}

void AccelerationStructuresBuildService::completeSourcesFromAnalysis() {
  std::set<std::pair<unsigned, unsigned>>& sources =
      stateService_.getAnalyzerResults().getAsSources();
  for (auto& keyOffset : sources) {
    auto itStates = stateByKeyOffset_.find(keyOffset);
    if (itStates != stateByKeyOffset_.end() && !itStates->second.empty()) {
      unsigned lastState = *itStates->second.rbegin();
      stateDestsBySource_[lastState].insert(UINT_MAX);
    }
  }
}

} // namespace DirectX
} // namespace gits
