// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "iunknownWrapper.h"
#include "log.h"

#include <unordered_map>
#include <mutex>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class RootSignatureService {
public:
  void serializeRootSignature(D3D12_ROOT_SIGNATURE_DESC* desc, unsigned blobKey);
  void serializeVersionedRootSignature(D3D12_VERSIONED_ROOT_SIGNATURE_DESC* desc, unsigned blobKey);
  void setBlobBufferPointer(unsigned blobKey, void* blobPointer);
  void createRootSignature(void* blobPointer, unsigned blobLength, unsigned RootSignatureKey);

  void setGraphicsRootSignature(unsigned commandListKey, unsigned RootSignatureKey);
  void setComputeRootSignature(unsigned commandListKey, unsigned RootSignatureKey);
  void resetRootSignatures(unsigned commandListKey);
  D3D12_DESCRIPTOR_HEAP_TYPE getGraphicsRootSignatureDescriptorHeapType(unsigned commandListKey,
                                                                        unsigned parameterIndex);
  D3D12_DESCRIPTOR_HEAP_TYPE getComputeRootSignatureDescriptorHeapType(unsigned commandListKey,
                                                                       unsigned parameterIndex);

private:
  class RootSignatureInfo {
  public:
    void setDescriptorTableHeapType(unsigned parameterIndex, D3D12_DESCRIPTOR_HEAP_TYPE type) {
      m_DescriptorTableHeapTypeByParameterIndex[parameterIndex] = type;
    }
    D3D12_DESCRIPTOR_HEAP_TYPE getDescriptorTableHeapType(unsigned parameterIndex) {
      auto it = m_DescriptorTableHeapTypeByParameterIndex.find(parameterIndex);
      GITS_ASSERT(it != m_DescriptorTableHeapTypeByParameterIndex.end());
      return it->second;
    }

  private:
    std::unordered_map<unsigned, D3D12_DESCRIPTOR_HEAP_TYPE>
        m_DescriptorTableHeapTypeByParameterIndex;
  };

private:
  template <typename ROOT_SIGNATURE_DESC>
  void parseRootSignatureDesc(ROOT_SIGNATURE_DESC& desc, RootSignatureInfo* rootSignatureInfo);

private:
  std::unordered_map<unsigned, RootSignatureInfo*> m_RootSignatureByBlobKey;
  std::unordered_map<void*, RootSignatureInfo*> m_RootSignatureByBlobPointer;
  std::unordered_map<unsigned, RootSignatureInfo*> m_RootSignatureByRootSignatureKey;

  std::unordered_map<unsigned, RootSignatureInfo*> m_GraphicsRootSignatureByCommandListKey;
  std::unordered_map<unsigned, RootSignatureInfo*> m_ComputeRootSignatureByCommandListKey;

  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
