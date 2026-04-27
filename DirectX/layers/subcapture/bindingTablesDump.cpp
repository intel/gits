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
                                         StateObjectInfo* StateObjectInfo,
                                         DescriptorHeaps descriptorHeaps,
                                         unsigned rootSignatureKey) {
  BindingTablesInfo* info = new BindingTablesInfo();
  info->Offset = offset;
  info->Size = size;
  info->Stride = stride;
  info->StateObjectInfo = StateObjectInfo;
  info->DescriptorHeaps = descriptorHeaps;
  info->RootSignatureKey = rootSignatureKey;

  StageResource(commandList, resource, state, *info);
}

void BindingTablesDump::DumpBuffer(DumpInfo& dumpInfo, void* data) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  BindingTablesInfo& info = static_cast<BindingTablesInfo&>(dumpInfo);
  unsigned recordCount = info.Size / info.Stride;
  for (unsigned recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
    uint8_t* p = static_cast<uint8_t*>(data) + recordIndex * info.Stride;

    unsigned rootSignatureKey = info.StateObjectInfo->GlobalRootSignature;
    if (!rootSignatureKey) {
      rootSignatureKey = info.RootSignatureKey;
    }
    CapturePlayerShaderIdentifierService::ShaderIdentifier shaderIdentifier;
    memcpy(shaderIdentifier.data(), p, shaderIdentifier.size());
    std::wstring exportName =
        m_RaytracingService.GetShaderIdentifierService().GetExportNameByCaptureIdentifier(
            shaderIdentifier);
    auto it = info.StateObjectInfo->ExportToRootSignature.find(exportName);
    if (it != info.StateObjectInfo->ExportToRootSignature.end()) {
      rootSignatureKey = it->second;
    }

    D3D12_ROOT_SIGNATURE_DESC* desc =
        m_RaytracingService.GetRootSignatureService().GetRootSignatureDesc(rootSignatureKey);
    GITS_ASSERT(desc);

    unsigned byteOffset = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    for (unsigned rootParameterIndex = 0; rootParameterIndex < desc->NumParameters;
         ++rootParameterIndex) {
      if (byteOffset >= info.Stride) {
        break;
      }
      const D3D12_ROOT_PARAMETER& param = desc->pParameters[rootParameterIndex];
      if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
        byteOffset += sizeof(uint32_t) * param.Constants.Num32BitValues;
      } else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV ||
                 param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV ||
                 param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV) {
        byteOffset = Align(byteOffset, sizeof(UINT64));
        if (byteOffset >= info.Stride) {
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
        if (byteOffset >= info.Stride) {
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
            heapInfo = m_RaytracingService.GetDescriptorHandleService()
                           .GetSamplerDescriptorHeapInfoByCaptureHandle(*descriptor);
            stride = m_RaytracingService.GetDescriptorHandleService().SamplerHeapIncrement();
            descriptorHeapKey = info.DescriptorHeaps.SamplerHeapKey;
            descriptorHeapSize = info.DescriptorHeaps.SamplerHeapSize;
          } else {
            heapInfo = m_RaytracingService.GetDescriptorHandleService()
                           .GetViewDescriptorHeapInfoByCaptureHandle(*descriptor);
            stride = m_RaytracingService.GetDescriptorHandleService().ViewHeapIncrement();
            descriptorHeapKey = info.DescriptorHeaps.ViewDescriptorHeapKey;
            descriptorHeapSize = info.DescriptorHeaps.ViewDescriptorHeapSize;
          }

          if (heapInfo) {
            unsigned DescriptorIndex = (*descriptor - heapInfo->CaptureStart) / stride;

            std::vector<unsigned> Indexes =
                m_RaytracingService.GetRootSignatureService().GetDescriptorTableIndexes(
                    rootSignatureKey, descriptorHeapKey, rootParameterIndex, DescriptorIndex,
                    descriptorHeapSize, true);
            for (unsigned index : Indexes) {
              DescriptorState* state =
                  m_RaytracingService.GetDescriptorService().GetDescriptorState(descriptorHeapKey,
                                                                                index);
              if (state) {
                m_BindingTablesResources.insert(state->ResourceKey);
                m_BindingTablesDescriptors.insert({descriptorHeapKey, index});
              }
            }
            m_BindingTablesResources.insert(descriptorHeapKey);
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
