// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceStateTrackingService.h"
#include "stateTrackingService.h"
#include "commandsAuto.h"
#include "commandWritersAuto.h"
#include "commandsCustom.h"
#include "commandWritersCustom.h"
#include "gits.h"
#include "log.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void ResourceStateTrackingService::resourceBarrier(unsigned commandListKey,
                                                   D3D12_RESOURCE_BARRIER* barriers,
                                                   std::vector<unsigned>& resourceKeys,
                                                   std::vector<unsigned>& resourceAfterKeys) {
  ResourceBarriers resourceBarriers;
  resourceBarriers.barriers.resize(resourceKeys.size());
  for (unsigned i = 0; i < resourceKeys.size(); ++i) {
    resourceBarriers.barriers[i] = barriers[i];
  }
  resourceBarriers.resourceKeys = resourceKeys;
  resourceBarriers.resourceAfterKeys = resourceAfterKeys;
  barriersByCommandList_[commandListKey].push_back(std::move(resourceBarriers));
}

void ResourceStateTrackingService::executeCommandLists(std::vector<unsigned>& commandListKeys) {
  for (unsigned key : commandListKeys) {
    auto it = barriersByCommandList_.find(key);
    if (it != barriersByCommandList_.end()) {
      for (ResourceBarriers& barriers : it->second) {
        resourceBarrier(barriers.barriers, barriers.resourceKeys, barriers.resourceAfterKeys);
      }
      barriersByCommandList_.erase(it);
    }
  }
}

unsigned ResourceStateTrackingService::getSubresourcesCount(ID3D12Resource* resource) {
  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  unsigned planes = 1;
  if (desc.Format != DXGI_FORMAT_UNKNOWN) {
    auto it = planesByFormat_.find(desc.Format);
    if (it == planesByFormat_.end()) {
      Microsoft::WRL::ComPtr<ID3D12Device> device;
      HRESULT hr = resource->GetDevice(IID_PPV_ARGS(&device));
      GITS_ASSERT(hr == S_OK);
      D3D12_FEATURE_DATA_FORMAT_INFO formatInfo = {desc.Format, 0};
      if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo,
                                                sizeof(formatInfo)))) {
        planes = formatInfo.PlaneCount;
        planesByFormat_[desc.Format] = planes;
      }
    } else {
      planes = it->second;
    }
  }
  unsigned subresources = desc.MipLevels * planes;
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
      desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D) {
    subresources *= desc.DepthOrArraySize;
  }
  return subresources;
}

void ResourceStateTrackingService::resourceBarrier(std::vector<D3D12_RESOURCE_BARRIER>& barriers,
                                                   std::vector<unsigned>& resourceKeys,
                                                   std::vector<unsigned>& resourceAfterKeys) {
  for (unsigned i = 0; i < barriers.size(); ++i) {
    if (barriers[i].Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION) {
      D3D12_RESOURCE_STATES stateAfter = barriers[i].Transition.StateAfter;
      ResourceStates& states = getResourceStates(resourceKeys[i]);
      unsigned subresource = barriers[i].Transition.Subresource;
      if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
        states.allEqual = true;
        for (unsigned i = 0; i < states.subresourceStates.size(); ++i) {
          states.subresourceStates[i] = stateAfter;
        }
      } else {
        states.allEqual = false;
        states.subresourceStates[subresource] = stateAfter;
      }
    } else if (barriers[i].Type == D3D12_RESOURCE_BARRIER_TYPE_ALIASING) {
      auto keys = std::make_pair(resourceKeys[i], resourceAfterKeys[i]);
      unsigned& count = aliasingBarriersCounted_[keys];
      ++count;
      aliasingBarriersOrdered_.push_back(std::make_pair(keys, count));
    }
  }
}

void ResourceStateTrackingService::addResource(unsigned deviceKey,
                                               ID3D12Resource* resource,
                                               unsigned resourceKey,
                                               D3D12_RESOURCE_STATES initialState,
                                               bool recreateState) {
  if (deviceKey) {
    deviceKey_ = deviceKey;
  }
  ResourceStates& states = resourceStates_[resourceKey];
  states.subresourceStates.resize(getSubresourcesCount(resource));
  for (unsigned i = 0; i < states.subresourceStates.size(); ++i) {
    states.subresourceStates[i] = initialState;
  }
  if (recreateState) {
    recreateStateResources_.insert(resourceKey);
  }
}

