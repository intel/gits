// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceContentRestore.h"
#include "resourceStateTrackingService.h"
#include "stateTrackingService.h"
#include "commandsAuto.h"
#include "commandSerializersAuto.h"
#include "commandsCustom.h"
#include "commandSerializersCustom.h"
#include "resourceSizeUtils.h"

#include <limits>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

void ResourceContentRestore::AddCommittedResourceState(ResourceState* resourceState) {
  if (resourceState->InitialState == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    return;
  }
  if (resourceState->SampleCount > 1) {
    return;
  }

  ResourceInfo state{static_cast<ID3D12Resource*>(resourceState->Object), resourceState->Key, 0};
  if (resourceState->IsMappable) {
    m_MappableResourceStates[state.Key] = state;
  } else if (resourceState->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    m_UnmappableResourceBuffers[state.Key] = state;
  } else {
    m_UnmappableResourceTextures[state.Key] = state;
  }
}

void ResourceContentRestore::AddPlacedResourceState(ResourceState* resourceState) {
  if (resourceState->InitialState == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    return;
  }
  if (resourceState->SampleCount > 1) {
    return;
  }

  ResourceInfo state{static_cast<ID3D12Resource*>(resourceState->Object), resourceState->Key,
                     resourceState->HeapKey};
  if (resourceState->IsMappable) {
    m_MappableResourceStates[state.Key] = state;
  } else if (resourceState->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    m_UnmappableResourceBuffers[state.Key] = state;
  } else {
    m_UnmappableResourceTextures[state.Key] = state;
  }
}

void ResourceContentRestore::RestoreContent(const std::vector<unsigned>& ResourceKeys,
                                            bool backBuffer) {
  enum ResourceBatchType {
    MappableResource,
    UnmappableResourceBuffer,
    UnmappableResourceTexture,
  };

  std::vector<std::pair<ResourceBatchType, std::vector<ResourceInfo>>> batches;

  // prepare batches
  {
    ResourceBatchType prevType{};

    for (unsigned ResourceKey : ResourceKeys) {
      ResourceBatchType type{};
      ResourceInfo resourceInfo{};
      if (m_MappableResourceStates.find(ResourceKey) != m_MappableResourceStates.end()) {
        type = MappableResource;
        resourceInfo = m_MappableResourceStates[ResourceKey];
      } else if (m_UnmappableResourceBuffers.find(ResourceKey) !=
                 m_UnmappableResourceBuffers.end()) {
        type = UnmappableResourceBuffer;
        resourceInfo = m_UnmappableResourceBuffers[ResourceKey];
      } else if (m_UnmappableResourceTextures.find(ResourceKey) !=
                 m_UnmappableResourceTextures.end()) {
        type = UnmappableResourceTexture;
        resourceInfo = m_UnmappableResourceTextures[ResourceKey];
      } else {
        continue;
      }

      if (batches.empty() || type != prevType) {
        batches.push_back({type, {resourceInfo}});
      } else {
        batches.back().second.push_back(resourceInfo);
      }

      prevType = type;
    }
  }

  // restore batches
  {
    auto restoreUnmappable = [this](std::vector<ResourceInfo>& resourceInfos, size_t batchSize) {
      unsigned startIndex = 0;
      unsigned restored = 0;
      do {
        restored = RestoreUnmappableResources(resourceInfos, startIndex, batchSize);
        startIndex += restored;
      } while (restored);
    };

    for (auto& [type, resourceInfos] : batches) {
      if (type == MappableResource) {
        RestoreMappableResources(resourceInfos);
      } else {
        if (!m_RestoreUnmappableResourcesInitialized) {
          ID3D12Resource* resource = resourceInfos[0].Resource;
          HRESULT hr = resource->GetDevice(IID_PPV_ARGS(&m_Device));
          GITS_ASSERT(hr == S_OK);
          InitRestoreUnmappableResources(backBuffer);
        }

        if (type == UnmappableResourceBuffer) {
          restoreUnmappable(resourceInfos, m_BuffersMaxBatchSize);
        } else if (type == UnmappableResourceTexture) {
          restoreUnmappable(resourceInfos, m_TexturesMaxBatchSize);
        }
      }
    }
  }
}

