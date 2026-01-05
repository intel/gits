// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

void ResourceStateTracker::addResource(unsigned resourceKey, D3D12_BARRIER_LAYOUT initialState) {
  D3D12_RESOURCE_STATES state = getResourceState(initialState);
  addResource(resourceKey, state);
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

D3D12_RESOURCE_STATES ResourceStateTracker::getResourceState(D3D12_BARRIER_LAYOUT layout) {

  D3D12_RESOURCE_STATES state{};

  switch (layout) {
  case D3D12_BARRIER_LAYOUT_COMMON:
    break;
  case D3D12_BARRIER_LAYOUT_GENERIC_READ:
    state = D3D12_RESOURCE_STATE_GENERIC_READ;
    break;
  case D3D12_BARRIER_LAYOUT_RENDER_TARGET:
    state = D3D12_RESOURCE_STATE_RENDER_TARGET;
    break;
  case D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS:
    state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    break;
  case D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE:
    state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    break;
  case D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ:
    state = D3D12_RESOURCE_STATE_DEPTH_READ;
    break;
  case D3D12_BARRIER_LAYOUT_SHADER_RESOURCE:
    state =
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    break;
  case D3D12_BARRIER_LAYOUT_COPY_SOURCE:
    state = D3D12_RESOURCE_STATE_COPY_SOURCE;
    break;
  case D3D12_BARRIER_LAYOUT_COPY_DEST:
    state = D3D12_RESOURCE_STATE_COPY_DEST;
    break;
  case D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE:
    state = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    break;
  case D3D12_BARRIER_LAYOUT_RESOLVE_DEST:
    state = D3D12_RESOURCE_STATE_RESOLVE_DEST;
    break;
  case D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE:
    state = D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
    break;
  default:
    static bool logged = false;
    if (!logged) {
      LOG_ERROR << "Barrier layout not handled " << layout << "!";
      logged = true;
    }
  }

  return state;
}

} // namespace DirectX
} // namespace gits
