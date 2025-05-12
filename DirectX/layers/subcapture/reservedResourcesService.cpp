// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "reservedResourcesService.h"
#include "stateTrackingService.h"
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"

#include <wrl/client.h>
#include <set>

namespace gits {
namespace DirectX {

void ReservedResourcesService::addUpdateTileMappings(
    ID3D12CommandQueueUpdateTileMappingsCommand& c) {

  TiledResource* tiledResource{};
  auto it = resources_.find(c.pResource_.key);
  if (it != resources_.end()) {
    tiledResource = it->second.get();
  } else {
    tiledResource = new TiledResource();
    resources_[c.pResource_.key].reset(tiledResource);

    tiledResource->resource = c.pResource_.value;
    tiledResource->desc = c.pResource_.value->GetDesc();
    tiledResource->resourceKey = c.pResource_.key;

    initTiledResource(*tiledResource);
  }

  unsigned heapKey = c.pHeap_.key;
  unsigned heapRangeIndex = 0;
  unsigned heapOffset = heapKey && c.pHeapRangeStartOffsets_.value
                            ? c.pHeapRangeStartOffsets_.value[heapRangeIndex]
                            : 0;
  D3D12_TILE_RANGE_FLAGS heapRangeFlag = D3D12_TILE_RANGE_FLAG_NONE;
  if (c.pRangeFlags_.value) {
    heapRangeFlag = c.pRangeFlags_.value[heapRangeIndex];
  }
  unsigned heapRangeSize = tiledResource->tiles.size();
  if (c.pRangeTileCounts_.value) {
    heapRangeSize = c.pRangeTileCounts_.value[heapRangeIndex];
  }
  unsigned tileIndexInHeapRange = 0;

  for (unsigned region = 0; region < c.NumResourceRegions_.value; ++region) {
    D3D12_TILED_RESOURCE_COORDINATE coord{};
    if (c.pResourceRegionStartCoordinates_.value) {
      coord = c.pResourceRegionStartCoordinates_.value[region];
    }
    D3D12_TILE_REGION_SIZE size{};
    if (c.pResourceRegionSizes_.value) {
      size = c.pResourceRegionSizes_.value[region];
    } else if (c.pResourceRegionStartCoordinates_.value) {
      size.NumTiles = 1;
    } else if (c.NumResourceRegions_.value == 1) {
      size.NumTiles = tiledResource->tiles.size();
    }

    D3D12_SUBRESOURCE_TILING& subresource = tiledResource->subresources[coord.Subresource];

    auto updateTile = [&](Tile& tile) {
      if (!heapKey) {
        tile.heapKey = 0;
        tile.heapOffset = 0;
        return;
      }

      if (tileIndexInHeapRange == heapRangeSize) {
        tileIndexInHeapRange = 0;
        ++heapRangeIndex;
        GITS_ASSERT(heapRangeIndex < c.NumRanges_.value);
        heapOffset =
            c.pHeapRangeStartOffsets_.value ? c.pHeapRangeStartOffsets_.value[heapRangeIndex] : 0;
        heapRangeFlag = D3D12_TILE_RANGE_FLAG_NONE;
        if (c.pRangeFlags_.value) {
          heapRangeFlag = c.pRangeFlags_.value[heapRangeIndex];
        }
        heapRangeSize = tiledResource->tiles.size();
        if (c.pRangeTileCounts_.value) {
          heapRangeSize = c.pRangeTileCounts_.value[heapRangeIndex];
        }
      }

      if (heapRangeFlag == D3D12_TILE_RANGE_FLAG_NONE ||
          heapRangeFlag == D3D12_TILE_RANGE_FLAG_REUSE_SINGLE_TILE) {
        tile.heapKey = heapKey;
        tile.heapOffset = heapOffset;
      } else if (heapRangeFlag == D3D12_TILE_RANGE_FLAG_NULL) {
        tile.heapKey = 0;
        tile.heapOffset = 0;
      } else if (heapRangeFlag == D3D12_TILE_RANGE_FLAG_SKIP) {
        static bool logged = false;
        if (!logged) {
          Log(ERR) << "D3D12_TILE_RANGE_FLAG_SKIP is not handled in subcapture!";
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
            updateTile(tiledResource->tiles[tile]);
          }
        }
      }
    } else {
      unsigned startTile = 0;
      if (subresource.StartTileIndexInOverallResource == D3D12_PACKED_TILE) {
        startTile = tiledResource->packedSubresourcesStartTiles[coord.Subresource] + coord.X;
      } else {
        startTile = subresource.StartTileIndexInOverallResource +
                    coord.Z * (subresource.HeightInTiles * subresource.WidthInTiles) +
                    coord.Y * subresource.WidthInTiles + coord.X;
      }
      for (unsigned i = 0; i < size.NumTiles; ++i) {
        unsigned tile = startTile + i;
        updateTile(tiledResource->tiles[tile]);
      }
    }
  }

