// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

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
  std::vector<unsigned> GetDescriptorTableIndexes(GITSKey rootSignatureKey,
                                                  GITSKey descriptorHeapKey,
                                                  unsigned parameterIndex,
                                                  unsigned baseIndex,
                                                  unsigned heapNumDescriptors,
                                                  bool checkRetrieved = true,
                                                  bool* unbounded = nullptr);
  std::vector<unsigned> GetBindlessDescriptorIndexes(GITSKey rootSignatureKey,
                                                     GITSKey descriptorHeapKey,
                                                     D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                                     unsigned heapNumDescriptors,
                                                     bool checkRetrieved = true);
  D3D12_ROOT_SIGNATURE_DESC* GetRootSignatureDesc(GITSKey rootSignatureKey);

private:
  bool UnboundedRetrieved(GITSKey descriptorHeapKey, unsigned index);
  bool BoundedRetrieved(GITSKey descriptorHeapKey, unsigned index, unsigned numDescriptors);

private:
  std::unordered_map<GITSKey, D3D12_ROOT_SIGNATURE_DESC*> m_RootSignatureDescs;
  std::unordered_map<GITSKey, GITSKey> m_UnboundedRetrieved;
  std::unordered_map<GITSKey, std::unordered_map<GITSKey, unsigned>> m_BoundedRetrieved;
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
