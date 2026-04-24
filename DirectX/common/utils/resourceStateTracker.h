// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"

#include <vector>
#include <unordered_map>
#include <map>

namespace gits {
namespace DirectX {

class ResourceStateTracker {
public:
  void AddResource(unsigned resourceKey, D3D12_RESOURCE_STATES initialState);
  void AddResource(unsigned resourceKey, D3D12_BARRIER_LAYOUT initialState);
  void ResourceBarrier(ID3D12GraphicsCommandList* commandList,
                       D3D12_RESOURCE_BARRIER* barriers,
                       unsigned barrierCount,
                       unsigned* resourceKeys);
  void ResourceBarrier(ID3D12GraphicsCommandList* commandList,
                       D3D12_BARRIER_GROUP* barriers,
                       unsigned barrierCount,
                       unsigned* resourceKeys);
  void ExecuteCommandLists(ID3D12GraphicsCommandList** commandLists, unsigned commandListCount);
  D3D12_RESOURCE_STATES GetResourceState(ID3D12GraphicsCommandList* commandList,
                                         unsigned resourceKey,
                                         unsigned subresource);

private:
  D3D12_RESOURCE_STATES GetResourceState(D3D12_BARRIER_LAYOUT layout);

private:
  using ResourceStatesBySubresource = std::map<unsigned, D3D12_RESOURCE_STATES>;
  using ResourceStatesByKey = std::unordered_map<unsigned, ResourceStatesBySubresource>;
  ResourceStatesByKey m_ResourceStates;
  std::unordered_map<ID3D12GraphicsCommandList*, ResourceStatesByKey> m_ResourceStatesByCommandList;
};

} // namespace DirectX
} // namespace gits
