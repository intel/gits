// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceContentRestore.h"
#include "stateTrackingService.h"
#include "commandsAuto.h"
#include "commandWritersAuto.h"
#include "commandsCustom.h"
#include "commandWritersCustom.h"
#include "config.h"

#include <limits>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

void ResourceContentRestore::restoreContent() {

  ID3D12Resource* resource{};
  if (!unmappableResourceBuffers_.empty()) {
    resource = unmappableResourceBuffers_[0].resource;
  } else if (!unmappableResourceTextures_.empty()) {
    resource = unmappableResourceTextures_[0].resource;
  }

  if (resource) {
    HRESULT hr = resource->GetDevice(IID_PPV_ARGS(&device_));
    GITS_ASSERT(hr == S_OK);

    initRestoreUnmappableResources();

    unsigned startIndex = 0;
    unsigned restored = 0;
    do {
      restored = restoreUnmappableResources(unmappableResourceBuffers_, startIndex);
      startIndex += restored;
    } while (restored);
    unmappableResourceBuffers_.clear();

    startIndex = 0;
    restored = 0;
    do {
      restored = restoreUnmappableResources(unmappableResourceTextures_, startIndex);
      startIndex += restored;
    } while (restored);
    unmappableResourceTextures_.clear();

    cleanupRestoreUnmappableResources();
  }

  restoreMappableResources();
  mappableResourceStates_.clear();
}

