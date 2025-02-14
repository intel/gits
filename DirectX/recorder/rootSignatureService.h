// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectInfos.h"
#include "iunknownWrapper.h"

#include <unordered_map>
#include <mutex>

namespace gits {
namespace DirectX {

class RootSignatureService {
public:
  void serializeRootSignature(D3D12_ROOT_SIGNATURE_DESC* desc, unsigned blobKey);
  void serializeVersionedRootSignature(D3D12_VERSIONED_ROOT_SIGNATURE_DESC* desc, unsigned blobKey);
  void setBlobBufferPointer(unsigned blobKey, void* blobPointer);
  void createRootSignature(void* blobPointer,
                           unsigned blobLength,
                           ObjectInfos* rootSignatureInfos,
                           Layer* layer);

private:
  template <typename ROOT_SIGNATURE_DESC>
  void parseRootSignatureDesc(ROOT_SIGNATURE_DESC& desc,
                              RootSignatureObjectInfo* rootSignatureInfo);

private:
  std::unordered_map<unsigned, RootSignatureObjectInfo*> rootSignatureInfosByBlobKey_;
  std::unordered_map<void*, RootSignatureObjectInfo*> rootSignatureInfosByBlobPointer_;

  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