void ResourceStateTrackingService::addResource(unsigned deviceKey,
                                               ID3D12Resource* resource,
                                               unsigned resourceKey,
                                               D3D12_BARRIER_LAYOUT initialState,
                                               bool recreateState) {
  D3D12_RESOURCE_STATES state = getResourceState(initialState);
  addResource(deviceKey, resource, resourceKey, state, recreateState);
}

void ResourceStateTrackingService::addBackBufferResource(ID3D12Resource* resource,
                                                         unsigned resourceKey,
                                                         unsigned buffer) {
  addResource(0, resource, resourceKey, D3D12_RESOURCE_STATE_COMMON, false);
  backBuffers_[buffer] = resourceKey;
}

D3D12_RESOURCE_STATES ResourceStateTrackingService::getResourceState(D3D12_BARRIER_LAYOUT layout) {

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
    LOG_ERROR << "Barrier layout not handled " << layout << "!";
  }

  return state;
}

D3D12_BARRIER_LAYOUT ResourceStateTrackingService::getResourceLayout(D3D12_RESOURCE_STATES state) {
  D3D12_BARRIER_LAYOUT layout{};

  switch (state) {
  case D3D12_RESOURCE_STATE_COMMON:
    break;
  case D3D12_RESOURCE_STATE_GENERIC_READ:
    layout = D3D12_BARRIER_LAYOUT_GENERIC_READ;
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
  case D3D12_RESOURCE_STATE_COPY_SOURCE:
    layout = D3D12_BARRIER_LAYOUT_COPY_SOURCE;
    break;
  case D3D12_RESOURCE_STATE_COPY_DEST:
    layout = D3D12_BARRIER_LAYOUT_COPY_DEST;
    break;
  case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:
    layout = D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE;
    break;
  case D3D12_RESOURCE_STATE_RESOLVE_DEST:
    layout = D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
    break;
  case D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE:
    layout = D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE;
    break;
  default:
    LOG_ERROR << "Barrier state not handled " << state << "!";
  }

  return layout;
}

void ResourceStateTrackingService::destroyResource(unsigned resourceKey) {
  recreateStateResources_.erase(resourceKey);
}

ResourceStateTrackingService::ResourceStates& ResourceStateTrackingService::getResourceStates(
    unsigned resourceKey) {
  auto it = resourceStates_.find(resourceKey);
  GITS_ASSERT(it != resourceStates_.end());
  return it->second;
}

D3D12_RESOURCE_STATES ResourceStateTrackingService::getResourceState(unsigned resourceKey) {
  ResourceStates& states = getResourceStates(resourceKey);
  return states.subresourceStates[0];
}

D3D12_RESOURCE_STATES ResourceStateTrackingService::getSubresourceState(unsigned resourceKey,
                                                                        unsigned subresource) {
  ResourceStates& states = getResourceStates(resourceKey);
  return states.subresourceStates[subresource];
}

D3D12_BARRIER_LAYOUT ResourceStateTrackingService::getResourceLayout(unsigned resourceKey) {
  D3D12_RESOURCE_STATES state = getResourceState(resourceKey);
  return getResourceLayout(state);
}

