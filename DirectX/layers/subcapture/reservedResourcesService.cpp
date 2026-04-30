// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "reservedResourcesService.h"
#include "resourceStateTrackingService.h"
#include "stateTrackingService.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
#include "resourceSizeUtils.h"
#include "log.h"

#include <wrl/client.h>
#include <set>

namespace gits {
namespace DirectX {

void ReservedResourcesService::AddUpdateTileMappings(
    ID3D12CommandQueueUpdateTileMappingsCommand& c) {

  TiledResource* tiledResource{};
  auto it = m_Resources.find(c.m_pResource.Key);
  if (it != m_Resources.end()) {
    tiledResource = it->second.get();
  } else {
    tiledResource = new TiledResource();
    m_Resources[c.m_pResource.Key].reset(tiledResource);

    tiledResource->Resource = c.m_pResource.Value;
    tiledResource->Desc = c.m_pResource.Value->GetDesc();
    tiledResource->ResourceKey = c.m_pResource.Key;

    InitTiledResource(*tiledResource);
  }

  unsigned heapKey = c.m_pHeap.Key;

  if (heapKey) {
    m_ResourcesByHeapKey[heapKey].insert(c.m_pResource.Key);
  }

  unsigned heapRangeIndex = 0;
  unsigned heapOffset = heapKey && c.m_pHeapRangeStartOffsets.Value
                            ? c.m_pHeapRangeStartOffsets.Value[heapRangeIndex]
                            : 0;
  D3D12_TILE_RANGE_FLAGS heapRangeFlag = D3D12_TILE_RANGE_FLAG_NONE;
  if (c.m_pRangeFlags.Value) {
    heapRangeFlag = c.m_pRangeFlags.Value[heapRangeIndex];
  }
  unsigned heapRangeSize = tiledResource->Tiles.size();
  if (c.m_pRangeTileCounts.Value) {
    heapRangeSize = c.m_pRangeTileCounts.Value[heapRangeIndex];
  }
  unsigned tileIndexInHeapRange = 0;

  for (unsigned region = 0; region < c.m_NumResourceRegions.Value; ++region) {
    D3D12_TILED_RESOURCE_COORDINATE coord{};
    if (c.m_pResourceRegionStartCoordinates.Value) {
      coord = c.m_pResourceRegionStartCoordinates.Value[region];
    }
    D3D12_TILE_REGION_SIZE size{};
    if (c.m_pResourceRegionSizes.Value) {
      size = c.m_pResourceRegionSizes.Value[region];
    } else if (c.m_pResourceRegionStartCoordinates.Value) {
      size.NumTiles = 1;
    } else if (c.m_NumResourceRegions.Value == 1) {
      size.NumTiles = tiledResource->Tiles.size();
    }

    D3D12_SUBRESOURCE_TILING& subresource = tiledResource->Subresources[coord.Subresource];

    auto updateTile = [&](Tile& tile) {
      if (!heapKey) {
        tile.HeapKey = 0;
        tile.HeapOffset = 0;
        return;
      }

      if (tileIndexInHeapRange == heapRangeSize) {
        tileIndexInHeapRange = 0;
        ++heapRangeIndex;
        GITS_ASSERT(heapRangeIndex < c.m_NumRanges.Value);
        heapOffset =
            c.m_pHeapRangeStartOffsets.Value ? c.m_pHeapRangeStartOffsets.Value[heapRangeIndex] : 0;
        heapRangeFlag = D3D12_TILE_RANGE_FLAG_NONE;
        if (c.m_pRangeFlags.Value) {
          heapRangeFlag = c.m_pRangeFlags.Value[heapRangeIndex];
        }
        heapRangeSize = tiledResource->Tiles.size();
        if (c.m_pRangeTileCounts.Value) {
          heapRangeSize = c.m_pRangeTileCounts.Value[heapRangeIndex];
        }
      }

      if (heapRangeFlag == D3D12_TILE_RANGE_FLAG_NONE ||
          heapRangeFlag == D3D12_TILE_RANGE_FLAG_REUSE_SINGLE_TILE) {
        tile.HeapKey = heapKey;
        tile.HeapOffset = heapOffset;
      } else if (heapRangeFlag == D3D12_TILE_RANGE_FLAG_NULL) {
        tile.HeapKey = 0;
        tile.HeapOffset = 0;
      } else if (heapRangeFlag == D3D12_TILE_RANGE_FLAG_SKIP) {
        static bool logged = false;
        if (!logged) {
          LOG_ERROR << "D3D12_TILE_RANGE_FLAG_SKIP is not handled in subcapture!";
          logged = true;
        }
      }

      ++tileIndexInHeapRange;
      if (!(heapRangeFlag == D3D12_TILE_RANGE_FLAG_REUSE_SINGLE_TILE)) {
        ++heapOffset;
      }
    };

    if (size.UseBox) {
      for (unsigned z = 0; z < size.Depth; ++z) {
        for (unsigned y = 0; y < size.Height; ++y) {
          for (unsigned x = 0; x < size.Width; ++x) {
            unsigned tile = subresource.StartTileIndexInOverallResource +
                            (z + coord.Z) * (subresource.HeightInTiles * subresource.WidthInTiles) +
                            (y + coord.Y) * subresource.WidthInTiles + (x + coord.X);
            updateTile(tiledResource->Tiles[tile]);
          }
        }
      }
    } else {
      unsigned startTile = 0;
      if (subresource.StartTileIndexInOverallResource == D3D12_PACKED_TILE) {
        startTile = tiledResource->PackedSubresourcesStartTiles[coord.Subresource] + coord.X;
      } else {
        startTile = subresource.StartTileIndexInOverallResource +
                    coord.Z * (subresource.HeightInTiles * subresource.WidthInTiles) +
                    coord.Y * subresource.WidthInTiles + coord.X;
      }
      for (unsigned i = 0; i < size.NumTiles; ++i) {
        unsigned tile = startTile + i;
        updateTile(tiledResource->Tiles[tile]);
      }
    }
  }

  ++tiledResource->UpdateId;
}

void ReservedResourcesService::InitTiledResource(TiledResource& tiledResource) {
  HRESULT hr = tiledResource.Resource->GetDevice(IID_PPV_ARGS(&m_Device));
  GITS_ASSERT(hr == S_OK);

  unsigned planes = 1;
  D3D12_FEATURE_DATA_FORMAT_INFO formatInfo = {tiledResource.Desc.Format, 0};
  if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo,
                                              sizeof(formatInfo)))) {
    planes = formatInfo.PlaneCount;
  }
  unsigned subresources = tiledResource.Desc.MipLevels * planes;
  if (tiledResource.Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
      tiledResource.Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D) {
    subresources *= tiledResource.Desc.DepthOrArraySize;
  }

  unsigned tilesNum{};
  tiledResource.Subresources.resize(subresources);
  m_Device->GetResourceTiling(tiledResource.Resource, &tilesNum, &tiledResource.PackedMipInfo,
                              nullptr, &subresources, 0, tiledResource.Subresources.data());
  tiledResource.Tiles.resize(tilesNum);

  struct SubresourceRange {
    unsigned subresourceIndex;
    unsigned startTile;
    unsigned numTiles;
    bool packed;
  };
  std::vector<SubresourceRange> subresourceRanges;

  for (unsigned subresourceIndex = 0; subresourceIndex < tiledResource.Subresources.size();
       ++subresourceIndex) {
    D3D12_SUBRESOURCE_TILING& tiling = tiledResource.Subresources[subresourceIndex];
    if (tiling.StartTileIndexInOverallResource != D3D12_PACKED_TILE) {
      SubresourceRange range{};
      range.subresourceIndex = subresourceIndex;
      range.startTile = tiling.StartTileIndexInOverallResource;
      range.numTiles = tiling.DepthInTiles * tiling.HeightInTiles * tiling.WidthInTiles;
      range.packed = false;
      subresourceRanges.push_back(range);
    }
  }
  if (tiledResource.PackedMipInfo.NumPackedMips) {
    unsigned arraySize = tiledResource.Desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D
                             ? tiledResource.Desc.DepthOrArraySize
                             : 1;
    unsigned firstPackedMip =
        tiledResource.Desc.MipLevels - tiledResource.PackedMipInfo.NumPackedMips;
    unsigned arraySliceSize = tiledResource.Subresources.size() / arraySize;
    unsigned tilesPerArraySlice = tilesNum / arraySize;
    for (unsigned arrayIndex = 0; arrayIndex < arraySize; ++arrayIndex) {
      SubresourceRange range{};
      range.subresourceIndex = arrayIndex * arraySliceSize + firstPackedMip;
      range.startTile = arrayIndex * tilesPerArraySlice +
                        tiledResource.PackedMipInfo.StartTileIndexInOverallResource;
      range.numTiles = tiledResource.PackedMipInfo.NumTilesForPackedMips;
      range.packed = true;
      subresourceRanges.push_back(range);

      for (unsigned subresourceInSlice = firstPackedMip; subresourceInSlice < arraySliceSize;
           ++subresourceInSlice) {
        unsigned subresourceIndex = arrayIndex * arraySliceSize + subresourceInSlice;
        tiledResource.PackedSubresourcesStartTiles[subresourceIndex] = range.startTile;
      }
    }
  }

  for (unsigned tileIndex = 0; tileIndex < tilesNum; ++tileIndex) {
    Tile& tile = tiledResource.Tiles[tileIndex];
    bool found = false;
    for (SubresourceRange& range : subresourceRanges) {
      if (tileIndex >= range.startTile && tileIndex < range.startTile + range.numTiles) {
        tile.Packed = range.packed;
        tile.SubresourceIndex = range.subresourceIndex;
        found = true;
        break;
      }
    }
    GITS_ASSERT(found);
  }
}

