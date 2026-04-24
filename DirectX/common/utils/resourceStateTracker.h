// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"
#include "capturePlayerGpuAddressService.h"

#include <vector>
#include <unordered_map>
#include <map>
#include <optional>

namespace gits {
namespace DirectX {

struct BarrierState {
  BarrierState() {}
  BarrierState(D3D12_RESOURCE_STATES state_) : State(state_) {}
  BarrierState(D3D12_BARRIER_LAYOUT layout_, D3D12_BARRIER_SYNC sync_, D3D12_BARRIER_ACCESS access_)
      : Layout(layout_), Sync(sync_), Access(access_) {}
  D3D12_RESOURCE_STATES State{};
  D3D12_BARRIER_LAYOUT Layout{};
  D3D12_BARRIER_SYNC Sync{};
  D3D12_BARRIER_ACCESS Access{};
  bool Enhanced{};
};

class ResourceStateTracker {
public:
  void AddResource(ID3D12Resource* resource,
                   unsigned resourceKey,
                   D3D12_RESOURCE_STATES initialState);
  void AddResource(ID3D12Resource* resource,
                   unsigned resourceKey,
                   D3D12_BARRIER_LAYOUT initialState);
  void ResourceBarrier(ID3D12GraphicsCommandList* commandList,
                       D3D12_RESOURCE_BARRIER* barriers,
                       unsigned barriersNum,
                       unsigned* resourceKeys);
  void ResourceBarrier(ID3D12GraphicsCommandList* commandList,
                       D3D12_BARRIER_GROUP* barriers,
                       unsigned barriersNum,
                       unsigned* resourceKeys);
  void ExecuteCommandLists(ID3D12GraphicsCommandList** commandLists, unsigned commandListNum);
  BarrierState GetResourceState(ID3D12GraphicsCommandList* commandList, unsigned resourceKey);
  BarrierState GetSubresourceState(ID3D12GraphicsCommandList* commandList,
                                   unsigned resourceKey,
                                   unsigned subresource);

private:
  struct ResourceStates {
    std::vector<BarrierState> SubresourceStates;
    bool AllEqual{true};
  };
  using ResourceStatesByKey = std::unordered_map<unsigned, ResourceStates>;
  ResourceStatesByKey m_ResourceStates;
  std::unordered_map<ID3D12GraphicsCommandList*, ResourceStatesByKey> m_ResourceStatesByCommandList;
};

BarrierState GetAdjustedCurrentState(ResourceStateTracker& stateTracker,
                                     ID3D12GraphicsCommandList* commandList,
                                     ID3D12Resource* resource,
                                     unsigned resourceKey,
                                     D3D12_RESOURCE_STATES expectedState,
                                     bool resourceOverlapping);

BarrierState GetAdjustedCurrentState(ResourceStateTracker& stateTracker,
                                     CapturePlayerGpuAddressService& addressService,
                                     ID3D12GraphicsCommandList* commandList,
                                     D3D12_GPU_VIRTUAL_ADDRESS captureGpuAddress,
                                     ID3D12Resource* resource,
                                     unsigned resourceKey,
                                     D3D12_RESOURCE_STATES expectedState);

BarrierState GetAdjustedCurrentState(ResourceStateTracker& stateTracker,
                                     CapturePlayerGpuAddressService& addressService,
                                     ID3D12GraphicsCommandList* commandList,
                                     ID3D12Resource* resource,
                                     UINT64 resourceOffset,
                                     unsigned resourceKey,
                                     D3D12_RESOURCE_STATES expectedState);

} // namespace DirectX
} // namespace gits
