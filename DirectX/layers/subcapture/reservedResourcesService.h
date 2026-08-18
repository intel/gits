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
    GITSKey HeapKey{};
    unsigned HeapOffset{};
    unsigned SubresourceIndex{};
    bool Packed{};
  };
  struct TiledResource {
    ID3D12Resource* Resource{};
    D3D12_RESOURCE_DESC Desc{};
    GITSKey ResourceKey{};
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
  void DestroyObject(GITSKey objectKey);
  void UpdateTileMappings(TiledResource& tiledResource,
                          GITSKey commandQueueKey,
                          TileRegionsBySubresource* tileRegions);
  TiledResource* GetTiledResource(GITSKey resourceKey);
  void RestoreContent(const std::vector<GITSKey>& resourceKeys);
  void CleanupRestore();

private:
  std::unordered_map<GITSKey, std::unique_ptr<TiledResource>> m_Resources;
  std::unordered_map<GITSKey, std::unordered_set<GITSKey>> m_ResourcesByHeapKey;

private:
  void InitRestore();
  void GetSubresourceSizes(
      ID3D12Device* device,
      D3D12_RESOURCE_DESC& desc,
      std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>>& sizes);
  void InitTiledResource(TiledResource& tiledResource);
  void CopySourceBarrier(ID3D12Resource* resource, GITSKey resourceKey, bool restoreState);
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
  GITSKey m_CommandQueueKey{};
  GITSKey m_CommandAllocatorKey{};
  GITSKey m_CommandListKey{};
  GITSKey m_FenceKey{};
  GITSKey m_UploadResourceKey{};
  UINT64 m_RecordedFenceValue{};
  size_t m_UploadResourceSize{};
  bool m_ContentRestoreInitialized{};
};

} // namespace DirectX
} // namespace gits
