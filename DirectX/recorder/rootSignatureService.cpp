// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rootSignatureService.h"
#include "captureManager.h"
#include "gits.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void RootSignatureService::serializeRootSignature(D3D12_ROOT_SIGNATURE_DESC* desc,
                                                  unsigned blobKey) {

  std::lock_guard<std::mutex> lock(mutex_);

  RootSignatureObjectInfo* info = new RootSignatureObjectInfo{};
  parseRootSignatureDesc(*desc, info);
  rootSignatureInfosByBlobKey_[blobKey] = info;
}

void RootSignatureService::serializeVersionedRootSignature(
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC* desc, unsigned blobKey) {

  std::lock_guard<std::mutex> lock(mutex_);

  RootSignatureObjectInfo* info = new RootSignatureObjectInfo{};

  switch (desc->Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0:
    parseRootSignatureDesc(desc->Desc_1_0, info);
    break;
  case D3D_ROOT_SIGNATURE_VERSION_1_1:
    parseRootSignatureDesc(desc->Desc_1_1, info);
    break;
  case D3D_ROOT_SIGNATURE_VERSION_1_2:
    parseRootSignatureDesc(desc->Desc_1_2, info);
    break;
  }

  rootSignatureInfosByBlobKey_[blobKey] = info;
}

template <typename ROOT_SIGNATURE_DESC>
void RootSignatureService::parseRootSignatureDesc(ROOT_SIGNATURE_DESC& desc,
                                                  RootSignatureObjectInfo* rootSignatureInfo) {

  for (unsigned paramIndex = 0; paramIndex < desc.NumParameters; ++paramIndex) {
    if (desc.pParameters[paramIndex].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
      for (unsigned rangeIndex = 0;
           rangeIndex < desc.pParameters[paramIndex].DescriptorTable.NumDescriptorRanges;
           ++rangeIndex) {
        if (desc.pParameters[paramIndex].DescriptorTable.pDescriptorRanges[rangeIndex].RangeType ==
            D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
          rootSignatureInfo->setDescriptorTableHeapType(paramIndex,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        } else {
          rootSignatureInfo->setDescriptorTableHeapType(paramIndex,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
      }
    }
  }
}

void RootSignatureService::setBlobBufferPointer(unsigned blobKey, void* blobPointer) {

  std::lock_guard<std::mutex> lock(mutex_);

  auto it = rootSignatureInfosByBlobKey_.find(blobKey);
  if (it != rootSignatureInfosByBlobKey_.end()) {
    rootSignatureInfosByBlobPointer_[blobPointer] = it->second;
    rootSignatureInfosByBlobKey_.erase(blobKey);
  }
}

void RootSignatureService::createRootSignature(void* blobPointer,
                                               unsigned blobLength,
                                               ObjectInfos* rootSignatureInfos,
                                               Layer* layer) {

  std::lock_guard<std::mutex> lock(mutex_);

  RootSignatureObjectInfo* info{};

  auto it = rootSignatureInfosByBlobPointer_.find(blobPointer);
  if (it != rootSignatureInfosByBlobPointer_.end()) {

    info = it->second;
    rootSignatureInfosByBlobPointer_.erase(blobPointer);
  } else {

    info = new RootSignatureObjectInfo{};

    Microsoft::WRL::ComPtr<ID3D12RootSignatureDeserializer> rootSignatureDeserializer;
    HRESULT res = D3D12CreateRootSignatureDeserializer(blobPointer, blobLength,
                                                       IID_PPV_ARGS(&rootSignatureDeserializer));
    GITS_ASSERT(res == S_OK);

    const D3D12_ROOT_SIGNATURE_DESC* desc = rootSignatureDeserializer->GetRootSignatureDesc();
    parseRootSignatureDesc(*desc, info);
  }

  rootSignatureInfos->addObjectInfo(layer, info);
}

} // namespace DirectX
} // namespace gits
