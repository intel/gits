// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rootSignatureService.h"
#include "captureManager.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void RootSignatureService::SerializeRootSignature(D3D12_ROOT_SIGNATURE_DESC* desc,
                                                  unsigned blobKey) {

  std::lock_guard<std::mutex> lock(m_Mutex);

  RootSignatureInfo* info = new RootSignatureInfo{};
  ParseRootSignatureDesc(*desc, info);
  m_RootSignatureByBlobKey[blobKey] = info;
}

void RootSignatureService::SerializeVersionedRootSignature(
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC* desc, unsigned blobKey) {

  std::lock_guard<std::mutex> lock(m_Mutex);

  RootSignatureInfo* info = new RootSignatureInfo{};

  switch (desc->Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0:
    ParseRootSignatureDesc(desc->Desc_1_0, info);
    break;
  case D3D_ROOT_SIGNATURE_VERSION_1_1:
    ParseRootSignatureDesc(desc->Desc_1_1, info);
    break;
  case D3D_ROOT_SIGNATURE_VERSION_1_2:
    ParseRootSignatureDesc(desc->Desc_1_2, info);
    break;
  }

  m_RootSignatureByBlobKey[blobKey] = info;
}

template <typename ROOT_SIGNATURE_DESC>
void RootSignatureService::ParseRootSignatureDesc(ROOT_SIGNATURE_DESC& desc,
                                                  RootSignatureInfo* rootSignatureInfo) {

  for (unsigned paramIndex = 0; paramIndex < desc.NumParameters; ++paramIndex) {
    if (desc.pParameters[paramIndex].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
      for (unsigned rangeIndex = 0;
           rangeIndex < desc.pParameters[paramIndex].DescriptorTable.NumDescriptorRanges;
           ++rangeIndex) {
        if (desc.pParameters[paramIndex].DescriptorTable.pDescriptorRanges[rangeIndex].RangeType ==
            D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
          rootSignatureInfo->SetDescriptorTableHeapType(paramIndex,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        } else {
          rootSignatureInfo->SetDescriptorTableHeapType(paramIndex,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
      }
    }
  }
}

void RootSignatureService::SetBlobBufferPointer(unsigned blobKey, void* blobPointer) {

  std::lock_guard<std::mutex> lock(m_Mutex);

  auto it = m_RootSignatureByBlobKey.find(blobKey);
  if (it != m_RootSignatureByBlobKey.end()) {
    m_RootSignatureByBlobPointer[blobPointer] = it->second;
    m_RootSignatureByBlobKey.erase(blobKey);
  }
}

void RootSignatureService::CreateRootSignature(void* blobPointer,
                                               unsigned blobLength,
                                               unsigned RootSignatureKey) {

  std::lock_guard<std::mutex> lock(m_Mutex);

  RootSignatureInfo* info{};

  auto it = m_RootSignatureByBlobPointer.find(blobPointer);
  if (it != m_RootSignatureByBlobPointer.end()) {

    info = it->second;
    m_RootSignatureByBlobPointer.erase(blobPointer);
  } else {

    info = new RootSignatureInfo{};

    Microsoft::WRL::ComPtr<ID3D12RootSignatureDeserializer> rootSignatureDeserializer;
    HRESULT res = D3D12CreateRootSignatureDeserializer(blobPointer, blobLength,
                                                       IID_PPV_ARGS(&rootSignatureDeserializer));
    GITS_ASSERT(res == S_OK);

    const D3D12_ROOT_SIGNATURE_DESC* desc = rootSignatureDeserializer->GetRootSignatureDesc();
    ParseRootSignatureDesc(*desc, info);
  }

  m_RootSignatureByRootSignatureKey[RootSignatureKey] = info;
}

void RootSignatureService::SetGraphicsRootSignature(unsigned commandListKey,
                                                    unsigned RootSignatureKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_RootSignatureByRootSignatureKey.find(RootSignatureKey);
  GITS_ASSERT(it != m_RootSignatureByRootSignatureKey.end());
  m_GraphicsRootSignatureByCommandListKey[commandListKey] = it->second;
}

void RootSignatureService::SetComputeRootSignature(unsigned commandListKey,
                                                   unsigned RootSignatureKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_RootSignatureByRootSignatureKey.find(RootSignatureKey);
  GITS_ASSERT(it != m_RootSignatureByRootSignatureKey.end());
  m_ComputeRootSignatureByCommandListKey[commandListKey] = it->second;
}

void RootSignatureService::ResetRootSignatures(unsigned commandListKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_GraphicsRootSignatureByCommandListKey.erase(commandListKey);
  m_ComputeRootSignatureByCommandListKey.erase(commandListKey);
}

D3D12_DESCRIPTOR_HEAP_TYPE RootSignatureService::GetGraphicsRootSignatureDescriptorHeapType(
    unsigned commandListKey, unsigned parameterIndex) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  D3D12_DESCRIPTOR_HEAP_TYPE type{};
  auto it = m_GraphicsRootSignatureByCommandListKey.find(commandListKey);
  if (it != m_GraphicsRootSignatureByCommandListKey.end()) {
    type = it->second->GetDescriptorTableHeapType(parameterIndex);
  }
  return type;
}

D3D12_DESCRIPTOR_HEAP_TYPE RootSignatureService::GetComputeRootSignatureDescriptorHeapType(
    unsigned commandListKey, unsigned parameterIndex) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  D3D12_DESCRIPTOR_HEAP_TYPE type{};
  auto it = m_ComputeRootSignatureByCommandListKey.find(commandListKey);
  if (it != m_ComputeRootSignatureByCommandListKey.end()) {
    type = it->second->GetDescriptorTableHeapType(parameterIndex);
  }
  return type;
}

} // namespace DirectX
} // namespace gits
