// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
    unsigned heapKey{};
    unsigned heapOffset{};
    unsigned subresourceIndex{};
    bool packed{};
  };
  struct TiledResource {
    ID3D12Resource* resource{};
    D3D12_RESOURCE_DESC desc{};
    unsigned resourceKey{};
    D3D12_PACKED_MIP_INFO packedMipInfo{};
    std::vector<D3D12_SUBRESOURCE_TILING> subresources;
    std::vector<Tile> tiles;
    std::unordered_map<unsigned, unsigned> packedSubresourcesStartTiles;
    unsigned updateId{};
    bool destroyed{};
  };

  struct TileRegion {
    D3D12_TILED_RESOURCE_COORDINATE coord;
    D3D12_TILE_REGION_SIZE size;
    bool packed;
  };
  using TileRegionsBySubresource = std::unordered_map<unsigned, std::vector<TileRegion>>;

public:
  ReservedResourcesService(StateTrackingService& stateService) : stateService_(stateService) {}
  void addUpdateTileMappings(ID3D12CommandQueueUpdateTileMappingsCommand& c);
  void destroyObject(unsigned objectKey);
  void updateTileMappings(TiledResource& tiledResource,
                          unsigned commandQueueKey,
                          TileRegionsBySubresource* tileRegions);
  TiledResource* getTiledResource(unsigned resourceKey);
  void restoreContent(const std::vector<unsigned>& resourceKeys);

private:
  std::unordered_map<unsigned, std::unique_ptr<TiledResource>> resources_;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> resourcesByHeapKey_;

private:
  void initRestore();
  void cleanupRestore();
  void getSubresourceSizes(
      ID3D12Device* device,
      D3D12_RESOURCE_DESC& desc,
      std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>>& sizes);
  void initTiledResource(TiledResource& tiledResource);

private:
  StateTrackingService& stateService_;

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
