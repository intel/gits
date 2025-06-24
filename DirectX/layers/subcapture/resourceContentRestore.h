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
  void addCommittedResourceState(ResourceState* resourceState);
  void addPlacedResourceState(ResourceState* resourceState);
  void restoreContent();

private:
  struct ResourceInfo {
    ID3D12Resource* resource{};
    unsigned key{};
    unsigned heapKey{};
    unsigned deviceKey{};
  };

private:
  void restoreMappableResources();
  unsigned restoreUnmappableResources(std::vector<ResourceInfo>& unmappableResourceStates,
                                      unsigned resourceStartIndex);
  void getSubresourceSizes(
      ID3D12Device* device,
      D3D12_RESOURCE_DESC& desc,
      std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>>& sizes);
  void initRestoreUnmappableResources();
  void cleanupRestoreUnmappableResources();
  UINT64 getAlignedSize(UINT64 size);
  bool isBarrierRestricted(unsigned resourceKey);
  ID3D12Resource* createAuxiliaryPlacedResource(unsigned primaryResourceKey);
  unsigned createSubcaptureAuxiliaryPlacedResource(unsigned primaryResourceKey);

private:
  StateTrackingService& stateService_;
  std::vector<ResourceInfo> mappableResourceStates_;
  std::vector<ResourceInfo> unmappableResourceBuffers_;
  std::vector<ResourceInfo> unmappableResourceTextures_;

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
