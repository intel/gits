// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "commandsAuto.h"

#include <vector>
#include <unordered_map>
#include <memory>
#include <unordered_set>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class StateTrackingService;
class ResourceStateTrackingService;

class ReservedResourcesService {
public:
  struct Tile {
    ObjectKey HeapKey{};
    unsigned HeapOffset{};
    unsigned SubresourceIndex{};
    bool Packed{};
  };
  struct TiledResource {
    ID3D12Resource* Resource{};
    D3D12_RESOURCE_DESC Desc{};
    ObjectKey ResourceKey{};
    D3D12_PACKED_MIP_INFO PackedMipInfo{};
    std::vector<D3D12_SUBRESOURCE_TILING> Subresources;
    std::vector<Tile> Tiles;
    std::unordered_map<unsigned, unsigned> PackedSubresourcesStartTiles;
    unsigned UpdateId{};
    bool Destroyed{};
  };

  struct TileRegion {
    D3D12_TILED_RESOURCE_COORDINATE Coord;
    D3D12_TILE_REGION_SIZE Size;
    bool Packed{};
  };
  using TileRegionsBySubresource = std::unordered_map<unsigned, std::vector<TileRegion>>;

public:
  ReservedResourcesService(StateTrackingService& stateService) : m_StateService(stateService) {}
  void AddUpdateTileMappings(ID3D12CommandQueueUpdateTileMappingsCommand& c);
  void DestroyObject(ObjectKey objectKey);
  void UpdateTileMappings(TiledResource& tiledResource,
                          ObjectKey commandQueueKey,
                          TileRegionsBySubresource* tileRegions);
  TiledResource* GetTiledResource(ObjectKey resourceKey);
  void RestoreContent(const std::vector<ObjectKey>& resourceKeys);
  void CleanupRestore();

private:
  std::unordered_map<ObjectKey, std::unique_ptr<TiledResource>> m_Resources;
  std::unordered_map<ObjectKey, std::unordered_set<ObjectKey>> m_ResourcesByHeapKey;

private:
  void InitRestore();
  void GetSubresourceSizes(
      ID3D12Device* device,
      D3D12_RESOURCE_DESC& desc,
      std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>>& sizes);
  void InitTiledResource(TiledResource& tiledResource);
  void CopySourceBarrier(ID3D12Resource* resource, ObjectKey resourceKey, bool restoreState);
  void MarkSubresourceNotFullyMapped(const TiledResource& tiledResource,
                                     const Tile& tile,
                                     std::vector<bool>& subresourceFullyMappedFlags);

private:
  StateTrackingService& m_StateService;

  ID3D12Device* m_Device{};
  ID3D12CommandQueue* m_CommandQueue{};
  ID3D12CommandAllocator* m_CommandAllocator{};
  ID3D12GraphicsCommandList* m_CommandList{};
  ID3D12Fence* m_Fence{};
  UINT64 m_CurrentFenceValue{};
  ObjectKey m_CommandQueueKey{};
  ObjectKey m_CommandAllocatorKey{};
  ObjectKey m_CommandListKey{};
  ObjectKey m_FenceKey{};
  ObjectKey m_UploadResourceKey{};
  UINT64 m_RecordedFenceValue{};
  size_t m_UploadResourceSize{};
  bool m_ContentRestoreInitialized{};
};

} // namespace DirectX
} // namespace gits
