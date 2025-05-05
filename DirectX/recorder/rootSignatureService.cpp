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

  RootSignatureInfo* info = new RootSignatureInfo{};
  parseRootSignatureDesc(*desc, info);
  rootSignatureByBlobKey_[blobKey] = info;
}

void RootSignatureService::serializeVersionedRootSignature(
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC* desc, unsigned blobKey) {

  std::lock_guard<std::mutex> lock(mutex_);

  RootSignatureInfo* info = new RootSignatureInfo{};

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

  rootSignatureByBlobKey_[blobKey] = info;
}

template <typename ROOT_SIGNATURE_DESC>
void RootSignatureService::parseRootSignatureDesc(ROOT_SIGNATURE_DESC& desc,
                                                  RootSignatureInfo* rootSignatureInfo) {

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

  auto it = rootSignatureByBlobKey_.find(blobKey);
  if (it != rootSignatureByBlobKey_.end()) {
    rootSignatureByBlobPointer_[blobPointer] = it->second;
    rootSignatureByBlobKey_.erase(blobKey);
  }
}

void RootSignatureService::createRootSignature(void* blobPointer,
                                               unsigned blobLength,
                                               unsigned rootSignatureKey) {

  std::lock_guard<std::mutex> lock(mutex_);

  RootSignatureInfo* info{};

  auto it = rootSignatureByBlobPointer_.find(blobPointer);
  if (it != rootSignatureByBlobPointer_.end()) {

    info = it->second;
    rootSignatureByBlobPointer_.erase(blobPointer);
  } else {

    info = new RootSignatureInfo{};

    Microsoft::WRL::ComPtr<ID3D12RootSignatureDeserializer> rootSignatureDeserializer;
    HRESULT res = D3D12CreateRootSignatureDeserializer(blobPointer, blobLength,
                                                       IID_PPV_ARGS(&rootSignatureDeserializer));
    GITS_ASSERT(res == S_OK);

    const D3D12_ROOT_SIGNATURE_DESC* desc = rootSignatureDeserializer->GetRootSignatureDesc();
    parseRootSignatureDesc(*desc, info);
  }

  rootSignatureByRootSignatureKey_[rootSignatureKey] = info;
}

void RootSignatureService::setGraphicsRootSignature(unsigned commandListKey,
                                                    unsigned rootSignatureKey) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = rootSignatureByRootSignatureKey_.find(rootSignatureKey);
  GITS_ASSERT(it != rootSignatureByRootSignatureKey_.end());
  graphicsRootSignatureByCommandListKey_[commandListKey] = it->second;
}

void RootSignatureService::setComputeRootSignature(unsigned commandListKey,
                                                   unsigned rootSignatureKey) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = rootSignatureByRootSignatureKey_.find(rootSignatureKey);
  GITS_ASSERT(it != rootSignatureByRootSignatureKey_.end());
  computeRootSignatureByCommandListKey_[commandListKey] = it->second;
}

void RootSignatureService::resetRootSignatures(unsigned commandListKey) {
  std::lock_guard<std::mutex> lock(mutex_);
  graphicsRootSignatureByCommandListKey_.erase(commandListKey);
  computeRootSignatureByCommandListKey_.erase(commandListKey);
}

D3D12_DESCRIPTOR_HEAP_TYPE RootSignatureService::getGraphicsRootSignatureDescriptorHeapType(
    unsigned commandListKey, unsigned parameterIndex) {
  std::lock_guard<std::mutex> lock(mutex_);
  D3D12_DESCRIPTOR_HEAP_TYPE type{};
  auto it = graphicsRootSignatureByCommandListKey_.find(commandListKey);
  if (it != graphicsRootSignatureByCommandListKey_.end()) {
    type = it->second->getDescriptorTableHeapType(parameterIndex);
  }
  return type;
}

D3D12_DESCRIPTOR_HEAP_TYPE RootSignatureService::getComputeRootSignatureDescriptorHeapType(
    unsigned commandListKey, unsigned parameterIndex) {
  std::lock_guard<std::mutex> lock(mutex_);
  D3D12_DESCRIPTOR_HEAP_TYPE type{};
  auto it = computeRootSignatureByCommandListKey_.find(commandListKey);
  if (it != computeRootSignatureByCommandListKey_.end()) {
    type = it->second->getDescriptorTableHeapType(parameterIndex);
  }
  return type;
}

} // namespace DirectX
} // namespace gits