void ResourceContentRestore::RestoreMappableResources(
    const std::vector<ResourceInfo>& resourceInfos) {
  for (const ResourceInfo& state : resourceInfos) {
    void* mappedData = nullptr;
    HRESULT hr = state.Resource->Map(0, nullptr, &mappedData);
    GITS_ASSERT(hr == S_OK);

    ID3D12ResourceMapCommand mapCommand;
    mapCommand.Key = m_StateService.GetUniqueCommandKey();
    mapCommand.m_Object.Key = state.Key;
    mapCommand.m_Subresource.Value = 0;
    mapCommand.m_pReadRange.Value = nullptr;
    mapCommand.m_ppData.CaptureValue = m_StateService.GetUniqueFakePointer();
    mapCommand.m_ppData.Value = &mapCommand.m_ppData.CaptureValue;
    m_StateService.GetRecorder().Record(ID3D12ResourceMapSerializer(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.Key = m_StateService.GetUniqueCommandKey();
    metaCommand.m_resource.Key = state.Key;
    metaCommand.m_mappedAddress.Value = mapCommand.m_ppData.CaptureValue;
    metaCommand.m_offset.Value = 0;
    metaCommand.m_data.Value = mappedData;
    metaCommand.m_data.Size = state.Resource->GetDesc().Width;
    m_StateService.GetRecorder().Record(MappedDataMetaSerializer(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.Key = m_StateService.GetUniqueCommandKey();
    unmapCommand.m_Object.Key = state.Key;
    unmapCommand.m_Subresource.Value = 0;
    unmapCommand.m_pWrittenRange.Value = nullptr;
    m_StateService.GetRecorder().Record(ID3D12ResourceUnmapSerializer(unmapCommand));

    state.Resource->Unmap(0, nullptr);
  }
}

unsigned ResourceContentRestore::RestoreUnmappableResources(
    std::vector<ResourceInfo>& unmappableResourceStates,
    unsigned resourceStartIndex,
    UINT64 maxBatchSize) {
  if (resourceStartIndex >= unmappableResourceStates.size()) {
    return 0;
  }

  // get resource sizes

  std::vector<std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>>> resourceSizes(
      unmappableResourceStates.size() - resourceStartIndex);
  for (unsigned i = 0; i < resourceSizes.size(); ++i) {
    ResourceInfo& state = unmappableResourceStates[i + resourceStartIndex];
    D3D12_RESOURCE_DESC desc = state.Resource->GetDesc();
    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
      resourceSizes[i].resize(1);
      resourceSizes[i][0].first = desc.Width;
    } else {
      GetSubresourceSizes(m_Device, desc, resourceSizes[i]);
    }
  }

  unsigned resourcesCount = 0;
  UINT64 totalSize = 0;
  UINT64 totalSizeRestored = 0;
  for (unsigned i = 0; i < resourceSizes.size(); ++i) {
    for (auto& entry : resourceSizes[i]) {
      totalSize += GetAlignedSize(entry.first);
    }
    if (totalSize > maxBatchSize) {
      if (!resourcesCount) {
        totalSizeRestored = totalSize;
        ++resourcesCount;
      }
      break;
    }
    totalSizeRestored = totalSize;
    ++resourcesCount;
  }

  GITS_ASSERT(totalSizeRestored <= m_UploadResourceSize);

  // make resources resident if residency changed

  std::vector<unsigned> residencyKeys;
  std::vector<ID3D12Pageable*> residencyObjects;
  {
    std::set<unsigned> residencyKeysUnique;
    std::set<ID3D12Pageable*> residencyObjectsUnique;
    for (unsigned i = 0; i < resourcesCount; ++i) {
      ResourceInfo& state = unmappableResourceStates[i + resourceStartIndex];
      if (state.HeapKey) {
        ObjectState* heapState = m_StateService.GetState(state.HeapKey);
        residencyKeysUnique.insert(state.HeapKey);
        residencyObjectsUnique.insert(static_cast<ID3D12Pageable*>(heapState->Object));
      } else {
        residencyKeysUnique.insert(state.Key);
        residencyObjectsUnique.insert(state.Resource);
      }
    }

    if (residencyKeysUnique != m_PrevResidencyKeys) {
      residencyKeys = std::vector<unsigned>(residencyKeysUnique.begin(), residencyKeysUnique.end());
      residencyObjects = std::vector<ID3D12Pageable*>(residencyObjectsUnique.begin(),
                                                      residencyObjectsUnique.end());
      EvictPrevResidencyObjects();
      m_PrevResidencyKeys = std::move(residencyKeysUnique);
      m_PrevResidencyObjects = std::move(residencyObjectsUnique);
    }
  }

  if (!residencyKeys.empty()) {
    HRESULT hr = m_Device->MakeResident(residencyObjects.size(), residencyObjects.data());
    GITS_ASSERT(hr == S_OK);
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
  HRESULT hr = m_Device->CreateCommittedResource(&heapPropertiesReadback, D3D12_HEAP_FLAG_NONE,
                                                 &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                                                 nullptr, IID_PPV_ARGS(&readbackResource));
  GITS_ASSERT(hr == S_OK);

  for (unsigned resourceIndex = 0; resourceIndex < resourcesCount; ++resourceIndex) {
    ResourceInfo& state = unmappableResourceStates[resourceIndex + resourceStartIndex];
    if (IsBarrierRestricted(state.Key)) {
      continue;
    }
    CopySourceBarrier(state, false);
  }

  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> auxiliaryPlacedResources;
  unsigned offsetReadback = 0;
  for (unsigned resourceIndex = 0; resourceIndex < resourcesCount; ++resourceIndex) {
    ResourceInfo& state = unmappableResourceStates[resourceIndex + resourceStartIndex];
    ID3D12Resource* resource = state.Resource;
    if (IsBarrierRestricted(state.Key)) {
      auxiliaryPlacedResources.emplace_back(CreateAuxiliaryPlacedResource(state.Key));
      resource = auxiliaryPlacedResources.back().Get();
    }

    D3D12_RESOURCE_DESC desc = resource->GetDesc();
    for (unsigned subresourceIndex = 0; subresourceIndex < resourceSizes[resourceIndex].size();
         ++subresourceIndex) {
      unsigned subresourceSize = resourceSizes[resourceIndex][subresourceIndex].first;
      if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        m_CommandList->CopyBufferRegion(readbackResource.Get(), offsetReadback, resource, 0,
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
        src.pResource = resource;
        src.SubresourceIndex = subresourceIndex;
        m_CommandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
      }
      offsetReadback += GetAlignedSize(subresourceSize);
    }
  }

  for (unsigned resourceIndex = 0; resourceIndex < resourcesCount; ++resourceIndex) {
    ResourceInfo& state = unmappableResourceStates[resourceIndex + resourceStartIndex];
    if (IsBarrierRestricted(state.Key)) {
      continue;
    }
    CopySourceBarrier(state, true);
  }

  m_CommandList->Close();

  ID3D12CommandList* commandLists[] = {m_CommandList};
  m_CommandQueue->ExecuteCommandLists(1, commandLists);
  m_CommandQueue->Signal(m_Fence, ++m_CurrentFenceValue);
  while (m_Fence->GetCompletedValue() < m_CurrentFenceValue) {
  }

  hr = m_CommandAllocator->Reset();
  GITS_ASSERT(hr == S_OK);
  hr = m_CommandList->Reset(m_CommandAllocator, nullptr);
  GITS_ASSERT(hr == S_OK);

  // release auxiliary placed resources

  auxiliaryPlacedResources.clear();

  // create upload resource with resources contents in subcaptured stream

  unsigned DeviceKey = m_StateService.GetDeviceKey();

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
  metaCommand.m_data.Size = resourceDesc.Width;
  m_StateService.GetRecorder().Record(MappedDataMetaSerializer(metaCommand));

  ID3D12ResourceUnmapCommand unmapCommand;
  unmapCommand.Key = m_StateService.GetUniqueCommandKey();
  unmapCommand.m_Object.Key = m_UploadResourceKey;
  unmapCommand.m_Subresource.Value = 0;
  unmapCommand.m_pWrittenRange.Value = nullptr;
  m_StateService.GetRecorder().Record(ID3D12ResourceUnmapSerializer(unmapCommand));

  readbackResource->Unmap(0, nullptr);

  // make resources resident

  if (!residencyKeys.empty()) {
    ID3D12DeviceMakeResidentCommand MakeResident;
    MakeResident.Key = m_StateService.GetUniqueCommandKey();
    MakeResident.m_Object.Key = DeviceKey;
    MakeResident.m_NumObjects.Value = static_cast<UINT>(residencyKeys.size());
    ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
    MakeResident.m_ppObjects.Value = &fakePtr;
    MakeResident.m_ppObjects.Size = residencyKeys.size();
    for (unsigned key : residencyKeys) {
      MakeResident.m_ppObjects.Keys.push_back(key);
    }
    m_StateService.GetRecorder().Record(ID3D12DeviceMakeResidentSerializer(MakeResident));
  }

  // restore resources contents from upload resource in subcaptured stream

  unsigned offsetUpload = 0;
  std::vector<unsigned> auxiliaryPlacedResourceKeys;
  for (unsigned resourceIndex = 0; resourceIndex < resourcesCount; ++resourceIndex) {
    ResourceInfo& state = unmappableResourceStates[resourceIndex + resourceStartIndex];
    D3D12_RESOURCE_DESC desc = state.Resource->GetDesc();
    unsigned ResourceKey = state.Key;
    if (IsBarrierRestricted(state.Key)) {
      ResourceKey = CreateSubcaptureAuxiliaryPlacedResource(state.Key);
      auxiliaryPlacedResourceKeys.push_back(ResourceKey);
    }

    for (unsigned subresourceIndex = 0; subresourceIndex < resourceSizes[resourceIndex].size();
         ++subresourceIndex) {
      unsigned subresourceSize = resourceSizes[resourceIndex][subresourceIndex].first;
      if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {

        ID3D12GraphicsCommandListCopyBufferRegionCommand copyBufferRegion;
        copyBufferRegion.Key = m_StateService.GetUniqueCommandKey();
        copyBufferRegion.m_Object.Key = m_CommandListKey;
        copyBufferRegion.m_pDstBuffer.Key = ResourceKey;
        copyBufferRegion.m_DstOffset.Value = 0;
        copyBufferRegion.m_pSrcBuffer.Key = m_UploadResourceKey;
        copyBufferRegion.m_SrcOffset.Value = offsetUpload;
        copyBufferRegion.m_NumBytes.Value = subresourceSize;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListCopyBufferRegionSerializer(copyBufferRegion));
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
        copyTextureRegion.Key = m_StateService.GetUniqueCommandKey();
        copyTextureRegion.m_Object.Key = m_CommandListKey;
        copyTextureRegion.m_pDst.Value = &dest;
        copyTextureRegion.m_pDst.ResourceKey = ResourceKey;
        copyTextureRegion.m_pSrc.Value = &src;
        copyTextureRegion.m_pSrc.ResourceKey = m_UploadResourceKey;
        m_StateService.GetRecorder().Record(
            ID3D12GraphicsCommandListCopyTextureRegionSerializer(copyTextureRegion));
      }

      offsetUpload += GetAlignedSize(subresourceSize);
    }
  }

  ID3D12GraphicsCommandListCloseCommand commandListClose;
  commandListClose.Key = m_StateService.GetUniqueCommandKey();
  commandListClose.m_Object.Key = m_CommandListKey;
  m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListCloseSerializer(commandListClose));

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
  m_StateService.GetRecorder().Record(ID3D12CommandAllocatorResetSerializer(commandAllocatorReset));

  ID3D12GraphicsCommandListResetCommand CommandListReset;
  CommandListReset.Key = m_StateService.GetUniqueCommandKey();
  CommandListReset.m_Object.Key = m_CommandListKey;
  CommandListReset.m_pAllocator.Key = m_CommandAllocatorKey;
  CommandListReset.m_pInitialState.Key = 0;
  m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListResetSerializer(CommandListReset));

  for (const auto key : auxiliaryPlacedResourceKeys) {
    IUnknownReleaseCommand releaseCommand;
    releaseCommand.Key = m_StateService.GetUniqueCommandKey();
    releaseCommand.m_Object.Key = key;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommand));
  }

  return resourcesCount;
}