void ReservedResourcesService::CopySourceBarrier(ID3D12Resource* resource,
                                                 unsigned resourceKey,
                                                 bool restoreState) {
  ResourceStateTrackingService::ResourceStates& resourceStates =
      m_StateService.GetResourceStateTrackingService().GetResourceStates(resourceKey);
  std::vector<D3D12_RESOURCE_BARRIER> barriers;
  std::vector<std::unique_ptr<D3D12_TEXTURE_BARRIER>> enhancedBarriers;
  std::vector<D3D12_BARRIER_GROUP> enhancedBarrierGroups;

  if (resourceStates.AllEqual) {
    if (!resourceStates.SubresourceStates[0].Enhanced) {
      if (resourceStates.SubresourceStates[0].State != D3D12_RESOURCE_STATE_COPY_SOURCE) {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = restoreState ? D3D12_RESOURCE_STATE_COPY_SOURCE
                                                      : resourceStates.SubresourceStates[0].State;
        barrier.Transition.StateAfter = restoreState ? resourceStates.SubresourceStates[0].State
                                                     : D3D12_RESOURCE_STATE_COPY_SOURCE;
        barriers.push_back(barrier);
      }
    } else {
      if (resourceStates.SubresourceStates[0].Layout != D3D12_BARRIER_LAYOUT_COPY_SOURCE &&
          resourceStates.SubresourceStates[0].Layout != D3D12_BARRIER_LAYOUT_UNDEFINED) {
        D3D12_TEXTURE_BARRIER* barrier = new D3D12_TEXTURE_BARRIER{};
        barrier->SyncBefore = restoreState ? D3D12_BARRIER_SYNC_ALL : D3D12_BARRIER_SYNC_ALL;
        barrier->SyncAfter = restoreState ? D3D12_BARRIER_SYNC_ALL : D3D12_BARRIER_SYNC_ALL;
        barrier->AccessBefore =
            restoreState ? D3D12_BARRIER_ACCESS_COPY_SOURCE : D3D12_BARRIER_ACCESS_COMMON;
        barrier->AccessAfter =
            restoreState ? D3D12_BARRIER_ACCESS_COMMON : D3D12_BARRIER_ACCESS_COPY_SOURCE;
        barrier->LayoutBefore = restoreState ? D3D12_BARRIER_LAYOUT_COPY_SOURCE
                                             : resourceStates.SubresourceStates[0].Layout;
        barrier->LayoutAfter = restoreState ? resourceStates.SubresourceStates[0].Layout
                                            : D3D12_BARRIER_LAYOUT_COPY_SOURCE;
        barrier->pResource = resource;
        barrier->Subresources.IndexOrFirstMipLevel = 0;
        enhancedBarriers.emplace_back(barrier);
        D3D12_BARRIER_GROUP barrierGroup{};
        barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
        barrierGroup.NumBarriers = 1;
        barrierGroup.pTextureBarriers = enhancedBarriers.back().get();
        enhancedBarrierGroups.push_back(barrierGroup);
      }
    }
  } else {
    for (unsigned i = 0; i < resourceStates.SubresourceStates.size(); ++i) {
      if (!resourceStates.SubresourceStates[i].Enhanced) {
        if (resourceStates.SubresourceStates[i].State != D3D12_RESOURCE_STATE_COPY_SOURCE) {
          D3D12_RESOURCE_BARRIER barrier{};
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Transition.pResource = resource;
          barrier.Transition.Subresource = i;
          barrier.Transition.StateBefore = restoreState ? D3D12_RESOURCE_STATE_COPY_SOURCE
                                                        : resourceStates.SubresourceStates[i].State;
          barrier.Transition.StateAfter = restoreState ? resourceStates.SubresourceStates[i].State
                                                       : D3D12_RESOURCE_STATE_COPY_SOURCE;
          barriers.push_back(barrier);
        }
      } else {
        if (resourceStates.SubresourceStates[i].Layout != D3D12_BARRIER_LAYOUT_COPY_SOURCE &&
            resourceStates.SubresourceStates[i].Layout != D3D12_BARRIER_LAYOUT_UNDEFINED) {
          D3D12_TEXTURE_BARRIER* barrier = new D3D12_TEXTURE_BARRIER{};
          barrier->SyncBefore = restoreState ? D3D12_BARRIER_SYNC_ALL : D3D12_BARRIER_SYNC_ALL;
          barrier->SyncAfter = restoreState ? D3D12_BARRIER_SYNC_ALL : D3D12_BARRIER_SYNC_ALL;
          barrier->AccessBefore =
              restoreState ? D3D12_BARRIER_ACCESS_COPY_SOURCE : D3D12_BARRIER_ACCESS_COMMON;
          barrier->AccessAfter =
              restoreState ? D3D12_BARRIER_ACCESS_COMMON : D3D12_BARRIER_ACCESS_COPY_SOURCE;
          barrier->LayoutBefore = restoreState ? D3D12_BARRIER_LAYOUT_COPY_SOURCE
                                               : resourceStates.SubresourceStates[i].Layout;
          barrier->LayoutAfter = restoreState ? resourceStates.SubresourceStates[i].Layout
                                              : D3D12_BARRIER_LAYOUT_COPY_SOURCE;
          barrier->pResource = resource;
          barrier->Subresources.IndexOrFirstMipLevel = i;
          enhancedBarriers.emplace_back(barrier);
          D3D12_BARRIER_GROUP barrierGroup{};
          barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
          barrierGroup.NumBarriers = 1;
          barrierGroup.pTextureBarriers = enhancedBarriers.back().get();
          enhancedBarrierGroups.push_back(barrierGroup);
        }
      }
    }
  }

  if (!barriers.empty()) {
    m_CommandList->ResourceBarrier(barriers.size(), barriers.data());
  }
  if (!enhancedBarrierGroups.empty()) {
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> commandList7;
    if (SUCCEEDED(m_CommandList->QueryInterface(IID_PPV_ARGS(&commandList7)))) {
      commandList7->Barrier(enhancedBarrierGroups.size(), enhancedBarrierGroups.data());
    }
  }
}

