// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"

#include <unordered_map>
#include <mutex>

namespace gits {
namespace DirectX {

class DescriptorRootSignatureService {
public:
  DescriptorRootSignatureService() {}
  DescriptorRootSignatureService(const DescriptorRootSignatureService&) = delete;
  DescriptorRootSignatureService& operator=(const DescriptorRootSignatureService&) = delete;
  ~DescriptorRootSignatureService();
  void createRootSignature(ID3D12DeviceCreateRootSignatureCommand& c);
  std::vector<unsigned> getDescriptorTableIndexes(unsigned rootSignatureKey,
                                                  unsigned descriptorHeapKey,
                                                  unsigned parameterIndex,
                                                  unsigned baseIndex,
                                                  unsigned heapNumDescriptors,
                                                  bool checkRetrieved = true);
  std::vector<unsigned> getBindlessDescriptorIndexes(unsigned rootSignatureKey,
                                                     unsigned descriptorHeapKey,
                                                     D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                                     unsigned heapNumDescriptors,
                                                     bool checkRetrieved = true);
  D3D12_ROOT_SIGNATURE_DESC* getRootSignatureDesc(unsigned rootSignatureKey);

private:
  bool unboundedRetrieved(unsigned descriptorHeapKey, unsigned index);
  bool boundedRetrieved(unsigned descriptorHeapKey, unsigned index, unsigned numDescriptors);

private:
  std::unordered_map<unsigned, D3D12_ROOT_SIGNATURE_DESC*> rootSignatureDescs_;
  std::unordered_map<unsigned, unsigned> unboundedRetrieved_;
  std::unordered_map<unsigned, std::unordered_map<unsigned, unsigned>> boundedRetrieved_;
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
