// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rootSignatureService.h"
#include "gits.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

RootSignatureService::~RootSignatureService() {
  for (auto& it : rootSignatureDescs_) {
    D3D12_ROOT_SIGNATURE_DESC* desc = it.second;
    for (unsigned i = 0; i < desc->NumParameters; ++i) {
      if (desc->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
        delete[] desc->pParameters[i].DescriptorTable.pDescriptorRanges;
      }
    }
    delete desc->pParameters;
    delete desc->pStaticSamplers;
    delete desc;
  }
}

void RootSignatureService::createRootSignature(ID3D12DeviceCreateRootSignatureCommand& c) {

  Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> deserializer;
  HRESULT hr = D3D12CreateVersionedRootSignatureDeserializer(
      c.pBlobWithRootSignature_.value, c.blobLengthInBytes_.value, IID_PPV_ARGS(&deserializer));
  GITS_ASSERT(hr == S_OK);
  const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* versionedDesc{};
  hr = deserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1, &versionedDesc);
  GITS_ASSERT(hr == S_OK);
  D3D12_ROOT_SIGNATURE_DESC* desc = new D3D12_ROOT_SIGNATURE_DESC(versionedDesc->Desc_1_0);
  desc->pParameters = new D3D12_ROOT_PARAMETER[desc->NumParameters];
  memcpy(const_cast<D3D12_ROOT_PARAMETER*>(desc->pParameters), versionedDesc->Desc_1_0.pParameters,
         desc->NumParameters * sizeof(D3D12_ROOT_PARAMETER));
  for (unsigned i = 0; i < desc->NumParameters; ++i) {
    if (desc->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
      const_cast<D3D12_ROOT_PARAMETER*>(desc->pParameters)[i].DescriptorTable.pDescriptorRanges =
          new D3D12_DESCRIPTOR_RANGE[desc->pParameters[i].DescriptorTable.NumDescriptorRanges];
      memcpy(const_cast<D3D12_DESCRIPTOR_RANGE*>(
                 desc->pParameters[i].DescriptorTable.pDescriptorRanges),
             versionedDesc->Desc_1_0.pParameters[i].DescriptorTable.pDescriptorRanges,
             desc->pParameters[i].DescriptorTable.NumDescriptorRanges *
                 sizeof(D3D12_DESCRIPTOR_RANGE));
    }
  }
  desc->pStaticSamplers = new D3D12_STATIC_SAMPLER_DESC[desc->NumStaticSamplers];
  memcpy(const_cast<D3D12_STATIC_SAMPLER_DESC*>(desc->pStaticSamplers),
         versionedDesc->Desc_1_0.pStaticSamplers,
         desc->NumStaticSamplers * sizeof(D3D12_STATIC_SAMPLER_DESC));
  rootSignatureDescs_[c.ppvRootSignature_.key] = desc;
}

std::vector<unsigned> RootSignatureService::getDescriptorTableIndexes(unsigned rootSignatureKey,
                                                                      unsigned descriptorHeapKey,
                                                                      unsigned parameterIndex,
                                                                      unsigned baseIndex,
                                                                      unsigned heapNumDescriptors) {
  std::vector<unsigned> indexes;

  auto it = rootSignatureDescs_.find(rootSignatureKey);
  GITS_ASSERT(it != rootSignatureDescs_.end());
  GITS_ASSERT(parameterIndex < it->second->NumParameters);
  const D3D12_ROOT_PARAMETER& param = it->second->pParameters[parameterIndex];
  GITS_ASSERT(param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
  unsigned index = baseIndex;
  for (unsigned i = 0; i < param.DescriptorTable.NumDescriptorRanges; ++i) {
    const D3D12_DESCRIPTOR_RANGE& range = param.DescriptorTable.pDescriptorRanges[i];
    if (range.OffsetInDescriptorsFromTableStart != D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND) {
      index = range.OffsetInDescriptorsFromTableStart + baseIndex;
    }
    unsigned numDescriptors = range.NumDescriptors;
    if (range.NumDescriptors == UINT_MAX) {
      if (unboundedRetrieved(descriptorHeapKey, index)) {
        continue;
      }
      numDescriptors = heapNumDescriptors - index;
    } else {
      if (boundedRetrieved(descriptorHeapKey, index, numDescriptors)) {
        continue;
      }
    }
    for (unsigned j = 0; j < numDescriptors; ++j) {
      indexes.push_back(index);
      ++index;
    }
  }

  return indexes;
}

bool RootSignatureService::unboundedRetrieved(unsigned descriptorHeapKey, unsigned index) {
  auto it = unboundedRetrieved_.find(descriptorHeapKey);
  if (it != unboundedRetrieved_.end() && it->second <= index) {
    return true;
  }
  unboundedRetrieved_[descriptorHeapKey] = index;
  return false;
}

bool RootSignatureService::boundedRetrieved(unsigned descriptorHeapKey,
                                            unsigned index,
                                            unsigned numDescriptors) {
  auto itHeap = boundedRetrieved_.find(descriptorHeapKey);
  if (itHeap != boundedRetrieved_.end()) {
    auto it = itHeap->second.find(index);
    if (it != itHeap->second.end() && it->second >= numDescriptors) {
      return true;
    }
  }
  boundedRetrieved_[descriptorHeapKey][index] = numDescriptors;
  return false;
}

} // namespace DirectX
} // namespace gits
