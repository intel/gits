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
    bool InitialEnhanced{};
  };

public:
  ResourceStateTrackingService(StateTrackingService& stateService) : m_StateService(stateService) {}
  void AddResource(unsigned DeviceKey,
                   ID3D12Resource* resource,
                   unsigned ResourceKey,
                   D3D12_RESOURCE_STATES initialState,
                   bool recreateState);
  void AddResource(unsigned DeviceKey,
                   ID3D12Resource* resource,
                   unsigned ResourceKey,
                   D3D12_BARRIER_LAYOUT initialState,
                   bool recreateState);
  void ResourceBarrier(unsigned commandListKey,
                       D3D12_RESOURCE_BARRIER* barriers,
                       std::vector<unsigned>& ResourceKeys,
                       std::vector<unsigned>& ResourceAfterKeys);
  void ResourceBarrier(unsigned commandListKey,
                       D3D12_BARRIER_GROUP* barriers,
                       unsigned barriersNum,
                       std::vector<unsigned>& ResourceKeys);
  void ExecuteCommandLists(std::vector<unsigned>& commandListKeys);
  void DestroyResource(unsigned ResourceKey);
  ResourceStates& GetResourceStates(unsigned ResourceKey);
  D3D12_RESOURCE_STATES GetResourceState(unsigned ResourceKey);
  D3D12_BARRIER_LAYOUT GetResourceLayout(unsigned ResourceKey);
  void RestoreResourceStates(const std::vector<unsigned>& orderedResources);
  void RestoreBackBufferState(unsigned commandQueueKey,
                              unsigned ResourceKey,
                              D3D12_RESOURCE_STATES beforeState);

private:
  unsigned GetSubresourcesCount(ID3D12Resource* resource);
  void ResourceBarrier(std::vector<D3D12_RESOURCE_BARRIER>& barriers,
                       std::vector<unsigned>& ResourceKeys,
                       std::vector<unsigned>& ResourceAfterKeys);
  void ResourceBarrier(std::vector<D3D12_TEXTURE_BARRIER>& barriers,
                       std::vector<unsigned>& ResourceKeys);
  D3D12_RESOURCE_STATES GetResourceState(D3D12_BARRIER_LAYOUT layout);
  D3D12_BARRIER_LAYOUT GetResourceLayout(D3D12_RESOURCE_STATES layout);
  void InsertIfNotResident(unsigned ResourceKey, std::set<unsigned>& residencyKeys);
  std::optional<unsigned> GetResidencyKeyForNotResidentResource(unsigned key);
  void RecordMakeResident(const std::set<unsigned>& keys);
  void RecordEvict(const std::set<unsigned>& keys);

private:
  struct ResourceBarriers {
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    std::vector<D3D12_TEXTURE_BARRIER> layouts;
    std::vector<unsigned> ResourceKeys;
    std::vector<unsigned> ResourceAfterKeys;
  };
  std::unordered_map<unsigned, std::vector<ResourceBarriers>> m_BarriersByCommandList;

  StateTrackingService& m_StateService;
  std::unordered_map<unsigned, ResourceStates> m_ResourceStates;
  std::unordered_set<unsigned> m_RecreateStateResources;
  unsigned m_DeviceKey{};

  using AliasingBarrierKeys = std::pair<unsigned, unsigned>;
  std::map<AliasingBarrierKeys, unsigned> m_AliasingBarriersCounted;
  std::vector<std::pair<AliasingBarrierKeys, unsigned>> m_AliasingBarriersOrdered;

  std::unordered_map<DXGI_FORMAT, unsigned> m_PlanesByFormat;
};

} // namespace DirectX
} // namespace gits