void ReservedResourcesService::DestroyObject(unsigned objectKey) {
  auto itResource = m_Resources.find(objectKey);
  if (itResource != m_Resources.end()) {
    itResource->second->Destroyed = true;
    return;
  }

  auto itHeap = m_ResourcesByHeapKey.find(objectKey);
  if (itHeap == m_ResourcesByHeapKey.end()) {
    return;
  }
  for (unsigned ResourceKey : itHeap->second) {
    auto itResource = m_Resources.find(ResourceKey);
    if (itResource == m_Resources.end()) {
      continue;
    }
    for (Tile& tile : itResource->second->Tiles) {
      if (tile.HeapKey == objectKey) {
        tile.HeapKey = 0;
        tile.HeapOffset = 0;
      }
    }
  }
  m_ResourcesByHeapKey.erase(itHeap);
}

ReservedResourcesService::TiledResource* ReservedResourcesService::GetTiledResource(
    unsigned resourceKey) {
  auto it = m_Resources.find(resourceKey);
  if (it == m_Resources.end()) {
    return nullptr;
  }
  return it->second.get();
}

void ReservedResourcesService::UpdateTileMappings(TiledResource& tiledResource,
                                                  unsigned commandQueueKey,
                                                  TileRegionsBySubresource* tileRegions) {
  unsigned arraySize = tiledResource.Desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D
                           ? tiledResource.Desc.DepthOrArraySize
                           : 1;

  std::set<unsigned> heapKeys;
  for (Tile& tile : tiledResource.Tiles) {
    if (tile.HeapKey) {
      heapKeys.insert(tile.HeapKey);
    }
  }

  for (unsigned heapKey : heapKeys) {
    m_StateService.RestoreState(heapKey);

    ID3D12CommandQueueUpdateTileMappingsCommand Command;
    Command.Key = m_StateService.GetUniqueCommandKey();
    Command.m_Object.Key = commandQueueKey;
    Command.m_pResource.Key = tiledResource.ResourceKey;
    Command.m_pHeap.Key = heapKey;

    std::vector<D3D12_TILED_RESOURCE_COORDINATE> coords;
    std::vector<D3D12_TILE_REGION_SIZE> sizes;
    std::vector<D3D12_TILE_RANGE_FLAGS> heapRangeFlags;
    std::vector<unsigned> heapRangeOffsets;
    std::vector<unsigned> heapRangeSizes;

    // non packed tiles
    unsigned packedTileIndex = 0;
    for (unsigned tileIndex = 0; tileIndex < tiledResource.Tiles.size(); ++tileIndex) {

      if (tileIndex % (tiledResource.Tiles.size() / arraySize) == 0) {
        packedTileIndex = 0;
      }

      Tile& tile = tiledResource.Tiles[tileIndex];

      if (tile.HeapKey == heapKey) {
        D3D12_TILED_RESOURCE_COORDINATE coord{};
        coord.Subresource = tile.SubresourceIndex;
        if (tile.Packed) {
          coord.X = packedTileIndex;
          coord.Y = 0;
          coord.Z = 0;
          ++packedTileIndex;
        } else {
          unsigned subresourceFirstTile =
              tiledResource.Subresources[tile.SubresourceIndex].StartTileIndexInOverallResource;
          D3D12_SUBRESOURCE_TILING& subresource = tiledResource.Subresources[tile.SubresourceIndex];
          coord.Z = (tileIndex - subresourceFirstTile) /
                    (subresource.HeightInTiles * subresource.WidthInTiles);
          coord.Y = ((tileIndex - subresourceFirstTile) -
                     subresource.HeightInTiles * subresource.WidthInTiles * coord.Z) /
                    subresource.WidthInTiles;
          coord.X = (tileIndex - subresourceFirstTile) -
                    subresource.HeightInTiles * subresource.WidthInTiles * coord.Z -
                    subresource.WidthInTiles * coord.Y;
        }
        coords.push_back(coord);

        D3D12_TILE_REGION_SIZE size{};
        size.NumTiles = 1;
        size.UseBox = FALSE;
        sizes.push_back(size);

        heapRangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NONE);
        heapRangeOffsets.push_back(tile.HeapOffset);
        heapRangeSizes.push_back(1);

        if (tileRegions) {
          TileRegion tileRegion{};
          tileRegion.Coord = coord;
          tileRegion.Size = size;
          tileRegion.Packed = tile.Packed;
          (*tileRegions)[tile.SubresourceIndex].push_back(tileRegion);
        }
      }
    }

    Command.m_NumResourceRegions.Value = coords.size();
    Command.m_pResourceRegionStartCoordinates.Value = coords.data();
    Command.m_pResourceRegionStartCoordinates.Size = coords.size();
    Command.m_pResourceRegionSizes.Value = sizes.data();
    Command.m_pResourceRegionSizes.Size = sizes.size();
    Command.m_NumRanges.Value = heapRangeFlags.size();
    Command.m_pRangeFlags.Value = heapRangeFlags.data();
    Command.m_pRangeFlags.Size = heapRangeFlags.size();
    Command.m_pHeapRangeStartOffsets.Value = heapRangeOffsets.data();
    Command.m_pHeapRangeStartOffsets.Size = heapRangeOffsets.size();
    Command.m_pRangeTileCounts.Value = heapRangeSizes.data();
    Command.m_pRangeTileCounts.Size = heapRangeSizes.size();
    Command.m_Flags.Value = D3D12_TILE_MAPPING_FLAG_NONE;
    m_StateService.GetRecorder().Record(ID3D12CommandQueueUpdateTileMappingsSerializer(Command));
  }
}