void ResourceContentRestore::GetSubresourceSizes(
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
    GetCopyableFootprintsSafe(device, &desc, i, 1, 0, &footprint, nullptr, nullptr, &size);
    sizes[i].first = size;
  }
}

void ResourceContentRestore::InitRestoreUnmappableResources(bool backBuffer) {
  if (m_RestoreUnmappableResourcesInitialized) {
    return;
  }

  size_t maxUploadSize = std::max(m_BuffersMaxBatchSize, m_TexturesMaxBatchSize);
  for (const auto& textureInfo : m_UnmappableResourceTextures) {
    std::vector<std::pair<unsigned, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>> subresourceSizes;
    D3D12_RESOURCE_DESC desc = textureInfo.second.Resource->GetDesc();
    GetSubresourceSizes(m_Device, desc, subresourceSizes);
    size_t uploadSize = 0;
    for (const auto& subresourceSize : subresourceSizes) {
      uploadSize += GetAlignedSize(subresourceSize.first);
    }
    if (uploadSize > maxUploadSize) {
      maxUploadSize = uploadSize;
    }
  }

  for (const auto& bufferInfo : m_UnmappableResourceBuffers) {
    D3D12_RESOURCE_DESC desc = bufferInfo.second.Resource->GetDesc();
    GITS_ASSERT(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
    size_t size = GetAlignedSize(desc.Width);
    if (size > maxUploadSize) {
      maxUploadSize = size;
    }
  }

  m_UploadResourceSize = maxUploadSize;
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

  unsigned DeviceKey = m_StateService.GetDeviceKey();

  ID3D12DeviceCreateCommittedResourceCommand createUploadResource;
  createUploadResource.Key = m_StateService.GetUniqueCommandKey();
  createUploadResource.m_Object.Key = DeviceKey;
  createUploadResource.m_pHeapProperties.Value = &heapPropertiesUpload;
  createUploadResource.m_HeapFlags.Value = D3D12_HEAP_FLAG_NONE;
  createUploadResource.m_pDesc.Value = &resourceDesc;
  createUploadResource.m_InitialResourceState.Value = D3D12_RESOURCE_STATE_GENERIC_READ;
  createUploadResource.m_pOptimizedClearValue.Value = nullptr;
  createUploadResource.m_riidResource.Value = IID_ID3D12Resource;
  createUploadResource.m_ppvResource.Key = m_UploadResourceKey;
  m_StateService.GetRecorder().Record(
      ID3D12DeviceCreateCommittedResourceSerializer(createUploadResource));

  HRESULT hr{};
  if (!backBuffer) {
    D3D12_COMMAND_QUEUE_DESC commandQueueDirectDesc{};
    commandQueueDirectDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = m_Device->CreateCommandQueue(&commandQueueDirectDesc, IID_PPV_ARGS(&m_CommandQueue));
    GITS_ASSERT(hr == S_OK);
  }
  hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                        IID_PPV_ARGS(&m_CommandAllocator));
  GITS_ASSERT(hr == S_OK);
  hr = m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator, nullptr,
                                   IID_PPV_ARGS(&m_CommandList));
  GITS_ASSERT(hr == S_OK);
  hr = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
  GITS_ASSERT(hr == S_OK);

  if (!backBuffer) {
    m_CommandQueueKey = m_StateService.GetUniqueObjectKey();
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    ID3D12DeviceCreateCommandQueueCommand CreateCommandQueue;
    CreateCommandQueue.Key = m_StateService.GetUniqueCommandKey();
    CreateCommandQueue.m_Object.Key = DeviceKey;
    CreateCommandQueue.m_pDesc.Value = &commandQueueDesc;
    CreateCommandQueue.m_riid.Value = IID_ID3D12CommandQueue;
    CreateCommandQueue.m_ppCommandQueue.Key = m_CommandQueueKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandQueueSerializer(CreateCommandQueue));
  }

  m_CommandAllocatorKey = m_StateService.GetUniqueObjectKey();
  ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
  createCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
  createCommandAllocator.m_Object.Key = DeviceKey;
  createCommandAllocator.m_type.Value = D3D12_COMMAND_LIST_TYPE_COPY;
  createCommandAllocator.m_riid.Value = IID_ID3D12CommandAllocator;
  createCommandAllocator.m_ppCommandAllocator.Key = m_CommandAllocatorKey;
  m_StateService.GetRecorder().Record(
      ID3D12DeviceCreateCommandAllocatorSerializer(createCommandAllocator));

  m_CommandListKey = m_StateService.GetUniqueObjectKey();
  ID3D12DeviceCreateCommandListCommand createCommandList;
  createCommandList.Key = m_StateService.GetUniqueCommandKey();
  createCommandList.m_Object.Key = DeviceKey;
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
  createFence.m_Object.Key = DeviceKey;
  createFence.m_InitialValue.Value = 0;
  createFence.m_Flags.Value = D3D12_FENCE_FLAG_NONE;
  createFence.m_riid.Value = IID_ID3D12Fence;
  createFence.m_ppFence.Key = m_FenceKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateFenceSerializer(createFence));

  m_RestoreUnmappableResourcesInitialized = true;
}

