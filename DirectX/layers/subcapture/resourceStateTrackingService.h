// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include <optional>

namespace gits {
namespace DirectX {

class StateTrackingService;

class ResourceStateTrackingService {
public:
  struct SubresourceState {
    D3D12_RESOURCE_STATES State{};
    D3D12_BARRIER_LAYOUT Layout{};
    bool Enhanced{};
  };
  struct ResourceStates {
    std::vector<SubresourceState> SubresourceStates;
    bool AllEqual{true};
    bool IsBuffer{};
  };

public:
  ResourceStateTrackingService(StateTrackingService& stateService) : m_StateService(stateService) {}
  void AddResource(GITSKey deviceKey,
                   ID3D12Resource* resource,
                   GITSKey resourceKey,
                   D3D12_RESOURCE_STATES initialState,
                   bool recreateState);
  void AddResource(GITSKey deviceKey,
                   ID3D12Resource* resource,
                   GITSKey resourceKey,
                   D3D12_BARRIER_LAYOUT initialState,
                   bool recreateState);
  void ResourceBarrier(GITSKey commandListKey,
                       D3D12_RESOURCE_BARRIER* barriers,
                       std::vector<GITSKey>& resourceKeys,
                       std::vector<GITSKey>& resourceAfterKeys);
  void ResourceBarrier(GITSKey commandListKey,
                       D3D12_BARRIER_GROUP* barriers,
                       unsigned barriersNum,
                       std::vector<GITSKey>& resourceKeys);
  void ExecuteCommandLists(std::vector<GITSKey>& commandListKeys);
  void DestroyResource(GITSKey resourceKey);
  ResourceStates& GetResourceStates(GITSKey resourceKey);
  D3D12_RESOURCE_STATES GetResourceState(GITSKey resourceKey);
  D3D12_BARRIER_LAYOUT GetResourceLayout(GITSKey resourceKey);
  void RestoreResourceStates(const std::vector<unsigned>& orderedResources);
  void RestoreBackBufferState(GITSKey commandQueueKey,
                              GITSKey resourceKey,
                              D3D12_RESOURCE_STATES beforeState);

private:
  void ResourceBarrier(std::vector<D3D12_RESOURCE_BARRIER>& barriers,
                       std::vector<GITSKey>& resourceKeys,
                       std::vector<GITSKey>& resourceAfterKeys);
  void ResourceBarrier(std::vector<D3D12_TEXTURE_BARRIER>& barriers,
                       std::vector<GITSKey>& resourceKeys);
  D3D12_RESOURCE_STATES GetResourceState(D3D12_BARRIER_LAYOUT layout);
  D3D12_BARRIER_LAYOUT GetResourceLayout(D3D12_RESOURCE_STATES layout);
  GITSKey GetDeviceKeyForRestore() const;

private:
  struct ResourceBarriers {
    std::vector<D3D12_RESOURCE_BARRIER> Barriers;
    std::vector<D3D12_TEXTURE_BARRIER> Layouts;
    std::vector<GITSKey> ResourceKeys;
    std::vector<GITSKey> ResourceAfterKeys;
  };
  std::unordered_map<GITSKey, std::vector<ResourceBarriers>> m_BarriersByCommandList;

  StateTrackingService& m_StateService;
  std::unordered_map<GITSKey, ResourceStates> m_ResourceStates;
  std::unordered_set<unsigned> m_RecreateStateResources;
  GITSKey m_DeviceKey{};

  using AliasingBarrierKeys = std::pair<GITSKey, GITSKey>;
  std::map<AliasingBarrierKeys, unsigned> m_AliasingBarriersCounted;
  std::vector<std::pair<AliasingBarrierKeys, unsigned>> m_AliasingBarriersOrdered;
};

} // namespace DirectX
} // namespace gits