void ReservedResourcesService::RestoreContent(const std::vector<unsigned>& resourceKeys) {
  if (m_Resources.empty()) {
    return;
  }

  InitRestore();

  for (unsigned ResourceKey : resourceKeys) {
    if (m_Resources.find(ResourceKey) == m_Resources.end()) {
      continue;
    }
    if (!m_StateService.StateRestored(ResourceKey)) {
      continue;
    }
    TiledResource* tiledResource = m_Resources.at(ResourceKey).get();
    if (tiledResource->Destroyed) {
      continue;
    }

    TileRegionsBySubresource tileRegions;
    UpdateTileMappings(*tiledResource, m_CommandQueueKey, &tileRegions);

    std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>> subresourceSizes;
    GetSubresourceSizes(m_Device, tiledResource->Desc, subresourceSizes);

    // copy resources contents into readback resource

    std::vector<bool> subresourceFullyMappedFlags(tiledResource->Subresources.size(), true);
    std::set<unsigned> heapKeys;
    for (const auto& tile : tiledResource->Tiles) {
      if (!tile.HeapKey) {
        subresourceFullyMappedFlags.at(tile.SubresourceIndex) = false;
      } else {
        heapKeys.insert(tile.HeapKey);
      }
    }

    const UINT64 tileSize = 64 * 1024;
    UINT64 totalSize = 0;
    for (unsigned subresourceIndex = 0; subresourceIndex < tiledResource->Subresources.size();
         ++subresourceIndex) {
      if (subresourceFullyMappedFlags.at(subresourceIndex)) {
        totalSize += subresourceSizes[subresourceIndex].first;
      } else if (tileRegions.count(subresourceIndex)) {
        for (TileRegion& region : tileRegions.at(subresourceIndex)) {
          totalSize += region.Size.NumTiles * tileSize;
        }
      }
    }
    if (totalSize == 0) {
      continue;
    }

    GITS_ASSERT(totalSize <= m_UploadResourceSize);

    D3D12_HEAP_PROPERTIES heapPropertiesReadback{};
    heapPropertiesReadback.Type = D3D12_HEAP_TYPE_READBACK;
    heapPropertiesReadback.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapPropertiesReadback.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapPropertiesReadback.CreationNodeMask = 1;
    heapPropertiesReadback.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC readbackResourceDesc{};
    readbackResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    readbackResourceDesc.Alignment = 0;
    readbackResourceDesc.Width = totalSize;
    readbackResourceDesc.Height = 1;
    readbackResourceDesc.DepthOrArraySize = 1;
    readbackResourceDesc.MipLevels = 1;
    readbackResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    readbackResourceDesc.SampleDesc.Count = 1;
    readbackResourceDesc.SampleDesc.Quality = 0;
    readbackResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    readbackResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    Microsoft::WRL::ComPtr<ID3D12Resource> readbackResource;
    HRESULT hr = m_Device->CreateCommittedResource(
        &heapPropertiesReadback, D3D12_HEAP_FLAG_NONE, &readbackResourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readbackResource));
    GITS_ASSERT(hr == S_OK);

    CopySourceBarrier(tiledResource->Resource, tiledResource->ResourceKey, false);

    UINT64 offsetReadback = 0;
    for (unsigned subresourceIndex = 0; subresourceIndex < tiledResource->Subresources.size();
         ++subresourceIndex) {
      if (subresourceFullyMappedFlags.at(subresourceIndex)) {
        if (tiledResource->Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
          m_CommandList->CopyBufferRegion(readbackResource.Get(), offsetReadback,
                                          tiledResource->Resource, 0,
                                          subresourceSizes.at(subresourceIndex).first);
        } else {
          D3D12_TEXTURE_COPY_LOCATION dest{};
          dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
          dest.pResource = readbackResource.Get();
          dest.PlacedFootprint.Footprint = subresourceSizes.at(subresourceIndex).second.Footprint;
          dest.PlacedFootprint.Offset = offsetReadback;
          D3D12_TEXTURE_COPY_LOCATION src{};
          src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
          src.pResource = tiledResource->Resource;
          src.SubresourceIndex = subresourceIndex;
          m_CommandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
        }
        offsetReadback += subresourceSizes.at(subresourceIndex).first;
      } else if (tileRegions.count(subresourceIndex)) {
        for (TileRegion& region : tileRegions.at(subresourceIndex)) {
          GITS_ASSERT(!region.Packed);
          if (tiledResource->Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
            m_CommandList->CopyBufferRegion(readbackResource.Get(), offsetReadback,
                                            tiledResource->Resource, region.Coord.X * tileSize,
                                            region.Size.NumTiles * tileSize);
          } else {
            m_CommandList->CopyTiles(tiledResource->Resource, &region.Coord, &region.Size,
                                     readbackResource.Get(), offsetReadback,
                                     D3D12_TILE_COPY_FLAG_SWIZZLED_TILED_RESOURCE_TO_LINEAR_BUFFER);
          }
          offsetReadback += region.Size.NumTiles * tileSize;
        }
      }
    }

    CopySourceBarrier(tiledResource->Resource, tiledResource->ResourceKey, true);

    m_CommandList->Close();

    // increase residency count or make resident

    std::vector<ID3D12Pageable*> residencyObjects;
    for (const auto heapKey : heapKeys) {
      ObjectState* heapState = m_StateService.GetState(heapKey);
      residencyObjects.push_back(static_cast<ID3D12Pageable*>(heapState->Object));
    }
    if (!residencyObjects.empty()) {
      m_Device->MakeResident(residencyObjects.size(), residencyObjects.data());
    }

    ID3D12CommandList* commandLists[] = {m_CommandList};
    m_CommandQueue->ExecuteCommandLists(1, commandLists);
    m_CommandQueue->Signal(m_Fence, ++m_CurrentFenceValue);
    while (m_Fence->GetCompletedValue() < m_CurrentFenceValue) {
    }

    hr = m_CommandAllocator->Reset();
    GITS_ASSERT(hr == S_OK);
    hr = m_CommandList->Reset(m_CommandAllocator, nullptr);
    GITS_ASSERT(hr == S_OK);

    // decrese residency count or Evict

    if (!residencyObjects.empty()) {
      HRESULT hr = m_Device->Evict(residencyObjects.size(), residencyObjects.data());
      GITS_ASSERT(hr == S_OK);
    }

    // create upload resource with resources contents in subcaptured stream

    unsigned deviceKey = m_StateService.GetDeviceKey();

    void* mappedData{};
    hr = readbackResource->Map(0, nullptr, &mappedData);

    ID3D12ResourceMapCommand mapCommand;
    mapCommand.Key = m_StateService.GetUniqueCommandKey();
    mapCommand.m_Object.Key = m_UploadResourceKey;
    mapCommand.m_Subresource.Value = 0;
    mapCommand.m_pReadRange.Value = nullptr;
    mapCommand.m_ppData.CaptureValue = m_StateService.GetUniqueFakePointer();
    mapCommand.m_ppData.Value = &mapCommand.m_ppData.CaptureValue;
    m_StateService.GetRecorder().Record(ID3D12ResourceMapSerializer(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.Key = m_StateService.GetUniqueCommandKey();
    metaCommand.m_resource.Key = m_UploadResourceKey;
    metaCommand.m_mappedAddress.Value = mapCommand.m_ppData.CaptureValue;
    metaCommand.m_offset.Value = 0;
    metaCommand.m_data.Value = mappedData;
    metaCommand.m_data.Size = readbackResourceDesc.Width;
    m_StateService.GetRecorder().Record(MappedDataMetaSerializer(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.Key = m_StateService.GetUniqueCommandKey();
    unmapCommand.m_Object.Key = m_UploadResourceKey;
    unmapCommand.m_Subresource.Value = 0;
    unmapCommand.m_pWrittenRange.Value = nullptr;
    m_StateService.GetRecorder().Record(ID3D12ResourceUnmapSerializer(unmapCommand));

    readbackResource->Unmap(0, nullptr);

    // restore resources contents from upload resource in subcaptured stream

    UINT64 offsetUpload = 0;
    for (unsigned subresourceIndex = 0; subresourceIndex < tiledResource->Subresources.size();
         ++subresourceIndex) {
      if (subresourceFullyMappedFlags.at(subresourceIndex)) {
        if (tiledResource->Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
          ID3D12GraphicsCommandListCopyBufferRegionCommand copyBufferRegion;
          copyBufferRegion.Key = m_StateService.GetUniqueCommandKey();
          copyBufferRegion.m_Object.Key = m_CommandListKey;
          copyBufferRegion.m_pDstBuffer.Key = tiledResource->ResourceKey;
          copyBufferRegion.m_DstOffset.Value = 0;
          copyBufferRegion.m_pSrcBuffer.Key = m_UploadResourceKey;
          copyBufferRegion.m_SrcOffset.Value = offsetUpload;
          copyBufferRegion.m_NumBytes.Value = subresourceSizes.at(subresourceIndex).first;
          m_StateService.GetRecorder().Record(
              ID3D12GraphicsCommandListCopyBufferRegionSerializer(copyBufferRegion));
        } else {
          D3D12_TEXTURE_COPY_LOCATION dest{};
          dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
          dest.SubresourceIndex = subresourceIndex;

          D3D12_TEXTURE_COPY_LOCATION src{};
          src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
          src.PlacedFootprint.Footprint = subresourceSizes.at(subresourceIndex).second.Footprint;
          src.PlacedFootprint.Offset = offsetUpload;

          ID3D12GraphicsCommandListCopyTextureRegionCommand copyTextureRegion;
          copyTextureRegion.Key = m_StateService.GetUniqueCommandKey();
          copyTextureRegion.m_Object.Key = m_CommandListKey;
          copyTextureRegion.m_pDst.Value = &dest;
          copyTextureRegion.m_pDst.ResourceKey = tiledResource->ResourceKey;
          copyTextureRegion.m_pSrc.Value = &src;
          copyTextureRegion.m_pSrc.ResourceKey = m_UploadResourceKey;
          m_StateService.GetRecorder().Record(
              ID3D12GraphicsCommandListCopyTextureRegionSerializer(copyTextureRegion));
        }
        offsetUpload += subresourceSizes.at(subresourceIndex).first;
      } else if (tileRegions.count(subresourceIndex)) {
        for (TileRegion& region : tileRegions.at(subresourceIndex)) {
          GITS_ASSERT(!region.Packed);
          if (tiledResource->Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
            ID3D12GraphicsCommandListCopyBufferRegionCommand copyBufferRegion;
            copyBufferRegion.Key = m_StateService.GetUniqueCommandKey();
            copyBufferRegion.m_Object.Key = m_CommandListKey;
            copyBufferRegion.m_pDstBuffer.Key = tiledResource->ResourceKey;
            copyBufferRegion.m_DstOffset.Value = region.Coord.X * tileSize;
            copyBufferRegion.m_pSrcBuffer.Key = m_UploadResourceKey;
            copyBufferRegion.m_SrcOffset.Value = offsetUpload;
            copyBufferRegion.m_NumBytes.Value = region.Size.NumTiles * tileSize;
            m_StateService.GetRecorder().Record(
                ID3D12GraphicsCommandListCopyBufferRegionSerializer(copyBufferRegion));
          } else {
            ID3D12GraphicsCommandListCopyTilesCommand copyTiles;
            copyTiles.Key = m_StateService.GetUniqueCommandKey();
            copyTiles.m_Object.Key = m_CommandListKey;
            copyTiles.m_pTiledResource.Key = tiledResource->ResourceKey;
            copyTiles.m_pTileRegionStartCoordinate.Value = &region.Coord;
            copyTiles.m_pTileRegionSize.Value = &region.Size;
            copyTiles.m_pBuffer.Key = m_UploadResourceKey;
            copyTiles.m_BufferStartOffsetInBytes.Value = offsetUpload;
            copyTiles.m_Flags.Value = D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE;
            m_StateService.GetRecorder().Record(
                ID3D12GraphicsCommandListCopyTilesSerializer(copyTiles));
          }
          offsetUpload += region.Size.NumTiles * tileSize;
        }
      }
    }

    ID3D12GraphicsCommandListCloseCommand commandListClose;
    commandListClose.Key = m_StateService.GetUniqueCommandKey();
    commandListClose.m_Object.Key = m_CommandListKey;
    m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListCloseSerializer(commandListClose));

    // increase residency count or make resident

    if (!heapKeys.empty()) {
      ID3D12DeviceMakeResidentCommand MakeResident;
      MakeResident.Key = m_StateService.GetUniqueCommandKey();
      MakeResident.m_Object.Key = deviceKey;
      MakeResident.m_NumObjects.Value = heapKeys.size();
      ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
      MakeResident.m_ppObjects.Value = &fakePtr;
      MakeResident.m_ppObjects.Size = heapKeys.size();
      for (unsigned key : heapKeys) {
        MakeResident.m_ppObjects.Keys.push_back(key);
      }
      m_StateService.GetRecorder().Record(ID3D12DeviceMakeResidentSerializer(MakeResident));
    }

    ID3D12CommandQueueExecuteCommandListsCommand ExecuteCommandLists;
    ExecuteCommandLists.Key = m_StateService.GetUniqueCommandKey();
    ExecuteCommandLists.m_Object.Key = m_CommandQueueKey;
    ExecuteCommandLists.m_NumCommandLists.Value = 1;
    ExecuteCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
    ExecuteCommandLists.m_ppCommandLists.Size = 1;
    ExecuteCommandLists.m_ppCommandLists.Keys.resize(1);
    ExecuteCommandLists.m_ppCommandLists.Keys[0] = m_CommandListKey;
    m_StateService.GetRecorder().Record(
        ID3D12CommandQueueExecuteCommandListsSerializer(ExecuteCommandLists));

    ID3D12CommandQueueSignalCommand CommandQueueSignal;
    CommandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
    CommandQueueSignal.m_Object.Key = m_CommandQueueKey;
    CommandQueueSignal.m_pFence.Key = m_FenceKey;
    CommandQueueSignal.m_Value.Value = ++m_RecordedFenceValue;
    m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(CommandQueueSignal));

    ID3D12FenceGetCompletedValueCommand getCompletedValue;
    getCompletedValue.Key = m_StateService.GetUniqueCommandKey();
    getCompletedValue.m_Object.Key = m_FenceKey;
    getCompletedValue.m_Result.Value = m_RecordedFenceValue;
    m_StateService.GetRecorder().Record(ID3D12FenceGetCompletedValueSerializer(getCompletedValue));

    ID3D12CommandAllocatorResetCommand commandAllocatorReset;
    commandAllocatorReset.Key = m_StateService.GetUniqueCommandKey();
    commandAllocatorReset.m_Object.Key = m_CommandAllocatorKey;
    m_StateService.GetRecorder().Record(
        ID3D12CommandAllocatorResetSerializer(commandAllocatorReset));

    ID3D12GraphicsCommandListResetCommand CommandListReset;
    CommandListReset.Key = m_StateService.GetUniqueCommandKey();
    CommandListReset.m_Object.Key = m_CommandListKey;
    CommandListReset.m_pAllocator.Key = m_CommandAllocatorKey;
    CommandListReset.m_pInitialState.Key = 0;
    m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListResetSerializer(CommandListReset));

    // decrese residency count or Evict

    if (!heapKeys.empty()) {
      ID3D12DeviceEvictCommand Evict;
      Evict.Key = m_StateService.GetUniqueCommandKey();
      Evict.m_Object.Key = deviceKey;
      Evict.m_NumObjects.Value = heapKeys.size();
      ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
      Evict.m_ppObjects.Value = &fakePtr;
      Evict.m_ppObjects.Size = heapKeys.size();
      for (unsigned key : heapKeys) {
        Evict.m_ppObjects.Keys.push_back(key);
      }
      m_StateService.GetRecorder().Record(ID3D12DeviceEvictSerializer(Evict));
    }
  }
}