void ResourceContentRestore::CleanupRestoreUnmappableResources() {
  if (!m_RestoreUnmappableResourcesInitialized) {
    return;
  }

  EvictPrevResidencyObjects();
  m_PrevResidencyKeys.clear();
  m_PrevResidencyObjects.clear();

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

  IUnknownReleaseCommand releaseUploadResource;
  releaseUploadResource.Key = m_StateService.GetUniqueCommandKey();
  releaseUploadResource.m_Object.Key = m_UploadResourceKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseUploadResource));

  m_RestoreUnmappableResourcesInitialized = false;
}

void ResourceContentRestore::RestoreBackBuffer(ID3D12CommandQueue* commandQueue,
                                               unsigned commandQueueKey,
                                               unsigned ResourceKey,
                                               ID3D12Resource* resource) {
  m_CommandQueue = commandQueue;
  m_CommandQueueKey = commandQueueKey;

  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    m_UnmappableResourceBuffers[ResourceKey] = ResourceInfo{resource, ResourceKey};
  } else {
    m_UnmappableResourceTextures[ResourceKey] = ResourceInfo{resource, ResourceKey};
  }

  std::vector<unsigned> ResourceKeys;
  ResourceKeys.push_back(ResourceKey);
  RestoreContent(ResourceKeys, true);
}