  ++tiledResource->updateId;
}

void ReservedResourcesService::initTiledResource(TiledResource& tiledResource) {
  HRESULT hr = tiledResource.resource->GetDevice(IID_PPV_ARGS(&device_));
  GITS_ASSERT(hr == S_OK);

  unsigned planes = 1;
  D3D12_FEATURE_DATA_FORMAT_INFO formatInfo = {tiledResource.desc.Format, 0};
  if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo,
                                             sizeof(formatInfo)))) {
    planes = formatInfo.PlaneCount;
  }
  unsigned subresources = tiledResource.desc.MipLevels * planes;
  if (tiledResource.desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
      tiledResource.desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D) {
    subresources *= tiledResource.desc.DepthOrArraySize;
  }

  unsigned tilesNum{};
  tiledResource.subresources.resize(subresources);
  device_->GetResourceTiling(tiledResource.resource, &tilesNum, &tiledResource.packedMipInfo,
                             nullptr, &subresources, 0, tiledResource.subresources.data());
  tiledResource.tiles.resize(tilesNum);

  struct SubresourceRange {
    unsigned subresourceIndex;
    unsigned startTile;
    unsigned numTiles;
    bool packed;
  };
  std::vector<SubresourceRange> subresourceRanges;

  for (unsigned subresourceIndex = 0; subresourceIndex < tiledResource.subresources.size();
       ++subresourceIndex) {
    D3D12_SUBRESOURCE_TILING& tiling = tiledResource.subresources[subresourceIndex];
    if (tiling.StartTileIndexInOverallResource != D3D12_PACKED_TILE) {
      SubresourceRange range{};
      range.subresourceIndex = subresourceIndex;
      range.startTile = tiling.StartTileIndexInOverallResource;
      range.numTiles = tiling.DepthInTiles * tiling.HeightInTiles * tiling.WidthInTiles;
      range.packed = false;
      subresourceRanges.push_back(range);
    }
  }
  if (tiledResource.packedMipInfo.NumPackedMips) {
    unsigned arraySize = tiledResource.desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D
                             ? tiledResource.desc.DepthOrArraySize
                             : 1;
    unsigned firstPackedMip =
        tiledResource.desc.MipLevels - tiledResource.packedMipInfo.NumPackedMips;
    unsigned arraySliceSize = tiledResource.subresources.size() / arraySize;
    unsigned tilesPerArraySlice = tilesNum / arraySize;
    for (unsigned arrayIndex = 0; arrayIndex < arraySize; ++arrayIndex) {
      SubresourceRange range{};
      range.subresourceIndex = arrayIndex * arraySliceSize + firstPackedMip;
      range.startTile = arrayIndex * tilesPerArraySlice +
                        tiledResource.packedMipInfo.StartTileIndexInOverallResource;
      range.numTiles = tiledResource.packedMipInfo.NumTilesForPackedMips;
      range.packed = true;
      subresourceRanges.push_back(range);

      for (unsigned subresourceInSlice = firstPackedMip; subresourceInSlice < arraySliceSize;
           ++subresourceInSlice) {
        unsigned subresourceIndex = arrayIndex * arraySliceSize + subresourceInSlice;
        tiledResource.packedSubresourcesStartTiles[subresourceIndex] = range.startTile;
      }
    }
  }

  for (unsigned tileIndex = 0; tileIndex < tilesNum; ++tileIndex) {
    Tile& tile = tiledResource.tiles[tileIndex];
    bool found = false;
    for (SubresourceRange& range : subresourceRanges) {
      if (tileIndex >= range.startTile && tileIndex < range.startTile + range.numTiles) {
        tile.packed = range.packed;
        tile.subresourceIndex = range.subresourceIndex;
        found = true;
        break;
      }
    }
    GITS_ASSERT(found);
  }
}

