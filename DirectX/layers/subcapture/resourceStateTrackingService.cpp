// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceStateTrackingService.h"
#include "stateTrackingService.h"
#include "commandsAuto.h"
#include "commandSerializersAuto.h"
#include "commandsCustom.h"
#include "commandSerializersCustom.h"
#include "resourceSizeUtils.h"
#include "log.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void ResourceStateTrackingService::ResourceBarrier(unsigned commandListKey,
                                                   D3D12_RESOURCE_BARRIER* barriers,
                                                   std::vector<unsigned>& resourceKeys,
                                                   std::vector<unsigned>& resourceAfterKeys) {
  ResourceBarriers resourceBarriers;
  resourceBarriers.Barriers.resize(resourceKeys.size());
  for (unsigned i = 0; i < resourceKeys.size(); ++i) {
    resourceBarriers.Barriers[i] = barriers[i];
  }
  resourceBarriers.ResourceKeys = resourceKeys;
  resourceBarriers.ResourceAfterKeys = resourceAfterKeys;
  m_BarriersByCommandList[commandListKey].push_back(std::move(resourceBarriers));
}

void ResourceStateTrackingService::ResourceBarrier(unsigned commandListKey,
                                                   D3D12_BARRIER_GROUP* barriers,
                                                   unsigned barriersNum,
                                                   std::vector<unsigned>& resourceKeys) {
  ResourceBarriers resourceBarriers;
  unsigned resourceKeyIndex = 0;
  for (unsigned i = 0; i < barriersNum; ++i) {
    D3D12_BARRIER_GROUP& barrierGroup = barriers[i];
    if (barrierGroup.Type == D3D12_BARRIER_TYPE_TEXTURE) {
      for (unsigned j = 0; j < barrierGroup.NumBarriers; ++j) {
        resourceBarriers.Layouts.push_back(barrierGroup.pTextureBarriers[j]);
        resourceBarriers.ResourceKeys.push_back(resourceKeys[resourceKeyIndex]);
        ++resourceKeyIndex;
      }
    } else if (barrierGroup.Type == D3D12_BARRIER_TYPE_BUFFER) {
      for (unsigned j = 0; j < barrierGroup.NumBarriers; ++j) {
        ++resourceKeyIndex;
      }
    }
  }
  m_BarriersByCommandList[commandListKey].push_back(std::move(resourceBarriers));
}

void ResourceStateTrackingService::ExecuteCommandLists(std::vector<unsigned>& commandListKeys) {
  for (unsigned key : commandListKeys) {
    auto it = m_BarriersByCommandList.find(key);
    if (it != m_BarriersByCommandList.end()) {
      for (ResourceBarriers& barriers : it->second) {
        if (!barriers.Barriers.empty()) {
          ResourceBarrier(barriers.Barriers, barriers.ResourceKeys, barriers.ResourceAfterKeys);
        } else {
          ResourceBarrier(barriers.Layouts, barriers.ResourceKeys);
        }
      }
      m_BarriersByCommandList.erase(it);
    }
  }
}

void ResourceStateTrackingService::ResourceBarrier(std::vector<D3D12_RESOURCE_BARRIER>& barriers,
                                                   std::vector<unsigned>& resourceKeys,
                                                   std::vector<unsigned>& resourceAfterKeys) {
  for (unsigned i = 0; i < barriers.size(); ++i) {
    if (barriers[i].Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION) {
      D3D12_RESOURCE_STATES stateAfter = barriers[i].Transition.StateAfter;
      ResourceStates& states = GetResourceStates(resourceKeys[i]);
      unsigned subresource = barriers[i].Transition.Subresource;
      if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
        states.AllEqual = true;
        for (unsigned j = 0; j < states.SubresourceStates.size(); ++j) {
          states.SubresourceStates[j].State = stateAfter;
          states.SubresourceStates[j].Enhanced = false;
        }
      } else {
        states.AllEqual = false;
        states.SubresourceStates[subresource].State = stateAfter;
        states.SubresourceStates[subresource].Enhanced = false;
      }
    } else if (barriers[i].Type == D3D12_RESOURCE_BARRIER_TYPE_ALIASING) {
      auto keys = std::make_pair(resourceKeys[i], resourceAfterKeys[i]);
      unsigned& count = m_AliasingBarriersCounted[keys];
      ++count;
      m_AliasingBarriersOrdered.push_back(std::make_pair(keys, count));
    }
  }
}

