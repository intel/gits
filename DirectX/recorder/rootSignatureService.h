// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "iunknownWrapper.h"
#include "gits.h"

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
  void createRootSignature(void* blobPointer, unsigned blobLength, unsigned rootSignatureKey);

  void setGraphicsRootSignature(unsigned commandListKey, unsigned rootSignatureKey);
  void setComputeRootSignature(unsigned commandListKey, unsigned rootSignatureKey);
  void resetRootSignatures(unsigned commandListKey);
  D3D12_DESCRIPTOR_HEAP_TYPE getGraphicsRootSignatureDescriptorHeapType(unsigned commandListKey,
                                                                        unsigned parameterIndex);
  D3D12_DESCRIPTOR_HEAP_TYPE getComputeRootSignatureDescriptorHeapType(unsigned commandListKey,
                                                                       unsigned parameterIndex);

private:
  class RootSignatureInfo {
  public:
    void setDescriptorTableHeapType(unsigned parameterIndex, D3D12_DESCRIPTOR_HEAP_TYPE type) {
      descriptorTableHeapTypeByParameterIndex_[parameterIndex] = type;
    }
    D3D12_DESCRIPTOR_HEAP_TYPE getDescriptorTableHeapType(unsigned parameterIndex) {
      auto it = descriptorTableHeapTypeByParameterIndex_.find(parameterIndex);
      GITS_ASSERT(it != descriptorTableHeapTypeByParameterIndex_.end());
      return it->second;
    }

  private:
    std::unordered_map<unsigned, D3D12_DESCRIPTOR_HEAP_TYPE>
        descriptorTableHeapTypeByParameterIndex_;
  };

private:
  template <typename ROOT_SIGNATURE_DESC>
  void parseRootSignatureDesc(ROOT_SIGNATURE_DESC& desc, RootSignatureInfo* rootSignatureInfo);

private:
  std::unordered_map<unsigned, RootSignatureInfo*> rootSignatureByBlobKey_;
  std::unordered_map<void*, RootSignatureInfo*> rootSignatureByBlobPointer_;
  std::unordered_map<unsigned, RootSignatureInfo*> rootSignatureByRootSignatureKey_;

  std::unordered_map<unsigned, RootSignatureInfo*> graphicsRootSignatureByCommandListKey_;
  std::unordered_map<unsigned, RootSignatureInfo*> computeRootSignatureByCommandListKey_;

  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