void ReservedResourcesService::InitRestore() {
  if (m_ContentRestoreInitialized) {
    return;
  }

  size_t maxUploadSize = 0;
  for (const auto& tiledResource : m_Resources) {
    if (tiledResource.second->Destroyed) {
      continue;
    }

    std::vector<bool> subresourceFullyMappedFlags(tiledResource.second->Subresources.size(), true);
    std::map<unsigned, unsigned> numTilesBySubresourceIndex;
    for (const auto& tile : tiledResource.second->Tiles) {
      if (tile.HeapKey) {
        ++numTilesBySubresourceIndex[tile.SubresourceIndex];
      } else {
        subresourceFullyMappedFlags.at(tile.SubresourceIndex) = false;
      }
    }

    std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>> subresourceSizes;
    GetSubresourceSizes(m_Device, tiledResource.second->Desc, subresourceSizes);

    const size_t tileSize = 64 * 1024;
    size_t uploadSize = 0;
    for (unsigned subresourceIndex = 0;
         subresourceIndex < tiledResource.second->Subresources.size(); ++subresourceIndex) {
      if (subresourceFullyMappedFlags.at(subresourceIndex)) {
        uploadSize += subresourceSizes[subresourceIndex].first;
      } else if (numTilesBySubresourceIndex.count(subresourceIndex)) {
        uploadSize += numTilesBySubresourceIndex[subresourceIndex] * tileSize;
      }
    }

    if (uploadSize > maxUploadSize) {
      maxUploadSize = uploadSize;
    }
  }

  m_UploadResourceSize = maxUploadSize;

  unsigned deviceKey = m_StateService.GetDeviceKey();

  if (m_UploadResourceSize) {
    m_UploadResourceKey = m_StateService.GetUniqueObjectKey();

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = m_UploadResourceSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapPropertiesUpload{};
    heapPropertiesUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapPropertiesUpload.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapPropertiesUpload.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapPropertiesUpload.CreationNodeMask = 1;
    heapPropertiesUpload.VisibleNodeMask = 1;

    ID3D12DeviceCreateCommittedResourceCommand createUploadResource;
    createUploadResource.Key = m_StateService.GetUniqueCommandKey();
    createUploadResource.m_Object.Key = deviceKey;
    createUploadResource.m_pHeapProperties.Value = &heapPropertiesUpload;
    createUploadResource.m_HeapFlags.Value = D3D12_HEAP_FLAG_NONE;
    createUploadResource.m_pDesc.Value = &resourceDesc;
    createUploadResource.m_InitialResourceState.Value = D3D12_RESOURCE_STATE_GENERIC_READ;
    createUploadResource.m_pOptimizedClearValue.Value = nullptr;
    createUploadResource.m_riidResource.Value = IID_ID3D12Resource;
    createUploadResource.m_ppvResource.Key = m_UploadResourceKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommittedResourceSerializer(createUploadResource));
  }

  D3D12_COMMAND_QUEUE_DESC commandQueueDirectDesc{};
  commandQueueDirectDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  HRESULT hr = m_Device->CreateCommandQueue(&commandQueueDirectDesc, IID_PPV_ARGS(&m_CommandQueue));
  GITS_ASSERT(hr == S_OK);
  hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                        IID_PPV_ARGS(&m_CommandAllocator));
  GITS_ASSERT(hr == S_OK);
  hr = m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator, nullptr,
                                   IID_PPV_ARGS(&m_CommandList));
  GITS_ASSERT(hr == S_OK);
  hr = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
  GITS_ASSERT(hr == S_OK);

  m_CommandQueueKey = m_StateService.GetUniqueObjectKey();
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
  ID3D12DeviceCreateCommandQueueCommand CreateCommandQueue;
  CreateCommandQueue.Key = m_StateService.GetUniqueCommandKey();
  CreateCommandQueue.m_Object.Key = deviceKey;
  CreateCommandQueue.m_pDesc.Value = &commandQueueDesc;
  CreateCommandQueue.m_riid.Value = IID_ID3D12CommandQueue;
  CreateCommandQueue.m_ppCommandQueue.Key = m_CommandQueueKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateCommandQueueSerializer(CreateCommandQueue));

  m_CommandAllocatorKey = m_StateService.GetUniqueObjectKey();
  ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
  createCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
  createCommandAllocator.m_Object.Key = deviceKey;
  createCommandAllocator.m_type.Value = D3D12_COMMAND_LIST_TYPE_COPY;
  createCommandAllocator.m_riid.Value = IID_ID3D12CommandAllocator;
  createCommandAllocator.m_ppCommandAllocator.Key = m_CommandAllocatorKey;
  m_StateService.GetRecorder().Record(
      ID3D12DeviceCreateCommandAllocatorSerializer(createCommandAllocator));

  m_CommandListKey = m_StateService.GetUniqueObjectKey();
  ID3D12DeviceCreateCommandListCommand createCommandList;
  createCommandList.Key = m_StateService.GetUniqueCommandKey();
  createCommandList.m_Object.Key = deviceKey;
  createCommandList.m_nodeMask.Value = 0;
  createCommandList.m_pCommandAllocator.Key = createCommandAllocator.m_ppCommandAllocator.Key;
  createCommandList.m_type.Value = D3D12_COMMAND_LIST_TYPE_COPY;
  createCommandList.m_pInitialState.Value = nullptr;
  createCommandList.m_riid.Value = IID_ID3D12CommandList;
  createCommandList.m_ppCommandList.Key = m_CommandListKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateCommandListSerializer(createCommandList));

  m_FenceKey = m_StateService.GetUniqueObjectKey();
  ID3D12DeviceCreateFenceCommand createFence;
  createFence.Key = m_StateService.GetUniqueCommandKey();
  createFence.m_Object.Key = deviceKey;
  createFence.m_InitialValue.Value = 0;
  createFence.m_Flags.Value = D3D12_FENCE_FLAG_NONE;
  createFence.m_riid.Value = IID_ID3D12Fence;
  createFence.m_ppFence.Key = m_FenceKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateFenceSerializer(createFence));

  m_ContentRestoreInitialized = true;
}