void ResourceStateTrackingService::ResourceBarrier(std::vector<D3D12_TEXTURE_BARRIER>& barriers,
                                                   std::vector<unsigned>& resourceKeys) {
  for (unsigned i = 0; i < barriers.size(); ++i) {
    ResourceStates& states = GetResourceStates(resourceKeys[i]);
    D3D12_BARRIER_SUBRESOURCE_RANGE& range = barriers[i].Subresources;
    if (range.NumMipLevels == 0) {
      if (range.IndexOrFirstMipLevel == 0XFFFFFFFF) {
        states.AllEqual = true;
        for (unsigned j = 0; j < states.SubresourceStates.size(); ++j) {
          states.SubresourceStates[j].Layout = barriers[i].LayoutAfter;
          states.SubresourceStates[j].Enhanced = true;
        }
      } else {
        states.AllEqual = false;
        states.SubresourceStates[range.IndexOrFirstMipLevel].Layout = barriers[i].LayoutAfter;
        states.SubresourceStates[range.IndexOrFirstMipLevel].Enhanced = true;
      }
    } else {
      D3D12_RESOURCE_DESC desc = barriers[i].pResource->GetDesc();
      unsigned arraySize =
          desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : desc.DepthOrArraySize;
      for (unsigned planeSlice = range.FirstPlane; planeSlice < range.FirstPlane + range.NumPlanes;
           ++planeSlice) {
        for (unsigned arraySlice = range.FirstArraySlice;
             arraySlice < range.FirstArraySlice + range.NumArraySlices; ++arraySlice) {
          for (unsigned mipLevel = range.IndexOrFirstMipLevel;
               mipLevel < range.IndexOrFirstMipLevel + range.NumMipLevels; ++mipLevel) {
            unsigned subresourceIndex = mipLevel + (arraySlice * desc.MipLevels) +
                                        (planeSlice * desc.MipLevels * arraySize);
            states.SubresourceStates[subresourceIndex].Layout = barriers[i].LayoutAfter;
            states.SubresourceStates[subresourceIndex].Enhanced = true;
          }
        }
      }
    }
  }
}

void ResourceStateTrackingService::AddResource(unsigned deviceKey,
                                               ID3D12Resource* resource,
                                               unsigned resourceKey,
                                               D3D12_RESOURCE_STATES initialState,
                                               bool recreateState) {
  if (deviceKey) {
    m_DeviceKey = deviceKey;
  }
  ResourceStates& states = m_ResourceStates[resourceKey];
  states.SubresourceStates.resize(GetSubresourcesCount(resource));
  states.IsBuffer = resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
  for (unsigned i = 0; i < states.SubresourceStates.size(); ++i) {
    states.SubresourceStates[i].State = initialState;
    states.SubresourceStates[i].Enhanced = false;
  }
  if (recreateState) {
    m_RecreateStateResources.insert(resourceKey);
  }
}

void ResourceStateTrackingService::AddResource(unsigned deviceKey,
                                               ID3D12Resource* resource,
                                               unsigned resourceKey,
                                               D3D12_BARRIER_LAYOUT initialState,
                                               bool recreateState) {
  if (deviceKey) {
    m_DeviceKey = deviceKey;
  }
  ResourceStates& states = m_ResourceStates[resourceKey];
  states.SubresourceStates.resize(GetSubresourcesCount(resource));
  states.IsBuffer = resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
  for (unsigned i = 0; i < states.SubresourceStates.size(); ++i) {
    states.SubresourceStates[i].Layout = initialState;
    states.SubresourceStates[i].Enhanced = true;
  }
  if (recreateState) {
    m_RecreateStateResources.insert(resourceKey);
  }
}

