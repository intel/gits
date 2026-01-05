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
  void addResource(unsigned resourceKey, D3D12_RESOURCE_STATES initialState);
  void addResource(unsigned resourceKey, D3D12_BARRIER_LAYOUT initialState);
  void resourceBarrier(ID3D12GraphicsCommandList* commandList,
                       D3D12_RESOURCE_BARRIER* barriers,
                       unsigned barriersNum,
                       unsigned* resourceKeys);
  void executeCommandLists(ID3D12GraphicsCommandList** commandLists, unsigned commandListNum);
  void destroyResource(unsigned resourceKey);
  D3D12_RESOURCE_STATES getResourceState(ID3D12GraphicsCommandList* commandList,
                                         unsigned resourceKey,
                                         unsigned subresource);

private:
  D3D12_RESOURCE_STATES getResourceState(D3D12_BARRIER_LAYOUT layout);

private:
  using ResourceStatesBySubresource = std::map<unsigned, D3D12_RESOURCE_STATES>;
  using ResourceStatesByKey = std::unordered_map<unsigned, ResourceStatesBySubresource>;
  ResourceStatesByKey resourceStates_;
  std::unordered_map<ID3D12GraphicsCommandList*, ResourceStatesByKey> resourceStatesByCommandList_;
};

} // namespace DirectX
} // namespace gits
