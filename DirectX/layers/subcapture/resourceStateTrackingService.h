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
  void AddResource(ObjectKey deviceKey,
                   ID3D12Resource* resource,
                   ObjectKey resourceKey,
                   D3D12_RESOURCE_STATES initialState,
                   bool recreateState);
  void AddResource(ObjectKey deviceKey,
                   ID3D12Resource* resource,
                   ObjectKey resourceKey,
                   D3D12_BARRIER_LAYOUT initialState,
                   bool recreateState);
  void ResourceBarrier(ObjectKey commandListKey,
                       D3D12_RESOURCE_BARRIER* barriers,
                       std::vector<ObjectKey>& resourceKeys,
                       std::vector<ObjectKey>& resourceAfterKeys);
  void ResourceBarrier(ObjectKey commandListKey,
                       D3D12_BARRIER_GROUP* barriers,
                       unsigned barriersNum,
                       std::vector<ObjectKey>& resourceKeys);
  void ExecuteCommandLists(std::vector<ObjectKey>& commandListKeys);
  void DestroyResource(ObjectKey resourceKey);
  ResourceStates& GetResourceStates(ObjectKey resourceKey);
  D3D12_RESOURCE_STATES GetResourceState(ObjectKey resourceKey);
  D3D12_BARRIER_LAYOUT GetResourceLayout(ObjectKey resourceKey);
  void RestoreResourceStates(const std::vector<ObjectKey>& orderedResources);
  void RestoreBackBufferState(ObjectKey commandQueueKey,
                              ObjectKey resourceKey,
                              D3D12_RESOURCE_STATES beforeState);

private:
  void ResourceBarrier(std::vector<D3D12_RESOURCE_BARRIER>& barriers,
                       std::vector<ObjectKey>& resourceKeys,
                       std::vector<ObjectKey>& resourceAfterKeys);
  void ResourceBarrier(std::vector<D3D12_TEXTURE_BARRIER>& barriers,
                       std::vector<ObjectKey>& resourceKeys);
  D3D12_RESOURCE_STATES GetResourceState(D3D12_BARRIER_LAYOUT layout);
  D3D12_BARRIER_LAYOUT GetResourceLayout(D3D12_RESOURCE_STATES layout);
  ObjectKey GetDeviceKeyForRestore() const;

private:
  struct ResourceBarriers {
    std::vector<D3D12_RESOURCE_BARRIER> Barriers;
    std::vector<D3D12_TEXTURE_BARRIER> Layouts;
    std::vector<ObjectKey> ResourceKeys;
    std::vector<ObjectKey> ResourceAfterKeys;
  };
  std::unordered_map<ObjectKey, std::vector<ResourceBarriers>> m_BarriersByCommandList;

  StateTrackingService& m_StateService;
  std::unordered_map<ObjectKey, ResourceStates> m_ResourceStates;
  std::unordered_set<ObjectKey> m_RecreateStateResources;
  ObjectKey m_DeviceKey{};

  using AliasingBarrierKeys = std::pair<ObjectKey, ObjectKey>;
  std::map<AliasingBarrierKeys, unsigned> m_AliasingBarriersCounted;
  std::vector<std::pair<AliasingBarrierKeys, unsigned>> m_AliasingBarriersOrdered;
};

} // namespace DirectX
} // namespace gits