UINT64 ResourceContentRestore::GetAlignedSize(UINT64 size) {
  return size % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT == 0
             ? size
             : ((size / D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) + 1) *
                   D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
}

bool ResourceContentRestore::IsBarrierRestricted(unsigned ResourceKey) {
  ObjectState* resourceObjectState = m_StateService.GetState(ResourceKey);
  GITS_ASSERT(resourceObjectState);
  return static_cast<ResourceState*>(resourceObjectState)->BarrierRestricted;
}

ID3D12Resource* ResourceContentRestore::CreateAuxiliaryPlacedResource(unsigned primaryResourceKey) {
  ObjectState* resourceObjectState = m_StateService.GetState(primaryResourceKey);
  ID3D12Resource* auxiliaryResource{};
  if (resourceObjectState->CreationCommand->GetId() ==
      CommandId::ID_ID3D12DEVICE_CREATEPLACEDRESOURCE) {
    auto* Command = static_cast<ID3D12DeviceCreatePlacedResourceCommand*>(
        resourceObjectState->CreationCommand.get());
    HRESULT hr = m_Device->CreatePlacedResource(
        Command->m_pHeap.Value, Command->m_HeapOffset.Value, Command->m_pDesc.Value,
        D3D12_RESOURCE_STATE_COPY_SOURCE, Command->m_pOptimizedClearValue.Value,
        IID_PPV_ARGS(&auxiliaryResource));
    GITS_ASSERT(hr == S_OK);
  } else if (resourceObjectState->CreationCommand->GetId() ==
             CommandId::ID_ID3D12DEVICE8_CREATEPLACEDRESOURCE1) {
    auto* Command = static_cast<ID3D12Device8CreatePlacedResource1Command*>(
        resourceObjectState->CreationCommand.get());
    Microsoft::WRL::ComPtr<ID3D12Device8> device;
    HRESULT hr = m_Device->QueryInterface(IID_PPV_ARGS(&device));
    GITS_ASSERT(hr == S_OK);
    hr = device->CreatePlacedResource1(Command->m_pHeap.Value, Command->m_HeapOffset.Value,
                                       Command->m_pDesc.Value, D3D12_RESOURCE_STATE_COPY_SOURCE,
                                       Command->m_pOptimizedClearValue.Value,
                                       IID_PPV_ARGS(&auxiliaryResource));
    GITS_ASSERT(hr == S_OK);
  } else if (resourceObjectState->CreationCommand->GetId() ==
             CommandId::ID_ID3D12DEVICE10_CREATEPLACEDRESOURCE2) {
    auto* Command = static_cast<ID3D12Device10CreatePlacedResource2Command*>(
        resourceObjectState->CreationCommand.get());
    Microsoft::WRL::ComPtr<ID3D12Device10> device;
    HRESULT hr = m_Device->QueryInterface(IID_PPV_ARGS(&device));
    GITS_ASSERT(hr == S_OK);
    hr = device->CreatePlacedResource2(
        Command->m_pHeap.Value, Command->m_HeapOffset.Value, Command->m_pDesc.Value,
        D3D12_BARRIER_LAYOUT_COPY_SOURCE, Command->m_pOptimizedClearValue.Value,
        Command->m_NumCastableFormats.Value, Command->m_pCastableFormats.Value,
        IID_PPV_ARGS(&auxiliaryResource));
    GITS_ASSERT(hr == S_OK);
  } else if (resourceObjectState->CreationCommand->GetId() ==
             CommandId::INTC_D3D12_CREATEPLACEDRESOURCE) {
    auto* Command = static_cast<INTC_D3D12_CreatePlacedResourceCommand*>(
        resourceObjectState->CreationCommand.get());
    Microsoft::WRL::ComPtr<ID3D12Device8> device;
    HRESULT hr = m_Device->QueryInterface(IID_PPV_ARGS(&device));
    GITS_ASSERT(hr == S_OK);
    hr = INTC_D3D12_CreatePlacedResource(
        Command->m_pExtensionContext.Value, Command->m_pHeap.Value, Command->m_HeapOffset.Value,
        Command->m_pDesc.Value, D3D12_RESOURCE_STATE_COPY_SOURCE,
        Command->m_pOptimizedClearValue.Value, IID_PPV_ARGS(&auxiliaryResource));
    GITS_ASSERT(hr == S_OK);
  } else {
    GITS_ASSERT(false && "Unhandled ObjectState");
  }

  return auxiliaryResource;
}

