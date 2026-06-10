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
              if (range.RangeType != D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
                stream << "\t\t\tDescriptor index " << index;
                if (descriptor && descriptor->ResourceKey) {
                  stream << " resource O" << descriptor->ResourceKey << "\n";
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

} // namespace DirectX
} // namespace gits
