// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "bindingTablesDump.h"
#include "analyzerRaytracingService.h"
#include "capturePlayerGpuAddressService.h"
#include "capturePlayerDescriptorHandleService.h"
#include "log.h"

namespace gits {
namespace DirectX {

void BindingTablesDump::DumpBindingTable(ID3D12GraphicsCommandList* commandList,
                                         ID3D12Resource* resource,
                                         unsigned offset,
                                         unsigned size,
                                         unsigned stride,
                                         BarrierState state,
                                         StateObjectInfo* stateObjectInfo,
                                         DescriptorHeaps descriptorHeaps,
                                         unsigned RootSignatureKey) {
  BindingTablesInfo* info = new BindingTablesInfo();
  info->Offset = offset;
  info->Size = size;
  info->stride = stride;
  info->stateObjectInfo = stateObjectInfo;
  info->descriptorHeaps = descriptorHeaps;
  info->RootSignatureKey = RootSignatureKey;

  StageResource(commandList, resource, state, *info);
}

void BindingTablesDump::DumpBuffer(DumpInfo& dumpInfo, void* data) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  BindingTablesInfo& info = static_cast<BindingTablesInfo&>(dumpInfo);
  unsigned recordCount = info.Size / info.stride;
  for (unsigned recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
    uint8_t* p = static_cast<uint8_t*>(data) + recordIndex * info.stride;

    unsigned RootSignatureKey = info.stateObjectInfo->GlobalRootSignature;
    if (!RootSignatureKey) {
      RootSignatureKey = info.RootSignatureKey;
    }
    CapturePlayerShaderIdentifierService::ShaderIdentifier shaderIdentifier;
    memcpy(shaderIdentifier.data(), p, shaderIdentifier.size());
    std::wstring exportName =
        m_RaytracingService.GetShaderIdentifierService().GetExportNameByCaptureIdentifier(
            shaderIdentifier);
    auto it = info.stateObjectInfo->ExportToRootSignature.find(exportName);
    if (it != info.stateObjectInfo->ExportToRootSignature.end()) {
      RootSignatureKey = it->second;
    }

    D3D12_ROOT_SIGNATURE_DESC* desc =
        m_RaytracingService.GetRootSignatureService().GetRootSignatureDesc(RootSignatureKey);
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
        byteOffset = Align(byteOffset, sizeof(UINT64));
        if (byteOffset >= info.stride) {
          break;
        }

        UINT64* address = reinterpret_cast<UINT64*>(p + byteOffset);
        if (*address) {
          CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
              m_RaytracingService.GetGpuAddressService().GetResourceInfoByCaptureAddress(*address);
          if (resourceInfo) {
            m_BindingTablesResources.insert(resourceInfo->Key);
          }
        }

        byteOffset += sizeof(UINT64);
      } else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
        byteOffset = Align(byteOffset, sizeof(UINT64));
        if (byteOffset >= info.stride) {
          break;
        }

        UINT64* descriptor = reinterpret_cast<UINT64*>(p + byteOffset);
        if (*descriptor) {
          unsigned DescriptorHeapKey{};
          unsigned descriptorHeapSize{};
          CapturePlayerDescriptorHandleService::DescriptorHeapInfo* heapInfo{};

          unsigned stride{};
          GITS_ASSERT(param.DescriptorTable.NumDescriptorRanges);
          if (param.DescriptorTable.pDescriptorRanges[0].RangeType ==
              D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
            heapInfo = m_RaytracingService.GetDescriptorHandleService()
                           .GetSamplerDescriptorHeapInfoByCaptureHandle(*descriptor);
            stride = m_RaytracingService.GetDescriptorHandleService().SamplerHeapIncrement();
            DescriptorHeapKey = info.descriptorHeaps.SamplerHeapKey;
            descriptorHeapSize = info.descriptorHeaps.SamplerHeapSize;
          } else {
            heapInfo = m_RaytracingService.GetDescriptorHandleService()
                           .GetViewDescriptorHeapInfoByCaptureHandle(*descriptor);
            stride = m_RaytracingService.GetDescriptorHandleService().ViewHeapIncrement();
            DescriptorHeapKey = info.descriptorHeaps.ViewDescriptorHeapKey;
            descriptorHeapSize = info.descriptorHeaps.ViewDescriptorHeapSize;
          }

          if (heapInfo) {
            unsigned DescriptorIndex = (*descriptor - heapInfo->CaptureStart) / stride;

            std::vector<unsigned> Indexes =
                m_RaytracingService.GetRootSignatureService().GetDescriptorTableIndexes(
                    RootSignatureKey, DescriptorHeapKey, rootParameterIndex, DescriptorIndex,
                    descriptorHeapSize, true);
            for (unsigned index : Indexes) {
              DescriptorState* state =
                  m_RaytracingService.GetDescriptorService().GetDescriptorState(DescriptorHeapKey,
                                                                                index);
              if (state) {
                m_BindingTablesResources.insert(state->ResourceKey);
                m_BindingTablesDescriptors.insert({DescriptorHeapKey, index});
              }
            }
            m_BindingTablesResources.insert(DescriptorHeapKey);
          }
        }
        byteOffset += sizeof(UINT64);
      }
    }
  }
}

unsigned BindingTablesDump::Align(unsigned value, unsigned alignment) {
  return ((value - 1) / alignment + 1) * alignment;
}

} // namespace DirectX
} // namespace gits
