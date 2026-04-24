// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceStateTracker.h"
#include "resourceSizeUtils.h"
#include "resourceStateEnhanced.h"
#include "log.h"

namespace gits {
namespace DirectX {

void ResourceStateTracker::AddResource(ID3D12Resource* resource,
                                       unsigned resourceKey,
                                       D3D12_RESOURCE_STATES initialState) {
  ResourceStates& states = m_ResourceStates[resourceKey];
  states.SubresourceStates.resize(GetSubresourcesCount(resource));
  for (unsigned i = 0; i < states.SubresourceStates.size(); ++i) {
    states.SubresourceStates[i].State = initialState;
    states.SubresourceStates[i].Enhanced = false;
  }
}

void ResourceStateTracker::AddResource(ID3D12Resource* resource,
                                       unsigned resourceKey,
                                       D3D12_BARRIER_LAYOUT initialState) {
  ResourceStates& states = m_ResourceStates[resourceKey];
  states.CreatedEnhanced = true;
  states.SubresourceStates.resize(GetSubresourcesCount(resource));
  for (unsigned i = 0; i < states.SubresourceStates.size(); ++i) {
    states.SubresourceStates[i].Layout = initialState;
    states.SubresourceStates[i].Sync = D3D12_BARRIER_SYNC_NONE;
    states.SubresourceStates[i].Access = D3D12_BARRIER_ACCESS_NO_ACCESS;
    states.SubresourceStates[i].Enhanced = true;
  }
}

void ResourceStateTracker::ResourceBarrier(ID3D12GraphicsCommandList* commandList,
                                           D3D12_RESOURCE_BARRIER* barriers,
                                           unsigned barriersNum,
                                           unsigned* resourceKeys) {
  ResourceStatesByKey* resourceStatesByCommandList =
      commandList ? &m_ResourceStatesByCommandList[commandList] : nullptr;

  for (unsigned i = 0; i < barriersNum; ++i) {
    if (!resourceKeys[i]) {
      continue;
    }
    ResourceStates* states{};
    if (resourceStatesByCommandList) {
      auto it = resourceStatesByCommandList->find(resourceKeys[i]);
      if (it == resourceStatesByCommandList->end()) {
        it = m_ResourceStates.find(resourceKeys[i]);
        if (it == m_ResourceStates.end()) {
          continue;
        }
        states = &resourceStatesByCommandList->insert(std::make_pair(resourceKeys[i], it->second))
                      .first->second;
      } else {
        states = &it->second;
      }
    } else {
      auto it = m_ResourceStates.find(resourceKeys[i]);
      if (it == m_ResourceStates.end()) {
        continue;
      }
      states = &it->second;
    }

    if (barriers[i].Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION) {
      unsigned subresource = barriers[i].Transition.Subresource;
      if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
        states->AllEqual = true;
        for (unsigned j = 0; j < states->SubresourceStates.size(); ++j) {
          states->SubresourceStates[j].State = barriers[i].Transition.StateAfter;
          states->SubresourceStates[j].Enhanced = false;
        }
      } else {
        states->SubresourceStates[subresource].State = barriers[i].Transition.StateAfter;
        states->SubresourceStates[subresource].Enhanced = false;
      }
    } else if (barriers[i].Type == D3D12_RESOURCE_BARRIER_TYPE_UAV) {
      states->AllEqual = true;
      for (unsigned j = 0; j < states->SubresourceStates.size(); ++j) {
        states->SubresourceStates[j].State = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        states->SubresourceStates[j].Enhanced = false;
      }
    }
  }
}

void ResourceStateTracker::ResourceBarrier(ID3D12GraphicsCommandList* commandList,
                                           D3D12_BARRIER_GROUP* barriers,
                                           unsigned barriersNum,
                                           unsigned* resourceKeys) {
  ResourceStatesByKey* resourceStatesByCommandList =
      commandList ? &m_ResourceStatesByCommandList[commandList] : nullptr;

  auto getResourceStates = [&](unsigned resourceKey) {
    ResourceStates* states{};
    if (resourceStatesByCommandList) {
      auto it = resourceStatesByCommandList->find(resourceKey);
      if (it == resourceStatesByCommandList->end()) {
        it = m_ResourceStates.find(resourceKey);
        if (it != m_ResourceStates.end()) {
          states = &resourceStatesByCommandList->insert(std::make_pair(resourceKey, it->second))
                        .first->second;
        }
      } else {
        states = &it->second;
      }
    } else {
      auto it = m_ResourceStates.find(resourceKey);
      if (it != m_ResourceStates.end()) {
        states = &it->second;
      }
    }
    return states;
  };

  unsigned resourceKeyIndex = 0;
  for (unsigned i = 0; i < barriersNum; ++i) {
    if (barriers[i].Type == D3D12_BARRIER_TYPE_BUFFER) {
      for (unsigned j = 0; j < barriers[i].NumBarriers; ++j, ++resourceKeyIndex) {
        ResourceStates* states = getResourceStates(resourceKeys[resourceKeyIndex]);
        if (!states) {
          continue;
        }
        const D3D12_BUFFER_BARRIER& barrier = barriers[i].pBufferBarriers[j];
        states->SubresourceStates[0].Sync = barrier.SyncAfter;
        states->SubresourceStates[0].Access = barrier.AccessAfter;
      }
    } else if (barriers[i].Type == D3D12_BARRIER_TYPE_TEXTURE) {
      for (unsigned j = 0; j < barriers[i].NumBarriers; ++j, ++resourceKeyIndex) {
        ResourceStates* states = getResourceStates(resourceKeys[resourceKeyIndex]);
        if (!states) {
          continue;
        }
        const D3D12_TEXTURE_BARRIER& barrier = barriers[i].pTextureBarriers[j];
        const D3D12_BARRIER_SUBRESOURCE_RANGE& range = barrier.Subresources;
        if (range.NumMipLevels == 0) {
          if (range.IndexOrFirstMipLevel == 0XFFFFFFFF) {
            states->AllEqual = true;
            for (unsigned k = 0; k < states->SubresourceStates.size(); ++k) {
              states->SubresourceStates[k].Layout = barrier.LayoutAfter;
              states->SubresourceStates[k].Sync = barrier.SyncAfter;
              states->SubresourceStates[k].Access = barrier.AccessAfter;
              states->SubresourceStates[k].Enhanced = true;
            }
          } else {
            states->AllEqual = false;
            states->SubresourceStates[range.IndexOrFirstMipLevel].Layout = barrier.LayoutAfter;
            states->SubresourceStates[range.IndexOrFirstMipLevel].Sync = barrier.SyncAfter;
            states->SubresourceStates[range.IndexOrFirstMipLevel].Access = barrier.AccessAfter;
            states->SubresourceStates[range.IndexOrFirstMipLevel].Enhanced = true;
          }
        } else {
          D3D12_RESOURCE_DESC desc = barrier.pResource->GetDesc();
          unsigned arraySize =
              desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : desc.DepthOrArraySize;
          for (unsigned planeSlice = range.FirstPlane;
               planeSlice < range.FirstPlane + range.NumPlanes; ++planeSlice) {
            for (unsigned arraySlice = range.FirstArraySlice;
                 arraySlice < range.FirstArraySlice + range.NumArraySlices; ++arraySlice) {
              for (unsigned mipLevel = range.IndexOrFirstMipLevel;
                   mipLevel < range.IndexOrFirstMipLevel + range.NumMipLevels; ++mipLevel) {
                unsigned subresourceIndex = mipLevel + (arraySlice * desc.MipLevels) +
                                            (planeSlice * desc.MipLevels * arraySize);
                states->SubresourceStates[subresourceIndex].Layout = barrier.LayoutAfter;
                states->SubresourceStates[subresourceIndex].Sync = barrier.SyncAfter;
                states->SubresourceStates[subresourceIndex].Access = barrier.AccessAfter;
                states->SubresourceStates[subresourceIndex].Enhanced = true;
              }
            }
          }
        }
      }
    }
  }
}

void ResourceStateTracker::ExecuteCommandLists(ID3D12GraphicsCommandList** commandLists,
                                               unsigned commandListNum) {
  for (unsigned i = 0; i < commandListNum; ++i) {
    auto itCommandList = m_ResourceStatesByCommandList.find(commandLists[i]);
    if (itCommandList != m_ResourceStatesByCommandList.end()) {
      for (auto& itResource : itCommandList->second) {
        ResourceStates& resourceStatesOnCommandList = itResource.second;
        ResourceStates& resourceStates = m_ResourceStates[itResource.first];
        resourceStates.AllEqual = resourceStatesOnCommandList.AllEqual;
        resourceStates.CreatedEnhanced = resourceStatesOnCommandList.CreatedEnhanced;
        for (unsigned i = 0; i < resourceStates.SubresourceStates.size(); ++i) {
          resourceStates.SubresourceStates[i] = resourceStatesOnCommandList.SubresourceStates[i];
        }
      }
    }
  }
}

BarrierState ResourceStateTracker::GetResourceState(ID3D12GraphicsCommandList* commandList,
                                                    unsigned resourceKey) {
  return GetSubresourceState(commandList, resourceKey, 0);
}

BarrierState ResourceStateTracker::GetSubresourceState(ID3D12GraphicsCommandList* commandList,
                                                       unsigned resourceKey,
                                                       unsigned subresource) {
  bool found = false;
  ResourceStatesByKey::iterator itState;
  if (commandList) {
    auto itCommandList = m_ResourceStatesByCommandList.find(commandList);
    if (itCommandList != m_ResourceStatesByCommandList.end()) {
      itState = itCommandList->second.find(resourceKey);
      if (itState != itCommandList->second.end()) {
        found = true;
      }
    }
  }
  if (!found) {
    itState = m_ResourceStates.find(resourceKey);
    found = true;
  }
  GITS_ASSERT(found);

  return itState->second.SubresourceStates[subresource];
}

BarrierState GetAdjustedCurrentState(ResourceStateTracker& stateTracker,
                                     ID3D12GraphicsCommandList* commandList,
                                     ID3D12Resource* resource,
                                     unsigned resourceKey,
                                     D3D12_RESOURCE_STATES expectedState,
                                     bool resourceOverlapping) {
  BarrierState barrierState = stateTracker.GetResourceState(commandList, resourceKey);
  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    GITS_ASSERT(false, "Not implemented.");
  }