void ResourceStateTrackingService::restoreResourceStates(
    const std::vector<unsigned>& orderedResources) {
  if (orderedResources.empty() && aliasingBarriersOrdered_.empty()) {
    return;
  }

  unsigned commandQueueKey = stateService_.getUniqueObjectKey();
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  ID3D12DeviceCreateCommandQueueCommand createCommandQueue;
  createCommandQueue.key = stateService_.getUniqueCommandKey();
  createCommandQueue.object_.key = deviceKey_;
  createCommandQueue.pDesc_.value = &commandQueueDesc;
  createCommandQueue.riid_.value = IID_ID3D12CommandQueue;
  createCommandQueue.ppCommandQueue_.key = commandQueueKey;
  stateService_.getRecorder().record(new ID3D12DeviceCreateCommandQueueWriter(createCommandQueue));

  unsigned commandAllocatorKey = stateService_.getUniqueObjectKey();
  ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
  createCommandAllocator.key = stateService_.getUniqueCommandKey();
  createCommandAllocator.object_.key = deviceKey_;
  createCommandAllocator.type_.value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  createCommandAllocator.riid_.value = IID_ID3D12CommandAllocator;
  createCommandAllocator.ppCommandAllocator_.key = commandAllocatorKey;
  stateService_.getRecorder().record(
      new ID3D12DeviceCreateCommandAllocatorWriter(createCommandAllocator));

  unsigned commandListKey = stateService_.getUniqueObjectKey();
  ID3D12DeviceCreateCommandListCommand createCommandList;
  createCommandList.key = stateService_.getUniqueCommandKey();
  createCommandList.object_.key = deviceKey_;
  createCommandList.nodeMask_.value = 0;
  createCommandList.pCommandAllocator_.key = createCommandAllocator.ppCommandAllocator_.key;
  createCommandList.type_.value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  createCommandList.pInitialState_.value = nullptr;
  createCommandList.riid_.value = IID_ID3D12CommandList;
  createCommandList.ppCommandList_.key = commandListKey;
  stateService_.getRecorder().record(new ID3D12DeviceCreateCommandListWriter(createCommandList));

  unsigned fenceKey = stateService_.getUniqueObjectKey();
  ID3D12DeviceCreateFenceCommand createFence;
  createFence.key = stateService_.getUniqueCommandKey();
  createFence.object_.key = deviceKey_;
  createFence.InitialValue_.value = 0;
  createFence.Flags_.value = D3D12_FENCE_FLAG_NONE;
  createFence.riid_.value = IID_ID3D12Fence;
  createFence.ppFence_.key = fenceKey;
  stateService_.getRecorder().record(new ID3D12DeviceCreateFenceWriter(createFence));

  for (unsigned resourceKey : orderedResources) {
    if (recreateStateResources_.find(resourceKey) == recreateStateResources_.end()) {
      continue;
    }
    if (!stateService_.stateRestored(resourceKey)) {
      continue;
    }

    auto writeResourceBarrier = [&](unsigned subresource, D3D12_RESOURCE_STATES beforeState,
                                    D3D12_RESOURCE_STATES afterState) {
      ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
      barrierCommand.key = stateService_.getUniqueCommandKey();
      barrierCommand.object_.key = commandListKey;
      barrierCommand.NumBarriers_.value = 1;
      D3D12_RESOURCE_BARRIER barrier{};
      barrierCommand.pBarriers_.value = &barrier;
      barrierCommand.pBarriers_.size = 1;
      barrierCommand.pBarriers_.resourceKeys.resize(1);
      barrierCommand.pBarriers_.resourceAfterKeys.resize(1);
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      barrier.Transition.Subresource = subresource;
      barrier.Transition.StateBefore = beforeState;
      barrier.Transition.StateAfter = afterState;
      barrierCommand.pBarriers_.resourceKeys[0] = resourceKey;
      stateService_.getRecorder().record(
          new ID3D12GraphicsCommandListResourceBarrierWriter(barrierCommand));
    };
    ResourceStates& resourceStates = getResourceStates(resourceKey);
    if (resourceStates.allEqual) {
      if (resourceStates.subresourceStates[0] != D3D12_RESOURCE_STATE_COPY_DEST) {
        writeResourceBarrier(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                             D3D12_RESOURCE_STATE_COPY_DEST, resourceStates.subresourceStates[0]);
      }
    } else {
      for (unsigned i = 0; i < resourceStates.subresourceStates.size(); ++i) {
        if (resourceStates.subresourceStates[i] != D3D12_RESOURCE_STATE_COPY_DEST) {
          writeResourceBarrier(i, D3D12_RESOURCE_STATE_COPY_DEST,
                               resourceStates.subresourceStates[i]);
        }
      }
    }
  }

  for (auto& it : aliasingBarriersOrdered_) {
    AliasingBarrierKeys& keys = it.first;
    if (keys.first && !stateService_.stateRestored(keys.first) ||
        keys.second && !stateService_.stateRestored(keys.second)) {
      continue;
    }

    unsigned count = aliasingBarriersCounted_[keys];
    if (it.second != count) {
      continue;
    }
    if (keys.first && resourceStates_.find(keys.first) == resourceStates_.end() ||
        keys.second && resourceStates_.find(keys.second) == resourceStates_.end()) {
      continue;
    }
    ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
    barrierCommand.key = stateService_.getUniqueCommandKey();
    barrierCommand.object_.key = commandListKey;
    barrierCommand.NumBarriers_.value = 1;
    D3D12_RESOURCE_BARRIER barrier{};
    barrierCommand.pBarriers_.value = &barrier;
    barrierCommand.pBarriers_.size = 1;
    barrierCommand.pBarriers_.resourceKeys.resize(1);
    barrierCommand.pBarriers_.resourceAfterKeys.resize(1);
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierCommand.pBarriers_.resourceKeys[0] = keys.first;
    barrierCommand.pBarriers_.resourceAfterKeys[0] = keys.second;
    stateService_.getRecorder().record(
        new ID3D12GraphicsCommandListResourceBarrierWriter(barrierCommand));
  }

  ID3D12GraphicsCommandListCloseCommand commandListClose;
  commandListClose.key = stateService_.getUniqueCommandKey();
  commandListClose.object_.key = commandListKey;
  stateService_.getRecorder().record(new ID3D12GraphicsCommandListCloseWriter(commandListClose));

  ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
  executeCommandLists.key = stateService_.getUniqueCommandKey();
  executeCommandLists.object_.key = commandQueueKey;
  executeCommandLists.NumCommandLists_.value = 1;
  executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
  executeCommandLists.ppCommandLists_.size = 1;
  executeCommandLists.ppCommandLists_.keys.resize(1);
  executeCommandLists.ppCommandLists_.keys[0] = commandListKey;
  stateService_.getRecorder().record(
      new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

  ID3D12CommandQueueSignalCommand commandQueueSignal;
  commandQueueSignal.key = stateService_.getUniqueCommandKey();
  commandQueueSignal.object_.key = commandQueueKey;
  commandQueueSignal.pFence_.key = fenceKey;
  commandQueueSignal.Value_.value = 1;
  stateService_.getRecorder().record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

  ID3D12FenceGetCompletedValueCommand getCompletedValue;
  getCompletedValue.key = stateService_.getUniqueCommandKey();
  getCompletedValue.object_.key = fenceKey;
  getCompletedValue.result_.value = 1;
  stateService_.getRecorder().record(new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

  IUnknownReleaseCommand releaseFence;
  releaseFence.key = stateService_.getUniqueCommandKey();
  releaseFence.object_.key = fenceKey;
  stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseFence));

  IUnknownReleaseCommand releaseCommandList;
  releaseCommandList.key = stateService_.getUniqueCommandKey();
  releaseCommandList.object_.key = commandListKey;
  stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseCommandList));

  IUnknownReleaseCommand releaseCommandAllocator;
  releaseCommandAllocator.key = stateService_.getUniqueCommandKey();
  releaseCommandAllocator.object_.key = commandAllocatorKey;
  stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseCommandAllocator));

  IUnknownReleaseCommand releaseCommandQueue;
  releaseCommandQueue.key = stateService_.getUniqueCommandKey();
  releaseCommandQueue.object_.key = commandQueueKey;
  stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseCommandQueue));
}