D3D12_RESOURCE_STATES ResourceStateTrackingService::GetResourceState(D3D12_BARRIER_LAYOUT layout) {
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

D3D12_BARRIER_LAYOUT ResourceStateTrackingService::GetResourceLayout(D3D12_RESOURCE_STATES state) {
  D3D12_BARRIER_LAYOUT layout = D3D12_BARRIER_LAYOUT_UNDEFINED;
  switch (state) {
  case D3D12_RESOURCE_STATE_COMMON:
    layout = D3D12_BARRIER_LAYOUT_COMMON;
    break;
  case D3D12_RESOURCE_STATE_RENDER_TARGET:
    layout = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    break;
  case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
    layout = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
    break;
  case D3D12_RESOURCE_STATE_DEPTH_WRITE:
    layout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
    break;
  case D3D12_RESOURCE_STATE_DEPTH_READ:
    layout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
    break;
  case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
    layout = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
    break;
  case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
    layout = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
    break;
  case D3D12_RESOURCE_STATE_COPY_DEST:
    layout = D3D12_BARRIER_LAYOUT_COPY_DEST;
    break;
  case D3D12_RESOURCE_STATE_COPY_SOURCE:
    layout = D3D12_BARRIER_LAYOUT_COPY_SOURCE;
    break;
  case D3D12_RESOURCE_STATE_RESOLVE_DEST:
    layout = D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
    break;
  case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:
    layout = D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE;
    break;
  case D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE:
    layout = D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE;
    break;
  }
  return layout;
}

void ResourceStateTrackingService::DestroyResource(unsigned resourceKey) {
  m_RecreateStateResources.erase(resourceKey);
}

ResourceStateTrackingService::ResourceStates& ResourceStateTrackingService::GetResourceStates(
    unsigned resourceKey) {
  auto it = m_ResourceStates.find(resourceKey);
  GITS_ASSERT(it != m_ResourceStates.end());
  return it->second;
}

D3D12_RESOURCE_STATES ResourceStateTrackingService::GetResourceState(unsigned resourceKey) {
  ResourceStates& states = GetResourceStates(resourceKey);
  if (states.SubresourceStates[0].Enhanced) {
    static bool logged = false;
    if (!logged) {
      LOG_WARNING << "ResourceStateTrackingService - converting enhanced barrier layout to legacy "
                     "barrier state.";
    }
    return GetResourceState(states.SubresourceStates[0].Layout);
  }
  return states.SubresourceStates[0].State;
}

D3D12_BARRIER_LAYOUT ResourceStateTrackingService::GetResourceLayout(unsigned resourceKey) {
  ResourceStates& states = GetResourceStates(resourceKey);
  if (!states.SubresourceStates[0].Enhanced) {
    static bool logged = false;
    if (!logged) {
      LOG_WARNING << "ResourceStateTrackingService - converting legacy barrier state to enhanced "
                     "barrier layout.";
    }
    return GetResourceLayout(states.SubresourceStates[0].State);
  }
  return states.SubresourceStates[0].Layout;
}

void ResourceStateTrackingService::RestoreResourceStates(
    const std::vector<unsigned>& orderedResources) {
  if (orderedResources.empty() && m_AliasingBarriersOrdered.empty()) {
    return;
  }

  unsigned commandQueueKey = m_StateService.GetUniqueObjectKey();
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  ID3D12DeviceCreateCommandQueueCommand createCommandQueue;
  createCommandQueue.Key = m_StateService.GetUniqueCommandKey();
  createCommandQueue.m_Object.Key = m_DeviceKey;
  createCommandQueue.m_pDesc.Value = &commandQueueDesc;
  createCommandQueue.m_riid.Value = IID_ID3D12CommandQueue;
  createCommandQueue.m_ppCommandQueue.Key = commandQueueKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateCommandQueueSerializer(createCommandQueue));

  unsigned commandAllocatorKey = m_StateService.GetUniqueObjectKey();
  ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
  createCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
  createCommandAllocator.m_Object.Key = m_DeviceKey;
  createCommandAllocator.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  createCommandAllocator.m_riid.Value = IID_ID3D12CommandAllocator;
  createCommandAllocator.m_ppCommandAllocator.Key = commandAllocatorKey;
  m_StateService.GetRecorder().Record(
      ID3D12DeviceCreateCommandAllocatorSerializer(createCommandAllocator));

  unsigned commandListKey = m_StateService.GetUniqueObjectKey();
  ID3D12DeviceCreateCommandListCommand createCommandList;
  createCommandList.Key = m_StateService.GetUniqueCommandKey();
  createCommandList.m_Object.Key = m_DeviceKey;
  createCommandList.m_nodeMask.Value = 0;
  createCommandList.m_pCommandAllocator.Key = createCommandAllocator.m_ppCommandAllocator.Key;
  createCommandList.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  createCommandList.m_pInitialState.Value = nullptr;
  createCommandList.m_riid.Value = IID_ID3D12CommandList;
  createCommandList.m_ppCommandList.Key = commandListKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateCommandListSerializer(createCommandList));

  unsigned fenceKey = m_StateService.GetUniqueObjectKey();
  ID3D12DeviceCreateFenceCommand createFence;
  createFence.Key = m_StateService.GetUniqueCommandKey();
  createFence.m_Object.Key = m_DeviceKey;
  createFence.m_InitialValue.Value = 0;
  createFence.m_Flags.Value = D3D12_FENCE_FLAG_NONE;
  createFence.m_riid.Value = IID_ID3D12Fence;
  createFence.m_ppFence.Key = fenceKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateFenceSerializer(createFence));

  std::set<unsigned> residencyKeys;

  for (unsigned resourceKey : orderedResources) {
    if (m_RecreateStateResources.find(resourceKey) == m_RecreateStateResources.end()) {
      continue;
    }
    if (!m_StateService.StateRestored(resourceKey)) {
      continue;
    }

    auto writeResourceBarrier = [&](unsigned subresource, D3D12_RESOURCE_STATES beforeState,
                                    D3D12_RESOURCE_STATES afterState) {
      InsertIfNotResident(resourceKey, residencyKeys);

      ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
      barrierCommand.Key = m_StateService.GetUniqueCommandKey();
      barrierCommand.m_Object.Key = commandListKey;
      barrierCommand.m_NumBarriers.Value = 1;
      D3D12_RESOURCE_BARRIER barrier{};
      barrierCommand.m_pBarriers.Value = &barrier;
      barrierCommand.m_pBarriers.Size = 1;
      barrierCommand.m_pBarriers.ResourceKeys.resize(1);
      barrierCommand.m_pBarriers.ResourceAfterKeys.resize(1);
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      barrier.Transition.Subresource = subresource;
      barrier.Transition.StateBefore = beforeState;
      barrier.Transition.StateAfter = afterState;
      barrierCommand.m_pBarriers.ResourceKeys[0] = resourceKey;
      m_StateService.GetRecorder().Record(
          ID3D12GraphicsCommandListResourceBarrierSerializer(barrierCommand));
    };

    auto writeResourceEnhancedBarrier = [&](unsigned subresource, D3D12_BARRIER_LAYOUT beforeLayout,
                                            D3D12_BARRIER_LAYOUT afterLayout) {
      InsertIfNotResident(resourceKey, residencyKeys);

      ID3D12GraphicsCommandList7BarrierCommand barrierCommand;
      barrierCommand.Key = m_StateService.GetUniqueCommandKey();
      barrierCommand.m_Object.Key = commandListKey;
      barrierCommand.m_NumBarrierGroups.Value = 1;

      D3D12_TEXTURE_BARRIER barrier{};
      barrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
      barrier.SyncAfter = D3D12_BARRIER_SYNC_NONE;
      barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
      barrier.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
      barrier.LayoutBefore = beforeLayout;
      barrier.LayoutAfter = afterLayout;
      barrier.Subresources.IndexOrFirstMipLevel = subresource;

      D3D12_BARRIER_GROUP barrierGroup{};
      barrierCommand.m_pBarrierGroups.Value = &barrierGroup;
      barrierCommand.m_pBarrierGroups.Size = 1;
      barrierCommand.m_pBarrierGroups.ResourceKeys.resize(1);
      barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
      barrierGroup.NumBarriers = 1;
      barrierGroup.pTextureBarriers = &barrier;

      barrierCommand.m_pBarrierGroups.ResourceKeys[0] = resourceKey;
      m_StateService.GetRecorder().Record(
          ID3D12GraphicsCommandList7BarrierSerializer(barrierCommand));
    };

    ResourceStates& resourceStates = GetResourceStates(resourceKey);

    if (resourceStates.AllEqual) {
      if (!resourceStates.SubresourceStates[0].Enhanced) {
        if (resourceStates.SubresourceStates[0].State != D3D12_RESOURCE_STATE_COPY_DEST) {
          writeResourceBarrier(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                               D3D12_RESOURCE_STATE_COPY_DEST,
                               resourceStates.SubresourceStates[0].State);
        }
      } else {
        if (!resourceStates.IsBuffer &&
            resourceStates.SubresourceStates[0].Layout != D3D12_BARRIER_LAYOUT_COPY_DEST &&
            resourceStates.SubresourceStates[0].Layout != D3D12_BARRIER_LAYOUT_UNDEFINED) {
          writeResourceEnhancedBarrier(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                       D3D12_BARRIER_LAYOUT_COPY_DEST,
                                       resourceStates.SubresourceStates[0].Layout);
        }
      }
    } else {
      for (unsigned i = 0; i < resourceStates.SubresourceStates.size(); ++i) {
        if (!resourceStates.SubresourceStates[i].Enhanced) {
          if (resourceStates.SubresourceStates[i].State != D3D12_RESOURCE_STATE_COPY_DEST) {
            writeResourceBarrier(i, D3D12_RESOURCE_STATE_COPY_DEST,
                                 resourceStates.SubresourceStates[i].State);
          }
        } else {
          if (!resourceStates.IsBuffer &&
              resourceStates.SubresourceStates[i].Layout != D3D12_BARRIER_LAYOUT_COPY_DEST &&
              resourceStates.SubresourceStates[i].Layout != D3D12_BARRIER_LAYOUT_UNDEFINED) {
            writeResourceEnhancedBarrier(i, D3D12_BARRIER_LAYOUT_COPY_DEST,
                                         resourceStates.SubresourceStates[i].Layout);
          }
        }
      }
    }
  }

  for (auto& it : m_AliasingBarriersOrdered) {
    AliasingBarrierKeys& keys = it.first;
    if (keys.first && !m_StateService.StateRestored(keys.first) ||
        keys.second && !m_StateService.StateRestored(keys.second)) {
      continue;
    }

    unsigned count = m_AliasingBarriersCounted[keys];
    if (it.second != count) {
      continue;
    }
    if (keys.first && m_ResourceStates.find(keys.first) == m_ResourceStates.end() ||
        keys.second && m_ResourceStates.find(keys.second) == m_ResourceStates.end()) {
      continue;
    }

    InsertIfNotResident(keys.first, residencyKeys);
    InsertIfNotResident(keys.second, residencyKeys);

    ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
    barrierCommand.Key = m_StateService.GetUniqueCommandKey();
    barrierCommand.m_Object.Key = commandListKey;
    barrierCommand.m_NumBarriers.Value = 1;
    D3D12_RESOURCE_BARRIER barrier{};
    barrierCommand.m_pBarriers.Value = &barrier;
    barrierCommand.m_pBarriers.Size = 1;
    barrierCommand.m_pBarriers.ResourceKeys.resize(1);
    barrierCommand.m_pBarriers.ResourceAfterKeys.resize(1);
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierCommand.m_pBarriers.ResourceKeys[0] = keys.first;
    barrierCommand.m_pBarriers.ResourceAfterKeys[0] = keys.second;
    m_StateService.GetRecorder().Record(
        ID3D12GraphicsCommandListResourceBarrierSerializer(barrierCommand));
  }

  ID3D12GraphicsCommandListCloseCommand commandListClose;
  commandListClose.Key = m_StateService.GetUniqueCommandKey();
  commandListClose.m_Object.Key = commandListKey;
  m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListCloseSerializer(commandListClose));

  RecordMakeResident(residencyKeys);

  ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
  executeCommandLists.Key = m_StateService.GetUniqueCommandKey();
  executeCommandLists.m_Object.Key = commandQueueKey;
  executeCommandLists.m_NumCommandLists.Value = 1;
  executeCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
  executeCommandLists.m_ppCommandLists.Size = 1;
  executeCommandLists.m_ppCommandLists.Keys.resize(1);
  executeCommandLists.m_ppCommandLists.Keys[0] = commandListKey;
  m_StateService.GetRecorder().Record(
      ID3D12CommandQueueExecuteCommandListsSerializer(executeCommandLists));

  ID3D12CommandQueueSignalCommand commandQueueSignal;
  commandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
  commandQueueSignal.m_Object.Key = commandQueueKey;
  commandQueueSignal.m_pFence.Key = fenceKey;
  commandQueueSignal.m_Value.Value = 1;
  m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(commandQueueSignal));

  ID3D12FenceGetCompletedValueCommand GetCompletedValue;
  GetCompletedValue.Key = m_StateService.GetUniqueCommandKey();
  GetCompletedValue.m_Object.Key = fenceKey;
  GetCompletedValue.m_Result.Value = 1;
  m_StateService.GetRecorder().Record(ID3D12FenceGetCompletedValueSerializer(GetCompletedValue));

  IUnknownReleaseCommand releaseFence;
  releaseFence.Key = m_StateService.GetUniqueCommandKey();
  releaseFence.m_Object.Key = fenceKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseFence));

  IUnknownReleaseCommand releaseCommandList;
  releaseCommandList.Key = m_StateService.GetUniqueCommandKey();
  releaseCommandList.m_Object.Key = commandListKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandList));

  IUnknownReleaseCommand releaseCommandAllocator;
  releaseCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
  releaseCommandAllocator.m_Object.Key = commandAllocatorKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandAllocator));

  IUnknownReleaseCommand releaseCommandQueue;
  releaseCommandQueue.Key = m_StateService.GetUniqueCommandKey();
  releaseCommandQueue.m_Object.Key = commandQueueKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandQueue));

  RecordEvict(residencyKeys);
}

