// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <set>

namespace gits {
namespace DirectX {

class StateTrackingService;

class ResourceStateTrackingService {
public:
  struct ResourceStates {
    std::vector<D3D12_RESOURCE_STATES> subresourceStates;
    bool allEqual{true};
  };

public:
  ResourceStateTrackingService(StateTrackingService& stateService) : stateService_(stateService) {}
  void addResource(unsigned deviceKey,
                   ID3D12Resource* resource,
                   unsigned resourceKey,
                   D3D12_RESOURCE_STATES initialState,
                   bool recreateState);
  void addResource(unsigned deviceKey,
                   ID3D12Resource* resource,
                   unsigned resourceKey,
                   D3D12_BARRIER_LAYOUT initialState,
                   bool recreateState);
  void resourceBarrier(unsigned commandListKey,
                       D3D12_RESOURCE_BARRIER* barriers,
                       std::vector<unsigned>& resourceKeys,
                       std::vector<unsigned>& resourceAfterKeys);
  void executeCommandLists(std::vector<unsigned>& commandListKeys);
  void destroyResource(unsigned resourceKey);
  ResourceStates& getResourceStates(unsigned resourceKey);
  D3D12_RESOURCE_STATES getResourceState(unsigned resourceKey);
  D3D12_RESOURCE_STATES getSubresourceState(unsigned resourceKey, unsigned subresource);
  D3D12_BARRIER_LAYOUT getResourceLayout(unsigned resourceKey);
  void restoreResourceStates();

private:
  unsigned getSubresourcesCount(ID3D12Resource* resource);
  void resourceBarrier(std::vector<D3D12_RESOURCE_BARRIER>& barriers,
                       std::vector<unsigned>& resourceKeys,
                       std::vector<unsigned>& resourceAfterKeys);
  D3D12_RESOURCE_STATES getResourceState(D3D12_BARRIER_LAYOUT layout);
  D3D12_BARRIER_LAYOUT getResourceLayout(D3D12_RESOURCE_STATES layout);

private:
  struct ResourceBarriers {
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    std::vector<unsigned> resourceKeys;
    std::vector<unsigned> resourceAfterKeys;
  };
  std::unordered_map<unsigned, std::vector<ResourceBarriers>> barriersByCommandList_;

  StateTrackingService& stateService_;
  std::unordered_map<unsigned, ResourceStates> resourceStates_;
  std::unordered_set<unsigned> recreateStateResources_;
  unsigned deviceKey_{};

  using AliasingBarrierKeys = std::pair<unsigned, unsigned>;
  std::map<AliasingBarrierKeys, unsigned> aliasingBarriersCounted_;
  std::vector<std::pair<AliasingBarrierKeys, unsigned>> aliasingBarriersOrdered_;

  std::unordered_map<DXGI_FORMAT, unsigned> planesByFormat_;
};

} // namespace DirectX
} // namespace gits
