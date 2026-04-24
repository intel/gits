// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

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
    unsigned HeapKey{};
    unsigned HeapOffset{};
    unsigned SubresourceIndex{};
    bool Packed{};
  };
  struct TiledResource {
    ID3D12Resource* Resource{};
    D3D12_RESOURCE_DESC Desc{};
    unsigned ResourceKey{};
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
    bool Packed;
  };
  using TileRegionsBySubresource = std::unordered_map<unsigned, std::vector<TileRegion>>;

public:
  ReservedResourcesService(StateTrackingService& stateService) : m_StateService(stateService) {}
  void AddUpdateTileMappings(ID3D12CommandQueueUpdateTileMappingsCommand& c);
  void DestroyObject(unsigned objectKey);
  void UpdateTileMappings(TiledResource& tiledResource,
                          unsigned commandQueueKey,
                          TileRegionsBySubresource* tileRegions);
  TiledResource* GetTiledResource(unsigned ResourceKey);
  void RestoreContent(const std::vector<unsigned>& ResourceKeys);
  void CleanupRestore();

private:
  std::unordered_map<unsigned, std::unique_ptr<TiledResource>> m_Resources;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> m_ResourcesByHeapKey;

private:
  void InitRestore();
  void GetSubresourceSizes(
      ID3D12Device* device,
      D3D12_RESOURCE_DESC& desc,
      std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>>& sizes);
  void InitTiledResource(TiledResource& tiledResource);
  void CopySourceBarrier(ID3D12Resource* resource, unsigned ResourceKey, bool RestoreState);

private:
  StateTrackingService& m_StateService;

  ID3D12Device* m_Device{};
  ID3D12CommandQueue* m_CommandQueue{};
  ID3D12CommandAllocator* m_CommandAllocator{};
  ID3D12GraphicsCommandList* m_CommandList{};
  ID3D12Fence* m_Fence{};
  UINT64 m_CurrentFenceValue{};
  unsigned m_CommandQueueKey{};
  unsigned m_CommandAllocatorKey{};
  unsigned m_CommandListKey{};
  unsigned m_FenceKey{};
  unsigned m_UploadResourceKey{};
  UINT64 m_RecordedFenceValue{};
  size_t m_UploadResourceSize{};
  bool m_ContentRestoreInitialized{};
};

} // namespace DirectX
} // namespace gits