void ResourceStateTrackingService::RestoreBackBufferState(unsigned commandQueueKey,
                                                          unsigned resourceKey,
                                                          D3D12_RESOURCE_STATES beforeState) {
  ResourceStates& resourceStates = GetResourceStates(resourceKey);
  D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_COMMON;
  if (!resourceStates.SubresourceStates[0].Enhanced) {
    afterState = resourceStates.SubresourceStates[0].State;
  } else {
    afterState = GetResourceState(resourceStates.SubresourceStates[0].Layout);
  }

  unsigned commandAllocatorKey = m_StateService.GetUniqueObjectKey();
  ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
  createCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
  createCommandAllocator.m_Object.Key = m_DeviceKey;
  createCommandAllocator.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  createCommandAllocator.m_riid.Value = IID_ID3D12CommandAllocator;
  createCommandAllocator.m_ppCommandAllocator.Key = commandAllocatorKey;
  m_StateService.GetRecorder().Record(
      ID3D12DeviceCreateCommandAllocatorSerializer(createCommandAllocator));

  unsigned commandListKey = m_StateService.GetUniqueObjectKey();
  ID3D12DeviceCreateCommandListCommand createCommandList;
  createCommandList.Key = m_StateService.GetUniqueCommandKey();
  createCommandList.m_Object.Key = m_DeviceKey;
  createCommandList.m_nodeMask.Value = 0;
  createCommandList.m_pCommandAllocator.Key = createCommandAllocator.m_ppCommandAllocator.Key;
  createCommandList.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  createCommandList.m_pInitialState.Value = nullptr;
  createCommandList.m_riid.Value = IID_ID3D12CommandList;
  createCommandList.m_ppCommandList.Key = commandListKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateCommandListSerializer(createCommandList));

  unsigned fenceKey = m_StateService.GetUniqueObjectKey();
  ID3D12DeviceCreateFenceCommand createFence;
  createFence.Key = m_StateService.GetUniqueCommandKey();
  createFence.m_Object.Key = m_DeviceKey;
  createFence.m_InitialValue.Value = 0;
  createFence.m_Flags.Value = D3D12_FENCE_FLAG_NONE;
  createFence.m_riid.Value = IID_ID3D12Fence;
  createFence.m_ppFence.Key = fenceKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateFenceSerializer(createFence));

  ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
  barrierCommand.Key = m_StateService.GetUniqueCommandKey();
  barrierCommand.m_Object.Key = commandListKey;
  barrierCommand.m_NumBarriers.Value = 1;
  D3D12_RESOURCE_BARRIER barrier{};
  barrierCommand.m_pBarriers.Value = &barrier;
  barrierCommand.m_pBarriers.Size = 1;
  barrierCommand.m_pBarriers.ResourceKeys.resize(1);
  barrierCommand.m_pBarriers.ResourceAfterKeys.resize(1);
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.Subresource = 0;
  barrier.Transition.StateBefore = beforeState;
  barrier.Transition.StateAfter = afterState;
  barrierCommand.m_pBarriers.ResourceKeys[0] = resourceKey;
  m_StateService.GetRecorder().Record(
      ID3D12GraphicsCommandListResourceBarrierSerializer(barrierCommand));

  ID3D12GraphicsCommandListCloseCommand commandListClose;
  commandListClose.Key = m_StateService.GetUniqueCommandKey();
  commandListClose.m_Object.Key = commandListKey;
  m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListCloseSerializer(commandListClose));

  ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
  executeCommandLists.Key = m_StateService.GetUniqueCommandKey();
  executeCommandLists.m_Object.Key = commandQueueKey;
  executeCommandLists.m_NumCommandLists.Value = 1;
  executeCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
  executeCommandLists.m_ppCommandLists.Size = 1;
  executeCommandLists.m_ppCommandLists.Keys.resize(1);
  executeCommandLists.m_ppCommandLists.Keys[0] = commandListKey;
  m_StateService.GetRecorder().Record(
      ID3D12CommandQueueExecuteCommandListsSerializer(executeCommandLists));

  ID3D12CommandQueueSignalCommand commandQueueSignal;
  commandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
  commandQueueSignal.m_Object.Key = commandQueueKey;
  commandQueueSignal.m_pFence.Key = fenceKey;
  commandQueueSignal.m_Value.Value = 1;
  m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(commandQueueSignal));

  ID3D12FenceGetCompletedValueCommand GetCompletedValue;
  GetCompletedValue.Key = m_StateService.GetUniqueCommandKey();
  GetCompletedValue.m_Object.Key = fenceKey;
  GetCompletedValue.m_Result.Value = 1;
  m_StateService.GetRecorder().Record(ID3D12FenceGetCompletedValueSerializer(GetCompletedValue));

  IUnknownReleaseCommand releaseFence;
  releaseFence.Key = m_StateService.GetUniqueCommandKey();
  releaseFence.m_Object.Key = fenceKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseFence));

  IUnknownReleaseCommand releaseCommandList;
  releaseCommandList.Key = m_StateService.GetUniqueCommandKey();
  releaseCommandList.m_Object.Key = commandListKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandList));

  IUnknownReleaseCommand releaseCommandAllocator;
  releaseCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
  releaseCommandAllocator.m_Object.Key = commandAllocatorKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandAllocator));
}