unsigned ResourceContentRestore::CreateSubcaptureAuxiliaryPlacedResource(
    unsigned primaryResourceKey) {
  ObjectState* resourceObjectState = m_StateService.GetState(primaryResourceKey);
  unsigned auxiliaryResourceKey{};
  if (resourceObjectState->CreationCommand->GetId() ==
      CommandId::ID_ID3D12DEVICE_CREATEPLACEDRESOURCE) {
    auto* Command = static_cast<ID3D12DeviceCreatePlacedResourceCommand*>(
        resourceObjectState->CreationCommand.get());

    ID3D12DeviceCreatePlacedResourceCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_Object.Key = Command->m_Object.Key;
    c.m_pHeap.Key = Command->m_pHeap.Key;
    c.m_HeapOffset.Value = Command->m_HeapOffset.Value;
    c.m_pDesc.Value = Command->m_pDesc.Value;
    c.m_InitialState.Value = D3D12_RESOURCE_STATE_COPY_DEST;
    c.m_pOptimizedClearValue.Value = Command->m_pOptimizedClearValue.Value;
    c.m_riid.Value = Command->m_riid.Value;
    c.m_ppvResource.Key = m_StateService.GetUniqueObjectKey();
    m_StateService.GetRecorder().Record(ID3D12DeviceCreatePlacedResourceSerializer(c));
    auxiliaryResourceKey = c.m_ppvResource.Key;
  } else if (resourceObjectState->CreationCommand->GetId() ==
             CommandId::ID_ID3D12DEVICE8_CREATEPLACEDRESOURCE1) {
    auto* Command = static_cast<ID3D12Device8CreatePlacedResource1Command*>(
        resourceObjectState->CreationCommand.get());

    ID3D12Device8CreatePlacedResource1Command c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_Object.Key = Command->m_Object.Key;
    c.m_pHeap.Key = Command->m_pHeap.Key;
    c.m_HeapOffset.Value = Command->m_HeapOffset.Value;
    c.m_pDesc.Value = Command->m_pDesc.Value;
    c.m_InitialState.Value = D3D12_RESOURCE_STATE_COPY_DEST;
    c.m_pOptimizedClearValue.Value = Command->m_pOptimizedClearValue.Value;
    c.m_riid.Value = Command->m_riid.Value;
    c.m_ppvResource.Key = m_StateService.GetUniqueObjectKey();
    m_StateService.GetRecorder().Record(ID3D12Device8CreatePlacedResource1Serializer(c));
    auxiliaryResourceKey = c.m_ppvResource.Key;
  } else if (resourceObjectState->CreationCommand->GetId() ==
             CommandId::ID_ID3D12DEVICE10_CREATEPLACEDRESOURCE2) {
    auto* Command = static_cast<ID3D12Device10CreatePlacedResource2Command*>(
        resourceObjectState->CreationCommand.get());

    ID3D12Device10CreatePlacedResource2Command c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_Object.Key = Command->m_Object.Key;
    c.m_pHeap.Key = Command->m_pHeap.Key;
    c.m_HeapOffset.Value = Command->m_HeapOffset.Value;
    c.m_pDesc.Value = Command->m_pDesc.Value;
    c.m_InitialLayout.Value = D3D12_BARRIER_LAYOUT_COPY_DEST;
    c.m_pOptimizedClearValue.Value = Command->m_pOptimizedClearValue.Value;
    c.m_NumCastableFormats.Value = Command->m_NumCastableFormats.Value;
    c.m_pCastableFormats.Value = Command->m_pCastableFormats.Value;
    c.m_riid.Value = Command->m_riid.Value;
    c.m_ppvResource.Key = m_StateService.GetUniqueObjectKey();
    m_StateService.GetRecorder().Record(ID3D12Device10CreatePlacedResource2Serializer(c));
    auxiliaryResourceKey = c.m_ppvResource.Key;
  } else if (resourceObjectState->CreationCommand->GetId() ==
             CommandId::INTC_D3D12_CREATEPLACEDRESOURCE) {
    auto* Command = static_cast<INTC_D3D12_CreatePlacedResourceCommand*>(
        resourceObjectState->CreationCommand.get());

    INTC_D3D12_CreatePlacedResourceCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_pExtensionContext.Key = Command->m_pExtensionContext.Key;
    c.m_pHeap.Key = Command->m_pHeap.Key;
    c.m_HeapOffset.Value = Command->m_HeapOffset.Value;
    c.m_pDesc.Value = Command->m_pDesc.Value;
    c.m_InitialState.Value = D3D12_RESOURCE_STATE_COPY_DEST;
    c.m_pOptimizedClearValue.Value = Command->m_pOptimizedClearValue.Value;
    c.m_riid.Value = Command->m_riid.Value;
    c.m_ppvResource.Key = m_StateService.GetUniqueObjectKey();
    m_StateService.GetRecorder().Record(INTC_D3D12_CreatePlacedResourceSerializer(c));
    auxiliaryResourceKey = c.m_ppvResource.Key;
  } else {
    GITS_ASSERT(false && "Unhandled ObjectState");
  }

  return auxiliaryResourceKey;
}