void ReservedResourcesService::CleanupRestore() {
  if (!m_ContentRestoreInitialized) {
    return;
  }

  m_Device->Release();
  m_CommandQueue->Release();
  m_CommandAllocator->Release();
  m_CommandList->Release();
  m_Fence->Release();

  IUnknownReleaseCommand releaseFence;
  releaseFence.Key = m_StateService.GetUniqueCommandKey();
  releaseFence.m_Object.Key = m_FenceKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseFence));

  IUnknownReleaseCommand releaseCommandList;
  releaseCommandList.Key = m_StateService.GetUniqueCommandKey();
  releaseCommandList.m_Object.Key = m_CommandListKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandList));

  IUnknownReleaseCommand releaseCommandAllocator;
  releaseCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
  releaseCommandAllocator.m_Object.Key = m_CommandAllocatorKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandAllocator));

  IUnknownReleaseCommand releaseCommandQueue;
  releaseCommandQueue.Key = m_StateService.GetUniqueCommandKey();
  releaseCommandQueue.m_Object.Key = m_CommandQueueKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandQueue));

  if (m_UploadResourceKey) {
    IUnknownReleaseCommand releaseUploadResource;
    releaseUploadResource.Key = m_StateService.GetUniqueCommandKey();
    releaseUploadResource.m_Object.Key = m_UploadResourceKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseUploadResource));
  }
}

void ReservedResourcesService::GetSubresourceSizes(
    ID3D12Device* device,
    D3D12_RESOURCE_DESC& desc,
    std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>>& sizes) {

  unsigned planes = 1;
  D3D12_FEATURE_DATA_FORMAT_INFO formatInfo = {desc.Format, 0};
  if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo,
                                            sizeof(formatInfo)))) {
    planes = formatInfo.PlaneCount;
  }

  unsigned subresources = desc.MipLevels * planes;
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
      desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D) {
    subresources *= desc.DepthOrArraySize;
  }

  sizes.resize(subresources);

  for (unsigned i = 0; i < subresources; ++i) {
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint = sizes[i].second;
    UINT64 size{};
    GetCopyableFootprintsSafe(device, &desc, i, 1, 0, &footprint, nullptr, nullptr, &size);
    sizes[i].first = size;
  }
}

} // namespace DirectX
} // namespace gits
