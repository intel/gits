// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"

#include <unordered_map>

namespace gits {
namespace DirectX {

class RootSignatureService {
public:
  RootSignatureService() {}
  RootSignatureService(const RootSignatureService&) = delete;
  RootSignatureService& operator=(const RootSignatureService&) = delete;
  ~RootSignatureService();
  void createRootSignature(ID3D12DeviceCreateRootSignatureCommand& c);
  std::vector<unsigned> getDescriptorTableIndexes(unsigned rootSignatureKey,
                                                  unsigned descriptorHeapKey,
                                                  unsigned parameterIndex,
                                                  unsigned baseIndex,
                                                  unsigned heapNumDescriptors);

private:
  bool unboundedRetrieved(unsigned descriptorHeapKey, unsigned index);
  bool boundedRetrieved(unsigned descriptorHeapKey, unsigned index, unsigned numDescriptors);

private:
  std::unordered_map<unsigned, D3D12_ROOT_SIGNATURE_DESC*> rootSignatureDescs_;
  std::unordered_map<unsigned, unsigned> unboundedRetrieved_;
  std::unordered_map<unsigned, std::unordered_map<unsigned, unsigned>> boundedRetrieved_;
};

} // namespace DirectX
} // namespace gits
