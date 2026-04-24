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
  DescriptorRootSignatureService() = default;
  DescriptorRootSignatureService(const DescriptorRootSignatureService&) = delete;
  DescriptorRootSignatureService& operator=(const DescriptorRootSignatureService&) = delete;
  ~DescriptorRootSignatureService();
  void CreateRootSignature(ID3D12DeviceCreateRootSignatureCommand& command);
  std::vector<unsigned> GetDescriptorTableIndexes(unsigned rootSignatureKey,
                                                  unsigned descriptorHeapKey,
                                                  unsigned parameterIndex,
                                                  unsigned baseIndex,
                                                  unsigned heapNumDescriptors,
                                                  bool checkRetrieved = true);
  std::vector<unsigned> GetBindlessDescriptorIndexes(unsigned rootSignatureKey,
                                                     unsigned descriptorHeapKey,
                                                     D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                                     unsigned heapNumDescriptors,
                                                     bool checkRetrieved = true);
  D3D12_ROOT_SIGNATURE_DESC* GetRootSignatureDesc(unsigned rootSignatureKey);

private:
  bool UnboundedRetrieved(unsigned descriptorHeapKey, unsigned index);
  bool BoundedRetrieved(unsigned descriptorHeapKey, unsigned index, unsigned numDescriptors);

private:
  std::unordered_map<unsigned, D3D12_ROOT_SIGNATURE_DESC*> m_RootSignatureDescs;
  std::unordered_map<unsigned, unsigned> m_UnboundedRetrieved;
  std::unordered_map<unsigned, std::unordered_map<unsigned, unsigned>> m_BoundedRetrieved;
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