void ResourceContentRestore::restoreMappableResources() {
  for (ResourceState& state : mappableResourceStates_) {
    void* mappedData = nullptr;
    HRESULT hr = state.resource->Map(0, nullptr, &mappedData);
    GITS_ASSERT(hr == S_OK);

    ID3D12ResourceMapCommand mapCommand;
    mapCommand.key = stateService_.getUniqueCommandKey();
    mapCommand.object_.key = state.key;
    mapCommand.Subresource_.value = 0;
    mapCommand.pReadRange_.value = nullptr;
    mapCommand.ppData_.captureValue = stateService_.getUniqueFakePointer();
    mapCommand.ppData_.value = &mapCommand.ppData_.captureValue;
    stateService_.recorder_.record(new ID3D12ResourceMapWriter(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.key = stateService_.getUniqueCommandKey();
    metaCommand.resource_.key = state.key;
    metaCommand.mappedAddress_.value = mapCommand.ppData_.captureValue;
    metaCommand.offset_.value = 0;
    metaCommand.data_.value = mappedData;
    metaCommand.data_.size = state.resource->GetDesc().Width;
    stateService_.recorder_.record(new MappedDataMetaWriter(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.key = stateService_.getUniqueCommandKey();
    unmapCommand.object_.key = state.key;
    unmapCommand.Subresource_.value = 0;
    unmapCommand.pWrittenRange_.value = nullptr;
    stateService_.recorder_.record(new ID3D12ResourceUnmapWriter(unmapCommand));

    state.resource->Unmap(0, nullptr);
  }
}

unsigned ResourceContentRestore::restoreUnmappableResources(
    std::vector<ResourceState>& unmappableResourceStates, unsigned resourceStartIndex) {
  if (resourceStartIndex >= unmappableResourceStates.size()) {
    return 0;
  }

  // get resource sizes

  std::vector<std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>>> resourceSizes(
      unmappableResourceStates.size() - resourceStartIndex);
  for (unsigned i = 0; i < resourceSizes.size(); ++i) {
    ResourceState& state = unmappableResourceStates[i + resourceStartIndex];
    D3D12_RESOURCE_DESC desc = state.resource->GetDesc();
    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
      resourceSizes[i].resize(1);
      resourceSizes[i][0].first = desc.Width;
    } else {
      getSubresourceSizes(device_, desc, resourceSizes[i]);
    }
  }

  const UINT64 maxChunkSize = 0x100000;
  unsigned resourcesCount = 0;
  UINT64 totalSize = 0;
  UINT64 totalSizeRestored = 0;
  for (unsigned i = 0; i < resourceSizes.size(); ++i) {
    for (auto& entry : resourceSizes[i]) {
      totalSize += getAlignedSize(entry.first);
    }
    if (totalSize > maxChunkSize) {
      if (!resourcesCount) {
        totalSizeRestored = totalSize;
        ++resourcesCount;
      }
      break;
    }
    totalSizeRestored = totalSize;
    ++resourcesCount;
  }

  // make resources resident

  std::vector<unsigned> residencyKeys;
  std::vector<ID3D12Pageable*> residencyObjects;
  for (unsigned i = 0; i < resourcesCount; ++i) {
    ResourceState& state = unmappableResourceStates[i + resourceStartIndex];
    if (state.heapKey) {
      ObjectState* heapState = stateService_.getState(state.heapKey);
      residencyKeys.push_back(state.heapKey);
      residencyObjects.push_back(static_cast<ID3D12Pageable*>(heapState->object));
    } else {
      residencyKeys.push_back(state.key);
      residencyObjects.push_back(state.resource);
    }
  }

  if (!residencyKeys.empty()) {
    Microsoft::WRL::ComPtr<ID3D12Device3> device3;
    HRESULT hr = device_->QueryInterface(IID_PPV_ARGS(&device3));
    GITS_ASSERT(hr == S_OK);
    hr = device3->EnqueueMakeResident(D3D12_RESIDENCY_FLAG_NONE, residencyObjects.size(),
                                      residencyObjects.data(), fence_, ++currentFenceValue_);
    GITS_ASSERT(hr == S_OK);
    while (fence_->GetCompletedValue() < currentFenceValue_) {
    }
  }

  // copy resources contents into readback resource

  D3D12_HEAP_PROPERTIES heapPropertiesReadback{};
  heapPropertiesReadback.Type = D3D12_HEAP_TYPE_READBACK;
  heapPropertiesReadback.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapPropertiesReadback.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapPropertiesReadback.CreationNodeMask = 1;
  heapPropertiesReadback.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = totalSizeRestored;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  Microsoft::WRL::ComPtr<ID3D12Resource> readbackResource;
  HRESULT hr = device_->CreateCommittedResource(&heapPropertiesReadback, D3D12_HEAP_FLAG_NONE,
                                                &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                                                nullptr, IID_PPV_ARGS(&readbackResource));
  GITS_ASSERT(hr == S_OK);

  {
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    for (unsigned resourceIndex = 0; resourceIndex < resourcesCount; ++resourceIndex) {
      ResourceState& state = unmappableResourceStates[resourceIndex + resourceStartIndex];
      ResourceStateTrackingService::ResourceStates& resourceStates =
          stateService_.resourceStateTrackingService_.getResourceStates(state.key);
      if (resourceStates.allEqual) {
        if (resourceStates.subresourceStates[0] != D3D12_RESOURCE_STATE_COPY_SOURCE) {
          D3D12_RESOURCE_BARRIER barrier{};
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          barrier.Transition.pResource = state.resource;
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
            barrier.Transition.pResource = state.resource;
            barrier.Transition.Subresource = i;
            barrier.Transition.StateBefore = resourceStates.subresourceStates[i];
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers.push_back(barrier);
          }
        }
      }
    }
    if (!barriers.empty()) {
      commandList_->ResourceBarrier(barriers.size(), barriers.data());
    }
  }

  unsigned offsetReadback = 0;
  for (unsigned resourceIndex = 0; resourceIndex < resourcesCount; ++resourceIndex) {
    ResourceState& state = unmappableResourceStates[resourceIndex + resourceStartIndex];
    D3D12_RESOURCE_DESC desc = state.resource->GetDesc();

    for (unsigned subresourceIndex = 0; subresourceIndex < resourceSizes[resourceIndex].size();
         ++subresourceIndex) {
      unsigned subresourceSize = resourceSizes[resourceIndex][subresourceIndex].first;
      if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        commandList_->CopyBufferRegion(readbackResource.Get(), offsetReadback, state.resource, 0,
                                       subresourceSize);
      } else {
        D3D12_TEXTURE_COPY_LOCATION dest{};
        dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dest.pResource = readbackResource.Get();
        dest.PlacedFootprint.Footprint =
            resourceSizes[resourceIndex][subresourceIndex].second.Footprint;
        dest.PlacedFootprint.Offset = offsetReadback;
        D3D12_TEXTURE_COPY_LOCATION src{};
        src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.pResource = state.resource;
        src.SubresourceIndex = subresourceIndex;
        commandList_->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
      }
      offsetReadback += getAlignedSize(subresourceSize);
    }
  }

  {
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    for (unsigned resourceIndex = 0; resourceIndex < resourcesCount; ++resourceIndex) {
      ResourceState& state = unmappableResourceStates[resourceIndex + resourceStartIndex];
      ResourceStateTrackingService::ResourceStates& resourceStates =
          stateService_.resourceStateTrackingService_.getResourceStates(state.key);
      if (resourceStates.allEqual) {
        if (resourceStates.subresourceStates[0] != D3D12_RESOURCE_STATE_COPY_SOURCE) {
          D3D12_RESOURCE_BARRIER barrier{};
          barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          barrier.Transition.pResource = state.resource;
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
            barrier.Transition.pResource = state.resource;
            barrier.Transition.Subresource = i;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barrier.Transition.StateAfter = resourceStates.subresourceStates[i];
            barriers.push_back(barrier);
          }
        }
      }
    }
    if (!barriers.empty()) {
      commandList_->ResourceBarrier(barriers.size(), barriers.data());
    }
  }

  commandList_->Close();

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

  if (!residencyKeys.empty()) {
    HRESULT hr = device_->Evict(residencyObjects.size(), residencyObjects.data());
    GITS_ASSERT(hr == S_OK);
  }

  // create upload resource with resources contents in subcaptured stream

  unsigned deviceKey = unmappableResourceStates[0].deviceKey;
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
  createUploadResource.pDesc_.value = &resourceDesc;
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
  metaCommand.data_.size = resourceDesc.Width;
  stateService_.recorder_.record(new MappedDataMetaWriter(metaCommand));

  ID3D12ResourceUnmapCommand unmapCommand;
  unmapCommand.key = stateService_.getUniqueCommandKey();
  unmapCommand.object_.key = uploadResourceKey;
  unmapCommand.Subresource_.value = 0;
  unmapCommand.pWrittenRange_.value = nullptr;
  stateService_.recorder_.record(new ID3D12ResourceUnmapWriter(unmapCommand));

  readbackResource->Unmap(0, nullptr);

  // make resources resident

  if (!residencyKeys.empty()) {
    ID3D12Device3EnqueueMakeResidentCommand makeResident;
    makeResident.key = stateService_.getUniqueCommandKey();
    makeResident.object_.key = deviceKey;
    makeResident.Flags_.value = D3D12_RESIDENCY_FLAG_NONE;
    makeResident.NumObjects_.value = residencyKeys.size();
    ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
    makeResident.ppObjects_.value = &fakePtr;
    makeResident.ppObjects_.size = residencyKeys.size();
    for (unsigned key : residencyKeys) {
      makeResident.ppObjects_.keys.push_back(key);
    }
    makeResident.pFenceToSignal_.key = fenceKey_;
    makeResident.FenceValueToSignal_.value = ++recordedFenceValue_;
    stateService_.recorder_.record(new ID3D12Device3EnqueueMakeResidentWriter(makeResident));

    ID3D12FenceGetCompletedValueCommand getCompletedValue;
    getCompletedValue.key = stateService_.getUniqueCommandKey();
    getCompletedValue.object_.key = fenceKey_;
    getCompletedValue.result_.value = recordedFenceValue_;
    stateService_.recorder_.record(new ID3D12FenceGetCompletedValueWriter(getCompletedValue));
  }

  // restore resources contents from upload resource in subcaptured stream

  unsigned offsetUpload = 0;
  for (unsigned resourceIndex = 0; resourceIndex < resourcesCount; ++resourceIndex) {
    ResourceState& state = unmappableResourceStates[resourceIndex + resourceStartIndex];
    D3D12_RESOURCE_DESC desc = state.resource->GetDesc();
    for (unsigned subresourceIndex = 0; subresourceIndex < resourceSizes[resourceIndex].size();
         ++subresourceIndex) {
      unsigned subresourceSize = resourceSizes[resourceIndex][subresourceIndex].first;
      if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {

        ID3D12GraphicsCommandListCopyBufferRegionCommand copyBufferRegion;
        copyBufferRegion.key = stateService_.getUniqueCommandKey();
        copyBufferRegion.object_.key = commandListKey_;
        copyBufferRegion.pDstBuffer_.key = state.key;
        copyBufferRegion.DstOffset_.value = 0;
        copyBufferRegion.pSrcBuffer_.key = uploadResourceKey;
        copyBufferRegion.SrcOffset_.value = offsetUpload;
        copyBufferRegion.NumBytes_.value = subresourceSize;
        stateService_.recorder_.record(
            new ID3D12GraphicsCommandListCopyBufferRegionWriter(copyBufferRegion));
      } else {

        D3D12_TEXTURE_COPY_LOCATION dest{};
        dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dest.SubresourceIndex = subresourceIndex;

        D3D12_TEXTURE_COPY_LOCATION src{};
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Footprint =
            resourceSizes[resourceIndex][subresourceIndex].second.Footprint;
        src.PlacedFootprint.Offset = offsetUpload;

        ID3D12GraphicsCommandListCopyTextureRegionCommand copyTextureRegion;
        copyTextureRegion.key = stateService_.getUniqueCommandKey();
        copyTextureRegion.object_.key = commandListKey_;
        copyTextureRegion.pDst_.value = &dest;
        copyTextureRegion.pDst_.resourceKey = state.key;
        copyTextureRegion.pSrc_.value = &src;
        copyTextureRegion.pSrc_.resourceKey = uploadResourceKey;
        stateService_.recorder_.record(
            new ID3D12GraphicsCommandListCopyTextureRegionWriter(copyTextureRegion));
      }
      offsetUpload += getAlignedSize(subresourceSize);
    }
  }

  ID3D12GraphicsCommandListCloseCommand commandListClose;
  commandListClose.key = stateService_.getUniqueCommandKey();
  commandListClose.object_.key = commandListKey_;
  stateService_.recorder_.record(new ID3D12GraphicsCommandListCloseWriter(commandListClose));

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

  if (!residencyKeys.empty()) {
    ID3D12DeviceEvictCommand evict;
    evict.key = stateService_.getUniqueCommandKey();
    evict.object_.key = deviceKey;
    evict.NumObjects_.value = residencyKeys.size();
    ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
    evict.ppObjects_.value = &fakePtr;
    evict.ppObjects_.size = residencyKeys.size();
    for (unsigned key : residencyKeys) {
      evict.ppObjects_.keys.push_back(key);
    }
    stateService_.recorder_.record(new ID3D12DeviceEvictWriter(evict));
  }

  return resourcesCount;
}

void ResourceContentRestore::getSubresourceSizes(
    ID3D12Device* device,
    D3D12_RESOURCE_DESC& desc_,
    std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>>& sizes) {

  D3D12_RESOURCE_DESC desc = desc_;
  desc.Flags = D3D12_RESOURCE_FLAG_NONE;

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

void ResourceContentRestore::initRestoreUnmappableResources() {

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

void ResourceContentRestore::cleanupRestoreUnmappableResources() {
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

UINT64 ResourceContentRestore::getAlignedSize(UINT64 size) {
  return size % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT == 0
             ? size
             : ((size / D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) + 1) *
                   D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
}

} // namespace DirectX
} // namespace gits
