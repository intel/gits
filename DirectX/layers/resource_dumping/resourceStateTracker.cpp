// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceStateTracker.h"
#include "gits.h"

namespace gits {
namespace DirectX {

void ResourceStateTracker::addResource(unsigned resourceKey, D3D12_RESOURCE_STATES initialState) {
  resourceStates_[resourceKey][D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES] = initialState;
}

void ResourceStateTracker::destroyResource(unsigned resourceKey) {
  resourceStates_.erase(resourceKey);
}

void ResourceStateTracker::resourceBarrier(ID3D12GraphicsCommandList* commandList,
                                           D3D12_RESOURCE_BARRIER* barriers,
                                           unsigned barriersNum,
                                           unsigned* resourceKeys) {
  ResourceStatesByKey& resourceStates =
      commandList ? resourceStatesByCommandList_[commandList] : resourceStates_;

  for (unsigned i = 0; i < barriersNum; ++i) {
    if (barriers[i].Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION) {
      std::map<unsigned, D3D12_RESOURCE_STATES>& subresourceBarriers =
          resourceStates[resourceKeys[i]];
      unsigned subresource = barriers[i].Transition.Subresource;
      if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
        subresourceBarriers.clear();
      }
      D3D12_RESOURCE_STATES stateAfter = barriers[i].Transition.StateAfter;
      subresourceBarriers[subresource] = stateAfter;
    } else if (barriers[i].Type == D3D12_RESOURCE_BARRIER_TYPE_UAV) {
      std::map<unsigned, D3D12_RESOURCE_STATES>& subresourceBarriers =
          resourceStates[resourceKeys[i]];
      subresourceBarriers.clear();
      subresourceBarriers[D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES] =
          D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
  }
}

void ResourceStateTracker::executeCommandLists(ID3D12GraphicsCommandList** commandLists,
                                               unsigned commandListNum) {
  for (unsigned i = 0; i < commandListNum; ++i) {
    auto itCommandList = resourceStatesByCommandList_.find(commandLists[i]);
    if (itCommandList != resourceStatesByCommandList_.end()) {
      for (auto& itResource : itCommandList->second) {
        ResourceStatesBySubresource& resourceStatesOnCommandList = itResource.second;
        ResourceStatesBySubresource& resourceStates = resourceStates_[itResource.first];
        for (auto& it : resourceStatesOnCommandList) {
          if (it.first == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
            resourceStates.clear();
          }
          resourceStates[it.first] = it.second;
        }
      }
    }
  }
}

D3D12_RESOURCE_STATES ResourceStateTracker::getResourceState(ID3D12GraphicsCommandList* commandList,
                                                             unsigned resourceKey,
                                                             unsigned subresource) {
  bool found = false;
  ResourceStatesByKey::iterator itResource;
  if (commandList) {
    auto itCommandList = resourceStatesByCommandList_.find(commandList);
    if (itCommandList != resourceStatesByCommandList_.end()) {
      itResource = itCommandList->second.find(resourceKey);
      if (itResource != itCommandList->second.end()) {
        found = true;
      }
    }
  }
  if (!found) {
    itResource = resourceStates_.find(resourceKey);
  }
  GITS_ASSERT(found || itResource != resourceStates_.end());

  auto it = itResource->second.find(subresource);
  if (it == itResource->second.end()) {
    it = itResource->second.find(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
  }
  GITS_ASSERT(it != itResource->second.end());

  return it->second;
}

} // namespace DirectX
} // namespace gits
