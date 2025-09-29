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
#include <unordered_map>

namespace gits {
namespace DirectX {

class StateTrackingService;
class ResourceStateTrackingService;

class ResourceContentRestore {
public:
  ResourceContentRestore(StateTrackingService& stateService) : stateService_(stateService) {}
  void addCommittedResourceState(ResourceState* resourceState);
  void addPlacedResourceState(ResourceState* resourceState);
  void restoreContent(const std::vector<unsigned>& resourceKeys);
  void cleanupRestoreUnmappableResources();

private:
  struct ResourceInfo {
    ID3D12Resource* resource{};
    unsigned key{};
    unsigned heapKey{};
    unsigned deviceKey{};
  };

private:
  void restoreMappableResources(const std::vector<ResourceInfo>& resourceInfos);
  unsigned restoreUnmappableResources(std::vector<ResourceInfo>& unmappableResourceStates,
                                      unsigned resourceStartIndex,
                                      UINT64 maxBatchSize);
  void getSubresourceSizes(
      ID3D12Device* device,
      D3D12_RESOURCE_DESC& desc,
      std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>>& sizes);
  void initRestoreUnmappableResources();
  UINT64 getAlignedSize(UINT64 size);
  bool isBarrierRestricted(unsigned resourceKey);
  ID3D12Resource* createAuxiliaryPlacedResource(unsigned primaryResourceKey);
  unsigned createSubcaptureAuxiliaryPlacedResource(unsigned primaryResourceKey);

private:
  static constexpr size_t texturesMaxBatchSize_{0x1000};
  static constexpr size_t buffersMaxBatchSize_{0x100000};

  StateTrackingService& stateService_;
  std::unordered_map<unsigned, ResourceInfo> mappableResourceStates_;
  std::unordered_map<unsigned, ResourceInfo> unmappableResourceBuffers_;
  std::unordered_map<unsigned, ResourceInfo> unmappableResourceTextures_;

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
  unsigned uploadResourceKey_{};
  UINT64 recordedFenceValue_{};
  size_t uploadResourceSize_{};
  bool restoreUnmappableResourcesInitialized_{};
};

} // namespace DirectX
} // namespace gits