void ReservedResourcesService::destroyObject(unsigned objectKey) {
  auto it = resources_.find(objectKey);
  if (it != resources_.end()) {
    it->second->destroyed = true;
  }
}

ReservedResourcesService::TiledResource* ReservedResourcesService::getTiledResource(
    unsigned resourceKey) {
  auto it = resources_.find(resourceKey);
  if (it == resources_.end()) {
    return nullptr;
  }
  return it->second.get();
}

void ReservedResourcesService::updateTileMappings(TiledResource& tiledResource,
                                                  unsigned commandQueueKey,
                                                  TileRegionsBySubresource* tileRegions) {
  unsigned arraySize = tiledResource.desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D
                           ? tiledResource.desc.DepthOrArraySize
                           : 1;

  std::set<unsigned> heapKeys;
  for (Tile& tile : tiledResource.tiles) {
    if (tile.heapKey) {
      heapKeys.insert(tile.heapKey);
    }
  }

  for (unsigned heapKey : heapKeys) {

    ID3D12CommandQueueUpdateTileMappingsCommand command;
    command.key = stateService_.getUniqueCommandKey();
    command.object_.key = commandQueueKey;
    command.pResource_.key = tiledResource.resourceKey;
    command.pHeap_.key = heapKey;

    std::vector<D3D12_TILED_RESOURCE_COORDINATE> coords;
    std::vector<D3D12_TILE_REGION_SIZE> sizes;
    std::vector<D3D12_TILE_RANGE_FLAGS> heapRangeFlags;
    std::vector<unsigned> heapRangeOffsets;
    std::vector<unsigned> heapRangeSizes;

    // non packed tiles
    unsigned packedTileIndex = 0;
    for (unsigned tileIndex = 0; tileIndex < tiledResource.tiles.size(); ++tileIndex) {

      if (tileIndex % (tiledResource.tiles.size() / arraySize) == 0) {
        packedTileIndex = 0;
      }

      Tile& tile = tiledResource.tiles[tileIndex];

      if (tile.heapKey == heapKey) {
        D3D12_TILED_RESOURCE_COORDINATE coord{};
        coord.Subresource = tile.subresourceIndex;
        if (tile.packed) {
          coord.X = packedTileIndex;
          coord.Y = 0;
          coord.Z = 0;
          ++packedTileIndex;
        } else {
          unsigned subresourceFirstTile =
              tiledResource.subresources[tile.subresourceIndex].StartTileIndexInOverallResource;
          D3D12_SUBRESOURCE_TILING& subresource = tiledResource.subresources[tile.subresourceIndex];
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
        heapRangeOffsets.push_back(tile.heapOffset);
        heapRangeSizes.push_back(1);

        if (tileRegions) {
          TileRegion tileRegion{};
          tileRegion.coord = coord;
          tileRegion.size = size;
          tileRegion.packed = tile.packed;
          (*tileRegions)[tile.subresourceIndex].push_back(tileRegion);
        }
      }
    }

    command.NumResourceRegions_.value = coords.size();
    command.pResourceRegionStartCoordinates_.value = coords.data();
    command.pResourceRegionStartCoordinates_.size = coords.size();
    command.pResourceRegionSizes_.value = sizes.data();
    command.pResourceRegionSizes_.size = sizes.size();
    command.NumRanges_.value = heapRangeFlags.size();
    command.pRangeFlags_.value = heapRangeFlags.data();
    command.pRangeFlags_.size = heapRangeFlags.size();
    command.pHeapRangeStartOffsets_.value = heapRangeOffsets.data();
    command.pHeapRangeStartOffsets_.size = heapRangeOffsets.size();
    command.pRangeTileCounts_.value = heapRangeSizes.data();
    command.pRangeTileCounts_.size = heapRangeSizes.size();
    command.Flags_.value = D3D12_TILE_MAPPING_FLAG_NONE;
    stateService_.recorder_.record(new ID3D12CommandQueueUpdateTileMappingsWriter(command));
  }
}

void ReservedResourcesService::restoreContent() {

  if (resources_.empty()) {
    return;
  }

  initRestore();

  for (auto& itResource : resources_) {
    TiledResource* tiledResource = itResource.second.get();
    if (tiledResource->destroyed) {
      continue;
    }

    TileRegionsBySubresource tileRegions;
    updateTileMappings(*tiledResource, commandQueueKey_, &tileRegions);

    std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>> subresourceSizes;
    getSubresourceSizes(device_, tiledResource->desc, subresourceSizes);

    // copy resources contents into readback resource

    std::vector<bool> subresourceFullyMappedFlags(tiledResource->subresources.size(), true);
    std::set<unsigned> heapKeys;
    for (const auto& tile : tiledResource->tiles) {
      if (!tile.heapKey) {
        subresourceFullyMappedFlags.at(tile.subresourceIndex) = false;
      } else {
        heapKeys.insert(tile.heapKey);
      }
    }

    const UINT64 tileSize = 64 * 1024;
    UINT64 totalSize = 0;
    for (unsigned subresourceIndex = 0; subresourceIndex < tiledResource->subresources.size();
         ++subresourceIndex) {
      if (subresourceFullyMappedFlags.at(subresourceIndex)) {
        totalSize += subresourceSizes[subresourceIndex].first;
      } else if (tileRegions.count(subresourceIndex)) {
        for (TileRegion& region : tileRegions.at(subresourceIndex)) {
          totalSize += region.size.NumTiles * tileSize;
        }
      }
    }

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
    HRESULT hr = device_->CreateCommittedResource(
        &heapPropertiesReadback, D3D12_HEAP_FLAG_NONE, &readbackResourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readbackResource));
    GITS_ASSERT(hr == S_OK);

    {
      std::vector<D3D12_RESOURCE_BARRIER> barriers;
      ResourceStateTrackingService::ResourceStates& resourceStates =
          stateService_.resourceStateTrackingService_.getResourceStates(tiledResource->resourceKey);
      if (resourceStates.allEqual) {
        if (resourceStates.subresourceStates[0] != D3D12_RESOURCE_STATE_COPY_SOURCE) {
          D3D12_RESOURCE_BARRIER barrier{};
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          barrier.Transition.pResource = tiledResource->resource;
          barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          barrier.Transition.StateBefore = resourceStates.subresourceStates[0];
          barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
          barriers.push_back(barrier);
        }
      } else {
        for (unsigned i = 0; i < resourceStates.subresourceStates.size(); ++i) {
          if (resourceStates.subresourceStates[i] != D3D12_RESOURCE_STATE_COPY_SOURCE) {
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = tiledResource->resource;
            barrier.Transition.Subresource = i;
            barrier.Transition.StateBefore = resourceStates.subresourceStates[i];
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers.push_back(barrier);
          }
        }
      }
      if (!barriers.empty()) {
        commandList_->ResourceBarrier(barriers.size(), barriers.data());
      }
    }

    UINT64 offsetReadback = 0;
    for (unsigned subresourceIndex = 0; subresourceIndex < tiledResource->subresources.size();
         ++subresourceIndex) {
      if (subresourceFullyMappedFlags.at(subresourceIndex)) {
        if (tiledResource->desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
          commandList_->CopyBufferRegion(readbackResource.Get(), offsetReadback,
                                         tiledResource->resource, 0,
                                         subresourceSizes.at(subresourceIndex).first);
        } else {
          D3D12_TEXTURE_COPY_LOCATION dest{};
          dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
          dest.pResource = readbackResource.Get();
          dest.PlacedFootprint.Footprint = subresourceSizes.at(subresourceIndex).second.Footprint;
          dest.PlacedFootprint.Offset = offsetReadback;
          D3D12_TEXTURE_COPY_LOCATION src{};
          src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
          src.pResource = tiledResource->resource;
          src.SubresourceIndex = subresourceIndex;
          commandList_->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
        }
        offsetReadback += subresourceSizes.at(subresourceIndex).first;
      } else if (tileRegions.count(subresourceIndex)) {
        for (TileRegion& region : tileRegions.at(subresourceIndex)) {
          GITS_ASSERT(!region.packed);
          static bool logged = false;
          if (!logged) {
            Log(WARN) << "Subresource not fully mapped. Using CopyTiles";
            logged = true;
          }
          commandList_->CopyTiles(tiledResource->resource, &region.coord, &region.size,
                                  readbackResource.Get(), offsetReadback,
                                  D3D12_TILE_COPY_FLAG_SWIZZLED_TILED_RESOURCE_TO_LINEAR_BUFFER);
          offsetReadback += region.size.NumTiles * tileSize;
        }
      }
    }

    {
      std::vector<D3D12_RESOURCE_BARRIER> barriers;
      ResourceStateTrackingService::ResourceStates& resourceStates =
          stateService_.resourceStateTrackingService_.getResourceStates(tiledResource->resourceKey);
      if (resourceStates.allEqual) {
        if (resourceStates.subresourceStates[0] != D3D12_RESOURCE_STATE_COPY_SOURCE) {
          D3D12_RESOURCE_BARRIER barrier{};
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          barrier.Transition.pResource = tiledResource->resource;
          barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
          barrier.Transition.StateAfter = resourceStates.subresourceStates[0];
          barriers.push_back(barrier);
        }
      } else {
        for (unsigned i = 0; i < resourceStates.subresourceStates.size(); ++i) {
          if (resourceStates.subresourceStates[i] != D3D12_RESOURCE_STATE_COPY_SOURCE) {
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = tiledResource->resource;
            barrier.Transition.Subresource = i;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barrier.Transition.StateAfter = resourceStates.subresourceStates[i];
            barriers.push_back(barrier);
          }
        }
      }
      if (!barriers.empty()) {
        commandList_->ResourceBarrier(barriers.size(), barriers.data());
      }
    }

    commandList_->Close();

    // increase residency count or make resident

    std::vector<ID3D12Pageable*> residencyObjects;
    for (const auto heapKey : heapKeys) {
      ObjectState* heapState = stateService_.getState(heapKey);
      residencyObjects.push_back(static_cast<ID3D12Pageable*>(heapState->object));
    }
    if (!residencyObjects.empty()) {
      device_->MakeResident(residencyObjects.size(), residencyObjects.data());
    }

    ID3D12CommandList* commandLists[] = {commandList_};
    commandQueue_->ExecuteCommandLists(1, commandLists);
    commandQueue_->Signal(fence_, ++currentFenceValue_);
    while (fence_->GetCompletedValue() < currentFenceValue_) {
    }

    hr = commandAllocator_->Reset();
    GITS_ASSERT(hr == S_OK);
    hr = commandList_->Reset(commandAllocator_, nullptr);
    GITS_ASSERT(hr == S_OK);

    // decrese residency count or evict

    if (!residencyObjects.empty()) {
      HRESULT hr = device_->Evict(residencyObjects.size(), residencyObjects.data());
      GITS_ASSERT(hr == S_OK);
    }

    // create upload resource with resources contents in subcaptured stream

    unsigned deviceKey = stateService_.deviceKey_;
    unsigned uploadResourceKey = stateService_.getUniqueObjectKey();

    D3D12_HEAP_PROPERTIES heapPropertiesUpload{};
    heapPropertiesUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapPropertiesUpload.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapPropertiesUpload.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapPropertiesUpload.CreationNodeMask = 1;
    heapPropertiesUpload.VisibleNodeMask = 1;

    ID3D12DeviceCreateCommittedResourceCommand createUploadResource;
    createUploadResource.key = stateService_.getUniqueCommandKey();
    createUploadResource.object_.key = deviceKey;
    createUploadResource.pHeapProperties_.value = &heapPropertiesUpload;
    createUploadResource.HeapFlags_.value = D3D12_HEAP_FLAG_NONE;
    createUploadResource.pDesc_.value = &readbackResourceDesc;
    createUploadResource.InitialResourceState_.value = D3D12_RESOURCE_STATE_GENERIC_READ;
    createUploadResource.pOptimizedClearValue_.value = nullptr;
    createUploadResource.riidResource_.value = IID_ID3D12Resource;
    createUploadResource.ppvResource_.key = uploadResourceKey;
    stateService_.recorder_.record(
        new ID3D12DeviceCreateCommittedResourceWriter(createUploadResource));

    void* mappedData{};
    hr = readbackResource->Map(0, nullptr, &mappedData);

    ID3D12ResourceMapCommand mapCommand;
    mapCommand.key = stateService_.getUniqueCommandKey();
    mapCommand.object_.key = uploadResourceKey;
    mapCommand.Subresource_.value = 0;
    mapCommand.pReadRange_.value = nullptr;
    mapCommand.ppData_.captureValue = stateService_.getUniqueFakePointer();
    mapCommand.ppData_.value = &mapCommand.ppData_.captureValue;
    stateService_.recorder_.record(new ID3D12ResourceMapWriter(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.key = stateService_.getUniqueCommandKey();
    metaCommand.resource_.key = uploadResourceKey;
    metaCommand.mappedAddress_.value = mapCommand.ppData_.captureValue;
    metaCommand.offset_.value = 0;
    metaCommand.data_.value = mappedData;
    metaCommand.data_.size = readbackResourceDesc.Width;
    stateService_.recorder_.record(new MappedDataMetaWriter(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.key = stateService_.getUniqueCommandKey();
    unmapCommand.object_.key = uploadResourceKey;
    unmapCommand.Subresource_.value = 0;
    unmapCommand.pWrittenRange_.value = nullptr;
    stateService_.recorder_.record(new ID3D12ResourceUnmapWriter(unmapCommand));

    readbackResource->Unmap(0, nullptr);

    // restore resources contents from upload resource in subcaptured stream

    UINT64 offsetUpload = 0;
    for (unsigned subresourceIndex = 0; subresourceIndex < tiledResource->subresources.size();
         ++subresourceIndex) {
      if (subresourceFullyMappedFlags.at(subresourceIndex)) {
        if (tiledResource->desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
          ID3D12GraphicsCommandListCopyBufferRegionCommand copyBufferRegion;
          copyBufferRegion.key = stateService_.getUniqueCommandKey();
          copyBufferRegion.object_.key = commandListKey_;
          copyBufferRegion.pDstBuffer_.key = tiledResource->resourceKey;
          copyBufferRegion.DstOffset_.value = 0;
          copyBufferRegion.pSrcBuffer_.key = uploadResourceKey;
          copyBufferRegion.SrcOffset_.value = offsetUpload;
          copyBufferRegion.NumBytes_.value = subresourceSizes.at(subresourceIndex).first;
          stateService_.recorder_.record(
              new ID3D12GraphicsCommandListCopyBufferRegionWriter(copyBufferRegion));
        } else {
          D3D12_TEXTURE_COPY_LOCATION dest{};
          dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
          dest.SubresourceIndex = subresourceIndex;

          D3D12_TEXTURE_COPY_LOCATION src{};
          src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
          src.PlacedFootprint.Footprint = subresourceSizes.at(subresourceIndex).second.Footprint;
          src.PlacedFootprint.Offset = offsetUpload;

          ID3D12GraphicsCommandListCopyTextureRegionCommand copyTextureRegion;
          copyTextureRegion.key = stateService_.getUniqueCommandKey();
          copyTextureRegion.object_.key = commandListKey_;
          copyTextureRegion.pDst_.value = &dest;
          copyTextureRegion.pDst_.resourceKey = tiledResource->resourceKey;
          copyTextureRegion.pSrc_.value = &src;
          copyTextureRegion.pSrc_.resourceKey = uploadResourceKey;
          stateService_.recorder_.record(
              new ID3D12GraphicsCommandListCopyTextureRegionWriter(copyTextureRegion));
        }
        offsetUpload += subresourceSizes.at(subresourceIndex).first;
      } else if (tileRegions.count(subresourceIndex)) {
        for (TileRegion& region : tileRegions.at(subresourceIndex)) {
          GITS_ASSERT(!region.packed);
          ID3D12GraphicsCommandListCopyTilesCommand copyTiles;
          copyTiles.key = stateService_.getUniqueCommandKey();
          copyTiles.object_.key = commandListKey_;
          copyTiles.pTiledResource_.key = tiledResource->resourceKey;
          copyTiles.pTileRegionStartCoordinate_.value = &region.coord;
          copyTiles.pTileRegionSize_.value = &region.size;
          copyTiles.pBuffer_.key = uploadResourceKey;
          copyTiles.BufferStartOffsetInBytes_.value = offsetUpload;
          copyTiles.Flags_.value = D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE;
          stateService_.recorder_.record(new ID3D12GraphicsCommandListCopyTilesWriter(copyTiles));

          offsetUpload += region.size.NumTiles * tileSize;
        }
      }
    }

    ID3D12GraphicsCommandListCloseCommand commandListClose;
    commandListClose.key = stateService_.getUniqueCommandKey();
    commandListClose.object_.key = commandListKey_;
    stateService_.recorder_.record(new ID3D12GraphicsCommandListCloseWriter(commandListClose));

    // increase residency count or make resident

    if (!heapKeys.empty()) {
      ID3D12DeviceMakeResidentCommand makeResident;
      makeResident.key = stateService_.getUniqueCommandKey();
      makeResident.object_.key = deviceKey;
      makeResident.NumObjects_.value = heapKeys.size();
      ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
      makeResident.ppObjects_.value = &fakePtr;
      makeResident.ppObjects_.size = heapKeys.size();
      for (unsigned key : heapKeys) {
        makeResident.ppObjects_.keys.push_back(key);
      }
      stateService_.recorder_.record(new ID3D12DeviceMakeResidentWriter(makeResident));
    }

    ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
    executeCommandLists.key = stateService_.getUniqueCommandKey();
    executeCommandLists.object_.key = commandQueueKey_;
    executeCommandLists.NumCommandLists_.value = 1;
    executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
    executeCommandLists.ppCommandLists_.size = 1;
    executeCommandLists.ppCommandLists_.keys.resize(1);
    executeCommandLists.ppCommandLists_.keys[0] = commandListKey_;
    stateService_.recorder_.record(
        new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

    ID3D12CommandQueueSignalCommand commandQueueSignal;
    commandQueueSignal.key = stateService_.getUniqueCommandKey();
    commandQueueSignal.object_.key = commandQueueKey_;
    commandQueueSignal.pFence_.key = fenceKey_;
    commandQueueSignal.Value_.value = ++recordedFenceValue_;
    stateService_.recorder_.record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

    ID3D12FenceGetCompletedValueCommand getCompletedValue;
    getCompletedValue.key = stateService_.getUniqueCommandKey();
    getCompletedValue.object_.key = fenceKey_;
    getCompletedValue.result_.value = recordedFenceValue_;
    stateService_.recorder_.record(new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

    ID3D12CommandAllocatorResetCommand commandAllocatorReset;
    commandAllocatorReset.key = stateService_.getUniqueCommandKey();
    commandAllocatorReset.object_.key = commandAllocatorKey_;
    stateService_.recorder_.record(new ID3D12CommandAllocatorResetWriter(commandAllocatorReset));

    ID3D12GraphicsCommandListResetCommand commandListReset;
    commandListReset.key = stateService_.getUniqueCommandKey();
    commandListReset.object_.key = commandListKey_;
    commandListReset.pAllocator_.key = commandAllocatorKey_;
    commandListReset.pInitialState_.key = 0;
    stateService_.recorder_.record(new ID3D12GraphicsCommandListResetWriter(commandListReset));

    IUnknownReleaseCommand releaseCommand;
    releaseCommand.key = stateService_.getUniqueCommandKey();
    releaseCommand.object_.key = uploadResourceKey;
    stateService_.recorder_.record(new IUnknownReleaseWriter(releaseCommand));

    // decrese residency count or evict

    if (!heapKeys.empty()) {
      ID3D12DeviceEvictCommand evict;
      evict.key = stateService_.getUniqueCommandKey();
      evict.object_.key = deviceKey;
      evict.NumObjects_.value = heapKeys.size();
      ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
      evict.ppObjects_.value = &fakePtr;
      evict.ppObjects_.size = heapKeys.size();
      for (unsigned key : heapKeys) {
        evict.ppObjects_.keys.push_back(key);
      }
      stateService_.recorder_.record(new ID3D12DeviceEvictWriter(evict));
    }
  }

  cleanupRestore();
}

void ReservedResourcesService::initRestore() {

  D3D12_COMMAND_QUEUE_DESC commandQueueDirectDesc{};
  commandQueueDirectDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  HRESULT hr = device_->CreateCommandQueue(&commandQueueDirectDesc, IID_PPV_ARGS(&commandQueue_));
  GITS_ASSERT(hr == S_OK);
  hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       IID_PPV_ARGS(&commandAllocator_));
  GITS_ASSERT(hr == S_OK);
  hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_, nullptr,
                                  IID_PPV_ARGS(&commandList_));
  GITS_ASSERT(hr == S_OK);
  hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
  GITS_ASSERT(hr == S_OK);

  unsigned deviceKey = stateService_.deviceKey_;

  commandQueueKey_ = stateService_.getUniqueObjectKey();
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
  ID3D12DeviceCreateCommandQueueCommand createCommandQueue;
  createCommandQueue.key = stateService_.getUniqueCommandKey();
  createCommandQueue.object_.key = deviceKey;
  createCommandQueue.pDesc_.value = &commandQueueDesc;
  createCommandQueue.riid_.value = IID_ID3D12CommandQueue;
  createCommandQueue.ppCommandQueue_.key = commandQueueKey_;
  stateService_.recorder_.record(new ID3D12DeviceCreateCommandQueueWriter(createCommandQueue));

  commandAllocatorKey_ = stateService_.getUniqueObjectKey();
  ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
  createCommandAllocator.key = stateService_.getUniqueCommandKey();
  createCommandAllocator.object_.key = deviceKey;
  createCommandAllocator.type_.value = D3D12_COMMAND_LIST_TYPE_COPY;
  createCommandAllocator.riid_.value = IID_ID3D12CommandAllocator;
  createCommandAllocator.ppCommandAllocator_.key = commandAllocatorKey_;
  stateService_.recorder_.record(
      new ID3D12DeviceCreateCommandAllocatorWriter(createCommandAllocator));

  commandListKey_ = stateService_.getUniqueObjectKey();
  ID3D12DeviceCreateCommandListCommand createCommandList;
  createCommandList.key = stateService_.getUniqueCommandKey();
  createCommandList.object_.key = deviceKey;
  createCommandList.nodeMask_.value = 0;
  createCommandList.pCommandAllocator_.key = createCommandAllocator.ppCommandAllocator_.key;
  createCommandList.type_.value = D3D12_COMMAND_LIST_TYPE_COPY;
  createCommandList.pInitialState_.value = nullptr;
  createCommandList.riid_.value = IID_ID3D12CommandList;
  createCommandList.ppCommandList_.key = commandListKey_;
  stateService_.recorder_.record(new ID3D12DeviceCreateCommandListWriter(createCommandList));

  fenceKey_ = stateService_.getUniqueObjectKey();
  ID3D12DeviceCreateFenceCommand createFence;
  createFence.key = stateService_.getUniqueCommandKey();
  createFence.object_.key = deviceKey;
  createFence.InitialValue_.value = 0;
  createFence.Flags_.value = D3D12_FENCE_FLAG_NONE;
  createFence.riid_.value = IID_ID3D12Fence;
  createFence.ppFence_.key = fenceKey_;
  stateService_.recorder_.record(new ID3D12DeviceCreateFenceWriter(createFence));
}

void ReservedResourcesService::cleanupRestore() {
  device_->Release();
  commandQueue_->Release();
  commandAllocator_->Release();
  commandList_->Release();
  fence_->Release();

  IUnknownReleaseCommand releaseFence;
  releaseFence.key = stateService_.getUniqueCommandKey();
  releaseFence.object_.key = fenceKey_;
  stateService_.recorder_.record(new IUnknownReleaseWriter(releaseFence));

  IUnknownReleaseCommand releaseCommandList;
  releaseCommandList.key = stateService_.getUniqueCommandKey();
  releaseCommandList.object_.key = commandListKey_;
  stateService_.recorder_.record(new IUnknownReleaseWriter(releaseCommandList));

  IUnknownReleaseCommand releaseCommandAllocator;
  releaseCommandAllocator.key = stateService_.getUniqueCommandKey();
  releaseCommandAllocator.object_.key = commandAllocatorKey_;
  stateService_.recorder_.record(new IUnknownReleaseWriter(releaseCommandAllocator));

  IUnknownReleaseCommand releaseCommandQueue;
  releaseCommandQueue.key = stateService_.getUniqueCommandKey();
  releaseCommandQueue.object_.key = commandQueueKey_;
  stateService_.recorder_.record(new IUnknownReleaseWriter(releaseCommandQueue));
}

void ReservedResourcesService::getSubresourceSizes(
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
    device->GetCopyableFootprints(&desc, i, 1, 0, &footprint, nullptr, nullptr, &size);
    sizes[i].first = size;
  }
}

} // namespace DirectX
} // namespace gits
