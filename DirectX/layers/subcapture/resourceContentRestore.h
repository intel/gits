// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectState.h"

#include <vector>
#include <map>

namespace gits {
namespace DirectX {

class StateTrackingService;

class ResourceContentRestore {
public:
  ResourceContentRestore(StateTrackingService& stateService) : stateService_(stateService) {}

  template <typename D3D12ResourceState>
  bool isRaytracingResource(D3D12ResourceState* state) {
    return state->initialResourceState == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
  }

  bool isRaytracingResource(D3D12CommittedResource3State* state) {
    return false;
  }

  bool isRaytracingResource(D3D12PlacedResource2State* state) {
    return false;
  }

  template <typename D3D12CommittedResourceState>
  void addCommittedResourceState(D3D12CommittedResourceState* committedState) {

    if (isRaytracingResource(committedState)) {
      return;
    }
    if (committedState->desc.SampleDesc.Count > 1) {
      return;
    }

    ResourceState state{static_cast<ID3D12Resource*>(committedState->object), committedState->key,
                        0, committedState->deviceKey};
    if (committedState->isMappable) {
      mappableResourceStates_.push_back(state);
    } else if (committedState->desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
      unmappableResourceBuffers_.push_back(state);
    } else {
      unmappableResourceTextures_.push_back(state);
    }
  }

  template <typename D3D12PlacedResourceState>
  void addPlacedResourceState(D3D12PlacedResourceState* placedState) {

    if (isRaytracingResource(placedState)) {
      return;
    }
    if (placedState->desc.SampleDesc.Count > 1) {
      return;
    }

    ResourceState state{static_cast<ID3D12Resource*>(placedState->object), placedState->key,
                        placedState->heapKey, placedState->deviceKey};
    if (placedState->isMappable) {
      mappableResourceStates_.push_back(state);
    } else if (placedState->desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
      unmappableResourceBuffers_.push_back(state);
    } else {
      unmappableResourceTextures_.push_back(state);
    }
  }

  void restoreContent();

private:
  struct ResourceState {
    ID3D12Resource* resource{};
    unsigned key{};
    unsigned heapKey{};
    unsigned deviceKey{};
  };

private:
  void restoreMappableResources();
  unsigned restoreUnmappableResources(std::vector<ResourceState>& unmappableResourceStates,
                                      unsigned resourceStartIndex);
  void getSubresourceSizes(
      ID3D12Device* device,
      D3D12_RESOURCE_DESC& desc,
      std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>>& sizes);
  void initRestoreUnmappableResources();
  void cleanupRestoreUnmappableResources();
  UINT64 getAlignedSize(UINT64 size);

private:
  StateTrackingService& stateService_;
  std::vector<ResourceState> mappableResourceStates_;
  std::vector<ResourceState> unmappableResourceBuffers_;
  std::vector<ResourceState> unmappableResourceTextures_;

  ID3D12Device* device_{};
  ID3D12CommandQueue* commandQueue_{};
  ID3D12CommandAllocator* commandAllocator_{};
  ID3D12GraphicsCommandList* commandList_{};
  ID3D12Fence* fence_{};
  UINT64 currentFenceValue_{};
  unsigned commandQueueKey_{};
  unsigned commandAllocatorKey_{};
  unsigned commandListKey_{};
  unsigned fenceKey_{};
  UINT64 recordedFenceValue_{};
};

} // namespace DirectX
} // namespace gits