void ResourceStateTrackingService::InsertIfNotResident(unsigned resourceKey,
                                                       std::set<unsigned>& residencyKeys) {
  if (!resourceKey) {
    return;
  }

  auto residencyKey = GetResidencyKeyForNotResidentResource(resourceKey);
  if (residencyKey.has_value() && residencyKey.value() != 0) {
    residencyKeys.insert(residencyKey.value());
  }
}

std::optional<unsigned> ResourceStateTrackingService::GetResidencyKeyForNotResidentResource(
    unsigned key) {
  ObjectState* state = m_StateService.GetState(key);
  if (!state) {
    return std::nullopt;
  }

  switch (state->CreationCommand->GetId()) {
  case CommandId::ID_ID3D12DEVICE_CREATECOMMITTEDRESOURCE: {
    auto* command =
        static_cast<ID3D12DeviceCreateCommittedResourceCommand*>(state->CreationCommand.get());
    if (command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      return key;
    }
  } break;
  case CommandId::ID_ID3D12DEVICE4_CREATECOMMITTEDRESOURCE1: {
    auto* command =
        static_cast<ID3D12Device4CreateCommittedResource1Command*>(state->CreationCommand.get());
    if (command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      return key;
    }
  } break;
  case CommandId::ID_ID3D12DEVICE8_CREATECOMMITTEDRESOURCE2: {
    auto* command =
        static_cast<ID3D12Device8CreateCommittedResource2Command*>(state->CreationCommand.get());
    if (command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      return key;
    }
  } break;
  case CommandId::ID_ID3D12DEVICE10_CREATECOMMITTEDRESOURCE3: {
    auto* command =
        static_cast<ID3D12Device10CreateCommittedResource3Command*>(state->CreationCommand.get());
    if (command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      return key;
    }
  } break;
  case CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE: {
    auto* command =
        static_cast<INTC_D3D12_CreateCommittedResourceCommand*>(state->CreationCommand.get());
    if (command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      return key;
    }
  } break;
  case CommandId::ID_ID3D12DEVICE_CREATEPLACEDRESOURCE:
  case CommandId::ID_ID3D12DEVICE8_CREATEPLACEDRESOURCE1:
  case CommandId::ID_ID3D12DEVICE10_CREATEPLACEDRESOURCE2:
  case CommandId::INTC_D3D12_CREATEPLACEDRESOURCE: {
    unsigned heapKey = static_cast<ResourceState*>(state)->HeapKey;
    ObjectState* heapState = m_StateService.GetState(heapKey);
    if (!heapState) {
      return std::nullopt;
    }
    switch (heapState->CreationCommand->GetId()) {
    case CommandId::ID_ID3D12DEVICE_CREATEHEAP: {
      auto* command = static_cast<ID3D12DeviceCreateHeapCommand*>(heapState->CreationCommand.get());
      if (command->m_pDesc.Value->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
        return heapKey;
      }
    } break;
    case CommandId::ID_ID3D12DEVICE4_CREATEHEAP1: {
      auto* command =
          static_cast<ID3D12Device4CreateHeap1Command*>(heapState->CreationCommand.get());
      if (command->m_pDesc.Value->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
        return heapKey;
      }
    } break;
    case CommandId::INTC_D3D12_CREATEHEAP: {
      auto* command = static_cast<INTC_D3D12_CreateHeapCommand*>(heapState->CreationCommand.get());
      if (command->m_pDesc.Value->pD3D12Desc->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
        return heapKey;
      }
    } break;
    default:
      return std::nullopt;
    }
  } break;
  }
  return std::nullopt;
}

