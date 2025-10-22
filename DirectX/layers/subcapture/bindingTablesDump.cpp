// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "bindingTablesDump.h"
#include "analyzerRaytracingService.h"
#include "capturePlayerGpuAddressService.h"
#include "capturePlayerDescriptorHandleService.h"
#include "gits.h"

namespace gits {
namespace DirectX {

void BindingTablesDump::dumpBindingTable(ID3D12GraphicsCommandList* commandList,
                                         ID3D12Resource* resource,
                                         unsigned offset,
                                         unsigned size,
                                         unsigned stride,
                                         D3D12_RESOURCE_STATES state,
                                         StateObjectInfo* stateObjectInfo,
                                         DescriptorHeaps descriptorHeaps,
                                         unsigned rootSignatureKey) {
  BindingTablesInfo* info = new BindingTablesInfo();
  info->offset = offset;
  info->size = size;
  info->stride = stride;
  info->stateObjectInfo = stateObjectInfo;
  info->descriptorHeaps = descriptorHeaps;
  info->rootSignatureKey = rootSignatureKey;

  stageResource(commandList, resource, state, *info);
}

void BindingTablesDump::dumpBuffer(DumpInfo& dumpInfo, void* data) {
  std::lock_guard<std::mutex> lock(mutex_);

  BindingTablesInfo& info = static_cast<BindingTablesInfo&>(dumpInfo);
  unsigned recordCount = info.size / info.stride;
  for (unsigned recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
    uint8_t* p = static_cast<uint8_t*>(data) + recordIndex * info.stride;

    unsigned rootSignatureKey = info.stateObjectInfo->globalRootSignature;
    if (!rootSignatureKey) {
      rootSignatureKey = info.rootSignatureKey;
    }
    CapturePlayerShaderIdentifierService::ShaderIdentifier shaderIdentifier;
    memcpy(shaderIdentifier.data(), p, shaderIdentifier.size());
    std::wstring exportName =
        raytracingService_.getShaderIdentifierService().getExportNameByCaptureIdentifier(
            shaderIdentifier);
    auto it = info.stateObjectInfo->exportToRootSignature.find(exportName);
    if (it != info.stateObjectInfo->exportToRootSignature.end()) {
      rootSignatureKey = it->second;
    }

    D3D12_ROOT_SIGNATURE_DESC* desc =
        raytracingService_.getRootSignatureService().getRootSignatureDesc(rootSignatureKey);
    GITS_ASSERT(desc);

    unsigned byteOffset = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    for (unsigned rootParameterIndex = 0; rootParameterIndex < desc->NumParameters;
         ++rootParameterIndex) {
      if (byteOffset >= info.stride) {
        break;
      }
      const D3D12_ROOT_PARAMETER& param = desc->pParameters[rootParameterIndex];
      if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
        byteOffset += sizeof(uint32_t) * param.Constants.Num32BitValues;
      } else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV ||
                 param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV ||
                 param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV) {
        byteOffset = align(byteOffset, sizeof(UINT64));
        if (byteOffset >= info.stride) {
          break;
        }

        UINT64* address = reinterpret_cast<UINT64*>(p + byteOffset);
        if (*address) {
          CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
              raytracingService_.getGpuAddressService().getResourceInfoByCaptureAddress(*address);
          if (resourceInfo) {
            bindingTablesResources_.insert(resourceInfo->key);
          }
        }

        byteOffset += sizeof(UINT64);
      } else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
        byteOffset = align(byteOffset, sizeof(UINT64));
        if (byteOffset >= info.stride) {
          break;
        }

        UINT64* descriptor = reinterpret_cast<UINT64*>(p + byteOffset);
        if (*descriptor) {
          unsigned descriptorHeapKey{};
          unsigned descriptorHeapSize{};
          CapturePlayerDescriptorHandleService::DescriptorHeapInfo* heapInfo{};

          unsigned stride{};
          GITS_ASSERT(param.DescriptorTable.NumDescriptorRanges);
          if (param.DescriptorTable.pDescriptorRanges[0].RangeType ==
              D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
            heapInfo = raytracingService_.getDescriptorHandleService()
                           .getSamplerDescriptorHeapInfoByCaptureHandle(*descriptor);
            stride = raytracingService_.getDescriptorHandleService().samplerHeapIncrement();
            descriptorHeapKey = info.descriptorHeaps.samplerHeapKey;
            descriptorHeapSize = info.descriptorHeaps.samplerHeapSize;
          } else {
            heapInfo = raytracingService_.getDescriptorHandleService()
                           .getViewDescriptorHeapInfoByCaptureHandle(*descriptor);
            stride = raytracingService_.getDescriptorHandleService().viewHeapIncrement();
            descriptorHeapKey = info.descriptorHeaps.viewDescriptorHeapKey;
            descriptorHeapSize = info.descriptorHeaps.viewDescriptorHeapSize;
          }

          if (heapInfo) {
            unsigned descriptorIndex = (*descriptor - heapInfo->captureStart) / stride;

            std::vector<unsigned> indexes =
                raytracingService_.getRootSignatureService().getDescriptorTableIndexes(
                    rootSignatureKey, descriptorHeapKey, rootParameterIndex, descriptorIndex,
                    descriptorHeapSize, true);
            for (unsigned index : indexes) {
              DescriptorState* state = raytracingService_.getDescriptorService().getDescriptorState(
                  descriptorHeapKey, index);
              if (state) {
                bindingTablesResources_.insert(state->resourceKey);
                bindingTablesDescriptors_.insert({descriptorHeapKey, index});
              }
            }
            bindingTablesResources_.insert(descriptorHeapKey);
          }
        }
        byteOffset += sizeof(UINT64);
      }
    }
  }
}

unsigned BindingTablesDump::align(unsigned value, unsigned alignment) {
  return ((value - 1) / alignment + 1) * alignment;
}

} // namespace DirectX
} // namespace gits
