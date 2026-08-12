// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "objectState.h"

#include <vector>
#include <map>
#include <unordered_map>
#include <set>

namespace gits {
namespace DirectX {

class StateTrackingService;
class ResourceStateTrackingService;

class ResourceContentRestore {
public:
  ResourceContentRestore(StateTrackingService& stateService) : m_StateService(stateService) {}
  void AddCommittedResourceState(ResourceState* resourceState);
  void AddPlacedResourceState(ResourceState* resourceState);
  void RestoreContent(const std::vector<GITSKey>& resourceKeys, bool backBuffer = false);
  void CleanupRestoreUnmappableResources();
  void RestoreBackBuffer(ID3D12CommandQueue* commandQueue,
                         GITSKey commandQueueKey,
                         GITSKey resourceKey,
                         ID3D12Resource* resource);

private:
  struct ResourceInfo {
    ID3D12Resource* Resource{};
    GITSKey Key{};
    GITSKey HeapKey{};
  };

private:
  void RestoreMappableResources(const std::vector<ResourceInfo>& resourceInfos);
  unsigned RestoreUnmappableResources(std::vector<ResourceInfo>& unmappableResourceStates,
                                      unsigned resourceStartIndex,
                                      UINT64 maxBatchSize);
  void GetSubresourceSizes(
      ID3D12Device* device,
      D3D12_RESOURCE_DESC& desc,
      std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>>& sizes);
  void InitRestoreUnmappableResources(bool backBuffer);
  UINT64 GetAlignedSize(UINT64 size);
  bool IsBarrierRestricted(GITSKey resourceKey);
  ID3D12Resource* CreateAuxiliaryPlacedResource(GITSKey primaryResourceKey);
  unsigned CreateSubcaptureAuxiliaryPlacedResource(GITSKey primaryResourceKey);
  void EvictPrevResidencyObjects();
  void CopySourceBarrier(ResourceInfo& state, bool RestoreState);

private:
  static constexpr size_t m_TexturesMaxBatchSize{0x1000};
  static constexpr size_t m_BuffersMaxBatchSize{0x100000};

  StateTrackingService& m_StateService;
  std::unordered_map<GITSKey, ResourceInfo> m_MappableResourceStates;
  std::unordered_map<GITSKey, ResourceInfo> m_UnmappableResourceBuffers;
  std::unordered_map<GITSKey, ResourceInfo> m_UnmappableResourceTextures;

  ID3D12Device* m_Device{};
  ID3D12CommandQueue* m_CommandQueue{};
  ID3D12CommandAllocator* m_CommandAllocator{};
  ID3D12GraphicsCommandList* m_CommandList{};
  ID3D12Fence* m_Fence{};
  UINT64 m_CurrentFenceValue{};
  GITSKey m_CommandQueueKey{};
  GITSKey m_CommandAllocatorKey{};
  GITSKey m_CommandListKey{};
  GITSKey m_FenceKey{};
  GITSKey m_UploadResourceKey{};
  UINT64 m_RecordedFenceValue{};
  size_t m_UploadResourceSize{};
  bool m_RestoreUnmappableResourcesInitialized{};
  std::set<GITSKey> m_PrevResidencyKeys;
  std::set<ID3D12Pageable*> m_PrevResidencyObjects;
};

} // namespace DirectX
} // namespace gits