void ResourceStateTrackingService::RecordMakeResident(const std::set<unsigned>& keys) {
  if (keys.empty()) {
    return;
  }

  ID3D12DeviceMakeResidentCommand makeResident;
  makeResident.Key = m_StateService.GetUniqueCommandKey();
  makeResident.m_Object.Key = m_DeviceKey;
  makeResident.m_NumObjects.Value = keys.size();
  ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
  makeResident.m_ppObjects.Value = &fakePtr;
  makeResident.m_ppObjects.Size = keys.size();
  for (unsigned key : keys) {
    makeResident.m_ppObjects.Keys.push_back(key);
  }
  m_StateService.GetRecorder().Record(ID3D12DeviceMakeResidentSerializer(makeResident));
}

void ResourceStateTrackingService::RecordEvict(const std::set<unsigned>& keys) {
  if (keys.empty()) {
    return;
  }

  ID3D12DeviceEvictCommand evict;
  evict.Key = m_StateService.GetUniqueCommandKey();
  evict.m_Object.Key = m_DeviceKey;
  evict.m_NumObjects.Value = keys.size();
  ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
  evict.m_ppObjects.Value = &fakePtr;
  evict.m_ppObjects.Size = keys.size();
  for (unsigned key : keys) {
    evict.m_ppObjects.Keys.push_back(key);
  }
  m_StateService.GetRecorder().Record(ID3D12DeviceEvictSerializer(evict));
}

} // namespace DirectX
} // namespace gits