void ResourceContentRestore::EvictPrevResidencyObjects() {
  if (m_PrevResidencyKeys.empty()) {
    return;
  }

  std::vector<unsigned> residencyKeys(m_PrevResidencyKeys.begin(), m_PrevResidencyKeys.end());
  std::vector<ID3D12Pageable*> residencyObjects(m_PrevResidencyObjects.begin(),
                                                m_PrevResidencyObjects.end());

  HRESULT hr = m_Device->Evict(residencyObjects.size(), residencyObjects.data());
  GITS_ASSERT(hr == S_OK);

  unsigned DeviceKey = m_StateService.GetDeviceKey();

  ID3D12DeviceEvictCommand Evict;
  Evict.Key = m_StateService.GetUniqueCommandKey();
  Evict.m_Object.Key = DeviceKey;
  Evict.m_NumObjects.Value = static_cast<UINT>(residencyKeys.size());
  ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
  Evict.m_ppObjects.Value = &fakePtr;
  Evict.m_ppObjects.Size = residencyKeys.size();
  for (unsigned key : residencyKeys) {
    Evict.m_ppObjects.Keys.push_back(key);
  }
  m_StateService.GetRecorder().Record(ID3D12DeviceEvictSerializer(Evict));
}

void ResourceContentRestore::CopySourceBarrier(ResourceInfo& state, bool RestoreState) {
  ResourceStateTrackingService::ResourceStates& resourceStates =
      m_StateService.GetResourceStateTrackingService().GetResourceStates(state.Key);
  std::vector<D3D12_RESOURCE_BARRIER> barriers;
  std::vector<std::unique_ptr<D3D12_TEXTURE_BARRIER>> enhancedBarriers;
  std::vector<D3D12_BARRIER_GROUP> enhancedBarrierGroups;

  if (resourceStates.AllEqual) {
    if (!resourceStates.SubresourceStates[0].Enhanced) {
      if (resourceStates.SubresourceStates[0].State != D3D12_RESOURCE_STATE_COPY_SOURCE) {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = state.Resource;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = RestoreState ? D3D12_RESOURCE_STATE_COPY_SOURCE
                                                      : resourceStates.SubresourceStates[0].State;
        barrier.Transition.StateAfter = RestoreState ? resourceStates.SubresourceStates[0].State
                                                     : D3D12_RESOURCE_STATE_COPY_SOURCE;
        barriers.push_back(barrier);
      }
    } else {
      if (resourceStates.SubresourceStates[0].Layout != D3D12_BARRIER_LAYOUT_COPY_SOURCE &&
          resourceStates.SubresourceStates[0].Layout != D3D12_BARRIER_LAYOUT_UNDEFINED) {
        D3D12_TEXTURE_BARRIER* barrier = new D3D12_TEXTURE_BARRIER{};
        barrier->SyncBefore = RestoreState ? D3D12_BARRIER_SYNC_ALL : D3D12_BARRIER_SYNC_ALL;
        barrier->SyncAfter = RestoreState ? D3D12_BARRIER_SYNC_ALL : D3D12_BARRIER_SYNC_ALL;
        barrier->AccessBefore =
            RestoreState ? D3D12_BARRIER_ACCESS_COPY_SOURCE : D3D12_BARRIER_ACCESS_COMMON;
        barrier->AccessAfter =
            RestoreState ? D3D12_BARRIER_ACCESS_COMMON : D3D12_BARRIER_ACCESS_COPY_SOURCE;
        barrier->LayoutBefore = RestoreState ? D3D12_BARRIER_LAYOUT_COPY_SOURCE
                                             : resourceStates.SubresourceStates[0].Layout;
        barrier->LayoutAfter = RestoreState ? resourceStates.SubresourceStates[0].Layout
                                            : D3D12_BARRIER_LAYOUT_COPY_SOURCE;
        barrier->pResource = state.Resource;
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
          barrier.Transition.pResource = state.Resource;
          barrier.Transition.Subresource = i;
          barrier.Transition.StateBefore = RestoreState ? D3D12_RESOURCE_STATE_COPY_SOURCE
                                                        : resourceStates.SubresourceStates[i].State;
          barrier.Transition.StateAfter = RestoreState ? resourceStates.SubresourceStates[i].State
                                                       : D3D12_RESOURCE_STATE_COPY_SOURCE;
          barriers.push_back(barrier);
        }
      } else {
        if (resourceStates.SubresourceStates[i].Layout != D3D12_BARRIER_LAYOUT_COPY_SOURCE &&
            resourceStates.SubresourceStates[i].Layout != D3D12_BARRIER_LAYOUT_UNDEFINED) {
          D3D12_TEXTURE_BARRIER* barrier = new D3D12_TEXTURE_BARRIER{};
          barrier->SyncBefore = RestoreState ? D3D12_BARRIER_SYNC_ALL : D3D12_BARRIER_SYNC_ALL;
          barrier->SyncAfter = RestoreState ? D3D12_BARRIER_SYNC_ALL : D3D12_BARRIER_SYNC_ALL;
          barrier->AccessBefore =
              RestoreState ? D3D12_BARRIER_ACCESS_COPY_SOURCE : D3D12_BARRIER_ACCESS_COMMON;
          barrier->AccessAfter =
              RestoreState ? D3D12_BARRIER_ACCESS_COMMON : D3D12_BARRIER_ACCESS_COPY_SOURCE;
          barrier->LayoutBefore = RestoreState ? D3D12_BARRIER_LAYOUT_COPY_SOURCE
                                               : resourceStates.SubresourceStates[i].Layout;
          barrier->LayoutAfter = RestoreState ? resourceStates.SubresourceStates[i].Layout
                                              : D3D12_BARRIER_LAYOUT_COPY_SOURCE;
          barrier->pResource = state.Resource;
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

} // namespace DirectX
} // namespace gits