void ResourceStateTrackingService::restoreBackBufferState(unsigned commandQueueKey,
                                                          unsigned buffer) {
  D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_COMMON;
  unsigned resourceKey = backBuffers_[buffer];
  if (!resourceKey) {
    return;
  }
  ResourceStates& resourceStates = getResourceStates(resourceKey);
  D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_COMMON;
  if (!resourceStates.subresourceStates.empty()) {
    afterState = resourceStates.subresourceStates[0];
  }
  if (afterState == D3D12_RESOURCE_STATE_COMMON) {
    return;
  }

  unsigned commandAllocatorKey = stateService_.getUniqueObjectKey();
  ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
  createCommandAllocator.key = stateService_.getUniqueCommandKey();
  createCommandAllocator.object_.key = deviceKey_;
  createCommandAllocator.type_.value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  createCommandAllocator.riid_.value = IID_ID3D12CommandAllocator;
  createCommandAllocator.ppCommandAllocator_.key = commandAllocatorKey;
  stateService_.getRecorder().record(
      new ID3D12DeviceCreateCommandAllocatorWriter(createCommandAllocator));

  unsigned commandListKey = stateService_.getUniqueObjectKey();
  ID3D12DeviceCreateCommandListCommand createCommandList;
  createCommandList.key = stateService_.getUniqueCommandKey();
  createCommandList.object_.key = deviceKey_;
  createCommandList.nodeMask_.value = 0;
  createCommandList.pCommandAllocator_.key = createCommandAllocator.ppCommandAllocator_.key;
  createCommandList.type_.value = D3D12_COMMAND_LIST_TYPE_DIRECT;
  createCommandList.pInitialState_.value = nullptr;
  createCommandList.riid_.value = IID_ID3D12CommandList;
  createCommandList.ppCommandList_.key = commandListKey;
  stateService_.getRecorder().record(new ID3D12DeviceCreateCommandListWriter(createCommandList));

  unsigned fenceKey = stateService_.getUniqueObjectKey();
  ID3D12DeviceCreateFenceCommand createFence;
  createFence.key = stateService_.getUniqueCommandKey();
  createFence.object_.key = deviceKey_;
  createFence.InitialValue_.value = 0;
  createFence.Flags_.value = D3D12_FENCE_FLAG_NONE;
  createFence.riid_.value = IID_ID3D12Fence;
  createFence.ppFence_.key = fenceKey;
  stateService_.getRecorder().record(new ID3D12DeviceCreateFenceWriter(createFence));

  ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
  barrierCommand.key = stateService_.getUniqueCommandKey();
  barrierCommand.object_.key = commandListKey;
  barrierCommand.NumBarriers_.value = 1;
  D3D12_RESOURCE_BARRIER barrier{};
  barrierCommand.pBarriers_.value = &barrier;
  barrierCommand.pBarriers_.size = 1;
  barrierCommand.pBarriers_.resourceKeys.resize(1);
  barrierCommand.pBarriers_.resourceAfterKeys.resize(1);
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.Subresource = 0;
  barrier.Transition.StateBefore = beforeState;
  barrier.Transition.StateAfter = afterState;
  barrierCommand.pBarriers_.resourceKeys[0] = resourceKey;
  stateService_.getRecorder().record(
      new ID3D12GraphicsCommandListResourceBarrierWriter(barrierCommand));

  ID3D12GraphicsCommandListCloseCommand commandListClose;
  commandListClose.key = stateService_.getUniqueCommandKey();
  commandListClose.object_.key = commandListKey;
  stateService_.getRecorder().record(new ID3D12GraphicsCommandListCloseWriter(commandListClose));

  ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
  executeCommandLists.key = stateService_.getUniqueCommandKey();
  executeCommandLists.object_.key = commandQueueKey;
  executeCommandLists.NumCommandLists_.value = 1;
  executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
  executeCommandLists.ppCommandLists_.size = 1;
  executeCommandLists.ppCommandLists_.keys.resize(1);
  executeCommandLists.ppCommandLists_.keys[0] = commandListKey;
  stateService_.getRecorder().record(
      new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

  ID3D12CommandQueueSignalCommand commandQueueSignal;
  commandQueueSignal.key = stateService_.getUniqueCommandKey();
  commandQueueSignal.object_.key = commandQueueKey;
  commandQueueSignal.pFence_.key = fenceKey;
  commandQueueSignal.Value_.value = 1;
  stateService_.getRecorder().record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

  ID3D12FenceGetCompletedValueCommand getCompletedValue;
  getCompletedValue.key = stateService_.getUniqueCommandKey();
  getCompletedValue.object_.key = fenceKey;
  getCompletedValue.result_.value = 1;
  stateService_.getRecorder().record(new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

  IUnknownReleaseCommand releaseFence;
  releaseFence.key = stateService_.getUniqueCommandKey();
  releaseFence.object_.key = fenceKey;
  stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseFence));

  IUnknownReleaseCommand releaseCommandList;
  releaseCommandList.key = stateService_.getUniqueCommandKey();
  releaseCommandList.object_.key = commandListKey;
  stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseCommandList));

  IUnknownReleaseCommand releaseCommandAllocator;
  releaseCommandAllocator.key = stateService_.getUniqueCommandKey();
  releaseCommandAllocator.object_.key = commandAllocatorKey;
  stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseCommandAllocator));
}

} // namespace DirectX
} // namespace gits
