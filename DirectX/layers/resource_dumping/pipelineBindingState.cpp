// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pipelineBindingState.h"
#include "to_string/enumToStrAuto.h"
#include "log.h"

namespace gits {
namespace DirectX {

void PipelineBindingState::Reset() {
  m_RootSignatureDesc = nullptr;
}

void PipelineBindingState::SetRootSignature(D3D12_ROOT_SIGNATURE_DESC2* desc) {
  m_RootSignatureDesc = desc;
}

void PipelineBindingState::SetDescriptorTable(unsigned parameterIndex,
                                              unsigned descriptorHeapKey,
                                              unsigned descriptorHeapIndex) {
  Binding& binding = m_Bindings[parameterIndex];
  binding.Type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  binding.DescriptorTable.DescriptorHeapKey = descriptorHeapKey;
  binding.DescriptorTable.DescriptorHeapIndex = descriptorHeapIndex;
}

void PipelineBindingState::SetConstant(unsigned parameterIndex, unsigned data, unsigned offset) {
  Binding& binding = m_Bindings[parameterIndex];
  binding.Type = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
}

void PipelineBindingState::SetConstants(unsigned parameterIndex,
                                        unsigned* data,
                                        unsigned offset,
                                        unsigned size) {
  Binding& binding = m_Bindings[parameterIndex];
  binding.Type = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
}

void PipelineBindingState::SetConstantBufferView(unsigned parameterIndex,
                                                 unsigned resourceKey,
                                                 unsigned resourceOffset) {
  Binding& binding = m_Bindings[parameterIndex];
  binding.Type = D3D12_ROOT_PARAMETER_TYPE_CBV;
  binding.Descriptor.ResourceKey = resourceKey;
  binding.Descriptor.ResourceOffset = resourceOffset;
}

void PipelineBindingState::SetUnorderedAccessView(unsigned parameterIndex,
                                                  unsigned resourceKey,
                                                  unsigned resourceOffset) {
  Binding& binding = m_Bindings[parameterIndex];
  binding.Type = D3D12_ROOT_PARAMETER_TYPE_UAV;
  binding.Descriptor.ResourceKey = resourceKey;
  binding.Descriptor.ResourceOffset = resourceOffset;
}

void PipelineBindingState::SetShaderResourceView(unsigned parameterIndex,
                                                 unsigned resourceKey,
                                                 unsigned resourceOffset) {
  Binding& binding = m_Bindings[parameterIndex];
  binding.Type = D3D12_ROOT_PARAMETER_TYPE_SRV;
  binding.Descriptor.ResourceKey = resourceKey;
  binding.Descriptor.ResourceOffset = resourceOffset;
}

void PipelineBindingState::DumpState(std::ofstream& stream) {
  stream << "Bindings\n";
  for (unsigned parameterIndex = 0; parameterIndex < m_RootSignatureDesc->NumParameters;
       ++parameterIndex) {
    const D3D12_ROOT_PARAMETER1& rootParameter = m_RootSignatureDesc->pParameters[parameterIndex];
    stream << "\tBinding[" << parameterIndex << "] " << toStr(rootParameter.ParameterType);
    auto it = m_Bindings.find(parameterIndex);
    if (it != m_Bindings.end()) {
      Binding& binding = it->second;
      if (rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
        GITS_ASSERT(binding.Type == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
        stream << " descriptorHeapKey O" << binding.DescriptorTable.DescriptorHeapKey
               << " baseIndex " << binding.DescriptorTable.DescriptorHeapIndex << "\n";
        unsigned index = binding.DescriptorTable.DescriptorHeapIndex;
        for (unsigned rangeIndex = 0;
             rangeIndex < rootParameter.DescriptorTable.NumDescriptorRanges; ++rangeIndex) {
          const D3D12_DESCRIPTOR_RANGE1& range =
              rootParameter.DescriptorTable.pDescriptorRanges[rangeIndex];
          stream << "\t\tRange[" << rangeIndex << "] " << toStr(range.RangeType);
          if (range.NumDescriptors == UINT_MAX) {
            stream << " UNBOUNDED\n";
          } else {
            stream << " num " << range.NumDescriptors << "\n";
            if (range.OffsetInDescriptorsFromTableStart != D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND) {
              index = range.OffsetInDescriptorsFromTableStart +
                      binding.DescriptorTable.DescriptorHeapIndex;
            }
            for (unsigned i = 0; i < range.NumDescriptors; ++i) {
              DescriptorHeapTracker::Descriptor* descriptor = m_DescriptorService.GetDescriptor(
                  binding.DescriptorTable.DescriptorHeapKey, index);
              if (range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
                stream << "\t\t\tSampler index " << index << "\n";
                if (descriptor && descriptor->IsDesc) {
                  stream << "\t\t\t\t";
                  DumpSampler(stream, descriptor->SamplerDesc);
                  stream << "\n";
                }
              } else {
                stream << "\t\t\tDescriptor index " << index;
                if (descriptor && descriptor->ResourceKey) {
                  stream << " resource O" << descriptor->ResourceKey << "\n";
                  if (descriptor->IsDesc) {
                    if (descriptor->Type ==
                        DescriptorHeapTracker::Descriptor::DescriptorType::SRV) {
                      stream << "\t\t\t\t";
                      DumpShaderResourceView(stream, descriptor->ShaderResourceViewDesc);
                      stream << "\n";
                    } else if (descriptor->Type ==
                               DescriptorHeapTracker::Descriptor::DescriptorType::UAV) {
                      if (descriptor->UavCounterResourceKey) {
                        stream << "\t\t\t\tuav counter resource O"
                               << descriptor->UavCounterResourceKey << "\n";
                      }
                      stream << "\t\t\t\t";
                      DumpUnorderedAccessView(stream, descriptor->UnorderedAccessViewDesc);
                      stream << "\n";
                    } else if (descriptor->Type ==
                               DescriptorHeapTracker::Descriptor::DescriptorType::CBV) {
                      stream << "\t\t\t\t";
                      stream << "C_B_V";
                      stream << "\n";
                    }
                  }
                } else {
                  stream << " null\n";
                }
              }
              ++index;
            }
          }
        }
      } else if (rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV ||
                 rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV ||
                 rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV) {
        GITS_ASSERT(binding.Type == D3D12_ROOT_PARAMETER_TYPE_CBV ||
                    binding.Type == D3D12_ROOT_PARAMETER_TYPE_SRV ||
                    binding.Type == D3D12_ROOT_PARAMETER_TYPE_UAV);
        stream << " resource O" << binding.Descriptor.ResourceKey << " offset "
               << binding.Descriptor.ResourceOffset << "\n";
      } else if (D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
        GITS_ASSERT(binding.Type == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);
        stream << "\n";
      } else {
        stream << "\n";
      }
    } else {
      stream << "\n";
    }
  }
}

void PipelineBindingState::DumpShaderResourceView(std::ofstream& stream,
                                                  const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) {
  stream << toStr(desc.Format) << ", " << toStr(desc.ViewDimension) << std::hex << ", 0x"
         << desc.Shader4ComponentMapping << std::dec << ", {";
  switch (desc.ViewDimension) {
  case D3D12_SRV_DIMENSION_BUFFER:
    stream << desc.Buffer.FirstElement << ", " << desc.Buffer.NumElements << ", "
           << desc.Buffer.StructureByteStride << ", " << toStr(desc.Buffer.Flags);
    break;
  case D3D12_SRV_DIMENSION_TEXTURE1D:
    stream << desc.Texture1D.MostDetailedMip << ", " << desc.Texture1D.MipLevels << ", "
           << desc.Texture1D.ResourceMinLODClamp;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
    stream << desc.Texture1DArray.MostDetailedMip << ", " << desc.Texture1DArray.MipLevels << ", "
           << desc.Texture1DArray.FirstArraySlice << ", " << desc.Texture1DArray.ArraySize << ", "
           << desc.Texture1DArray.ResourceMinLODClamp;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2D:
    stream << desc.Texture2D.MostDetailedMip << ", " << desc.Texture2D.MipLevels << ", "
           << desc.Texture2D.PlaneSlice << ", " << desc.Texture2D.ResourceMinLODClamp;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
    stream << desc.Texture2DArray.MostDetailedMip << ", " << desc.Texture2DArray.MipLevels << ", "
           << desc.Texture2DArray.FirstArraySlice << ", " << desc.Texture2DArray.ArraySize << ", "
           << desc.Texture2DArray.PlaneSlice << ", " << desc.Texture2DArray.ResourceMinLODClamp;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DMS:
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
    stream << desc.Texture2DMSArray.FirstArraySlice << ", " << desc.Texture2DMSArray.ArraySize;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE3D:
    stream << desc.Texture3D.MostDetailedMip << ", " << desc.Texture3D.MipLevels << ", "
           << desc.Texture3D.ResourceMinLODClamp;
    break;
  case D3D12_SRV_DIMENSION_TEXTURECUBE:
    stream << desc.TextureCube.MostDetailedMip << ", " << desc.TextureCube.MipLevels << ", "
           << desc.TextureCube.ResourceMinLODClamp;
    break;
  case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
    stream << desc.TextureCubeArray.MostDetailedMip << ", " << desc.TextureCubeArray.MipLevels
           << ", " << desc.TextureCubeArray.First2DArrayFace << ", "
           << desc.TextureCubeArray.NumCubes << ", " << desc.TextureCubeArray.ResourceMinLODClamp;
    break;
  case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
    stream << desc.RaytracingAccelerationStructure.Location;
    break;
  }
  stream << "}";
}

void PipelineBindingState::DumpUnorderedAccessView(std::ofstream& stream,
                                                   const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc) {
  stream << toStr(desc.Format) << ", " << toStr(desc.ViewDimension) << ", {";
  switch (desc.ViewDimension) {
  case D3D12_UAV_DIMENSION_BUFFER:
    stream << desc.Buffer.FirstElement << ", " << desc.Buffer.NumElements << ", "
           << desc.Buffer.StructureByteStride << ", " << desc.Buffer.CounterOffsetInBytes << ", "
           << toStr(desc.Buffer.Flags);
    break;
  case D3D12_UAV_DIMENSION_TEXTURE1D:
    stream << desc.Texture1D.MipSlice;
    break;
  case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
    stream << desc.Texture1DArray.MipSlice << ", " << desc.Texture1DArray.FirstArraySlice << ", "
           << desc.Texture1DArray.ArraySize;
    break;
  case D3D12_UAV_DIMENSION_TEXTURE2D:
    stream << desc.Texture2D.MipSlice << ", " << desc.Texture2D.PlaneSlice;
    break;
  case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
    stream << desc.Texture2DArray.MipSlice << ", " << desc.Texture2DArray.FirstArraySlice << ", "
           << desc.Texture2DArray.ArraySize << ", " << desc.Texture2DArray.PlaneSlice;
    break;
  case D3D12_UAV_DIMENSION_TEXTURE2DMS:
    break;
  case D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY:
    stream << desc.Texture2DMSArray.FirstArraySlice << ", " << desc.Texture2DMSArray.ArraySize;
    break;
  case D3D12_UAV_DIMENSION_TEXTURE3D:
    stream << desc.Texture3D.MipSlice << ", " << desc.Texture3D.FirstWSlice << ", "
           << desc.Texture3D.WSize;
    break;
  }
  stream << "}";
}

void PipelineBindingState::DumpSampler(std::ofstream& stream, const D3D12_SAMPLER_DESC& desc) {
  stream << toStr(desc.Filter) << ", " << toStr(desc.AddressU) << ", " << toStr(desc.AddressV)
         << ", " << toStr(desc.AddressW) << ", " << desc.MipLODBias << ", " << desc.MaxAnisotropy
         << ", " << toStr(desc.ComparisonFunc) << ", {" << desc.BorderColor[0] << ", "
         << desc.BorderColor[1] << ", " << desc.BorderColor[2] << ", " << desc.BorderColor[3]
         << "}, " << desc.MinLOD << ", " << desc.MaxLOD;
}

} // namespace DirectX
} // namespace gits