  if (!resourceOverlapping) {
    if (!barrierState.Enhanced) {
      if (barrierState.State != D3D12_RESOURCE_STATE_GENERIC_READ) {
        barrierState.State = expectedState;
      }
    } else {
      barrierState.Access = ResourceStateEnhanced::GetAccess(expectedState);
      barrierState.Sync = ResourceStateEnhanced::GetSync(expectedState);
    }
  } else {
    if (!barrierState.Enhanced) {
      if (barrierState.State != expectedState) {
        static bool logged = false;
        if (!logged) {
          LOG_WARNING << "Resource barrier - state of overlapped resource different than expected.";
          logged = true;
        }
      }
      if (commandList->GetType() == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        if ((barrierState.State & ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) !=
            barrierState.State) {
          barrierState.State &= ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
          static bool logged = false;
          if (!logged) {
            LOG_WARNING << "Resource barrier - state of overlapped resource adjusted for compute "
                           "command list.";
            logged = true;
          }
        }
      }
    } else {
      GITS_ASSERT(false, "Not implemented.");
    }
  }

  return barrierState;
}

BarrierState GetAdjustedCurrentState(ResourceStateTracker& stateTracker,
                                     CapturePlayerGpuAddressService& addressService,
                                     ID3D12GraphicsCommandList* commandList,
                                     D3D12_GPU_VIRTUAL_ADDRESS captureGpuAddress,
                                     ID3D12Resource* resource,
                                     unsigned resourceKey,
                                     D3D12_RESOURCE_STATES expectedState) {
  bool overlapping = false;
  CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
      addressService.GetResourceInfoByCaptureAddress(captureGpuAddress);
  if (resourceInfo) {
    overlapping = resourceInfo->Overlapping();
  }
  return GetAdjustedCurrentState(stateTracker, commandList, resource, resourceKey, expectedState,
                                 overlapping);
}

BarrierState GetAdjustedCurrentState(ResourceStateTracker& stateTracker,
                                     CapturePlayerGpuAddressService& addressService,
                                     ID3D12GraphicsCommandList* commandList,
                                     ID3D12Resource* resource,
                                     UINT64 resourceOffset,
                                     unsigned resourceKey,
                                     D3D12_RESOURCE_STATES expectedState) {
  D3D12_GPU_VIRTUAL_ADDRESS playerGpuAddress = resource->GetGPUVirtualAddress();
  GITS_ASSERT(playerGpuAddress);
  playerGpuAddress += resourceOffset;
  bool overlapping = false;
  CapturePlayerGpuAddressService::ResourceInfo* resourceInfo =
      addressService.GetResourceInfoByPlayerAddress(playerGpuAddress);
  if (resourceInfo) {
    overlapping = resourceInfo->Overlapping();
  }
  return GetAdjustedCurrentState(stateTracker, commandList, resource, resourceKey, expectedState,
                                 overlapping);
}

} // namespace DirectX
} // namespace gits
