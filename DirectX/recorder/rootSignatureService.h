// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "iunknownWrapper.h"
#include "log.h"

#include <unordered_map>
#include <mutex>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class RootSignatureService {
public:
  void SerializeRootSignature(D3D12_ROOT_SIGNATURE_DESC* desc, GITSKey blobKey);
  void SerializeVersionedRootSignature(D3D12_VERSIONED_ROOT_SIGNATURE_DESC* desc, GITSKey blobKey);
  void SetBlobBufferPointer(GITSKey blobKey, void* blobPointer);
  void CreateRootSignature(void* blobPointer, unsigned blobLength, GITSKey rootSignatureKey);

  void SetGraphicsRootSignature(GITSKey commandListKey, GITSKey rootSignatureKey);
  void SetComputeRootSignature(GITSKey commandListKey, GITSKey rootSignatureKey);
  void ResetRootSignatures(GITSKey commandListKey);
  D3D12_DESCRIPTOR_HEAP_TYPE GetGraphicsRootSignatureDescriptorHeapType(GITSKey commandListKey,
                                                                        unsigned parameterIndex);
  D3D12_DESCRIPTOR_HEAP_TYPE GetComputeRootSignatureDescriptorHeapType(GITSKey commandListKey,
                                                                       unsigned parameterIndex);

private:
  class RootSignatureInfo {
  public:
    void SetDescriptorTableHeapType(unsigned parameterIndex, D3D12_DESCRIPTOR_HEAP_TYPE type) {
      m_DescriptorTableHeapTypeByParameterIndex[parameterIndex] = type;
    }
    D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorTableHeapType(unsigned parameterIndex) {
      auto it = m_DescriptorTableHeapTypeByParameterIndex.find(parameterIndex);
      GITS_ASSERT(it != m_DescriptorTableHeapTypeByParameterIndex.end());
      return it->second;
    }

  private:
    std::unordered_map<GITSKey, D3D12_DESCRIPTOR_HEAP_TYPE>
        m_DescriptorTableHeapTypeByParameterIndex;
  };

private:
  template <typename ROOT_SIGNATURE_DESC>
  void ParseRootSignatureDesc(ROOT_SIGNATURE_DESC& desc, RootSignatureInfo* rootSignatureInfo);

private:
  std::unordered_map<GITSKey, RootSignatureInfo*> m_RootSignatureByBlobKey;
  std::unordered_map<void*, RootSignatureInfo*> m_RootSignatureByBlobPointer;
  std::unordered_map<GITSKey, RootSignatureInfo*> m_RootSignatureByRootSignatureKey;

  std::unordered_map<GITSKey, RootSignatureInfo*> m_GraphicsRootSignatureByCommandListKey;
  std::unordered_map<GITSKey, RootSignatureInfo*> m_ComputeRootSignatureByCommandListKey;

  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
