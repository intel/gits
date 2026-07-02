// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresInputBuffersService.h"
#include "stateTrackingService.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"

#include "xxhash.h"
#include <algorithm>

namespace gits {
namespace DirectX {

void AccelerationStructuresInputBuffersService::ExecuteCommandLists(
    ID3D12CommandQueueExecuteCommandListsCommand& c) {
  m_BufferInputDump.ExecuteCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                        c.m_ppCommandLists.Value, c.m_NumCommandLists.Value);
}

void AccelerationStructuresInputBuffersService::CommandQueueWait(ID3D12CommandQueueWaitCommand& c) {
  m_BufferInputDump.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void AccelerationStructuresInputBuffersService::CommandQueueSignal(
    ID3D12CommandQueueSignalCommand& c) {
  m_BufferInputDump.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void AccelerationStructuresInputBuffersService::FenceSignal(unsigned key,
                                                            unsigned fenceKey,
                                                            UINT64 fenceValue) {
  m_BufferInputDump.FenceSignal(key, fenceKey, fenceValue);
}

void AccelerationStructuresInputBuffersService::StoreBufferRegion(unsigned bufferKey,
                                                                  unsigned bufferOffset,
                                                                  unsigned bufferSize) {
  m_BufferRegionsByInputKey[bufferKey].emplace_back(bufferOffset, bufferOffset + bufferSize);
}

void AccelerationStructuresInputBuffersService::StoreBuffers(
    unsigned commandKey, ID3D12GraphicsCommandList* commandList) {

  InputBuffers* inputBuffers = new InputBuffers();
  m_InputBuffers[commandKey].reset(inputBuffers);

  for (auto& [inputKey, bufferRegions] : m_BufferRegionsByInputKey) {

    std::sort(bufferRegions.begin(), bufferRegions.end(),
              [](const BufferRegion& a, const BufferRegion& b) { return a.Start < b.Start; });

    std::vector<BufferRegion> bufferRegionsMerged;
    for (BufferRegion bufferRegion : bufferRegions) {
      if (bufferRegionsMerged.empty() || bufferRegionsMerged.back().End < bufferRegion.Start) {
        bufferRegionsMerged.push_back(bufferRegion);
      } else {
        bufferRegionsMerged.back().End = std::max(bufferRegionsMerged.back().End, bufferRegion.End);
      }
    }

    for (BufferRegion bufferRegion : bufferRegionsMerged) {
      m_StateService.KeepState(inputKey);
      ResourceState* bufferState = static_cast<ResourceState*>(m_StateService.GetState(inputKey));
      bufferState->CurrentState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      if (bufferState->DenyShaderResource) {
        bufferState->CurrentState = D3D12_RESOURCE_STATE_COMMON;
      }

      unsigned inputOffset = bufferRegion.Start;
      unsigned inputSize = bufferRegion.End - bufferRegion.Start;

      BarrierState currentState = GetAdjustedCurrentState(
          m_ResourceStateTracker, m_GpuAddressService, commandList,
          bufferState->GpuVirtualAddress + inputOffset,
          static_cast<ID3D12Resource*>(bufferState->Object), inputKey, bufferState->CurrentState);

      m_BufferInputDump.DumpBuffer(commandList, static_cast<ID3D12Resource*>(bufferState->Object),
                                   inputKey, inputOffset, inputSize, currentState, commandKey,
                                   bufferState->IsMappable);

      inputBuffers->Buffers[inputKey] = bufferState;
      ReservedResourcesService::TiledResource* tiledResource =
          m_ReservedResourcesService.GetTiledResource(inputKey);
      if (tiledResource) {
        auto it = inputBuffers->TiledResources.find(inputKey);
        if (it == inputBuffers->TiledResources.end()) {
          inputBuffers->TiledResources[inputKey] = *tiledResource;
        }
        for (ReservedResourcesService::Tile& tile : tiledResource->Tiles) {
          if (tile.HeapKey) {
            m_StateService.KeepState(tile.HeapKey);
          }
        }
      }
    }
  }
  m_BufferRegionsByInputKey.clear();
}

void AccelerationStructuresInputBuffersService::RestoreBuffersInitialization(
    std::vector<unsigned>& commandKeys, unsigned deviceKey) {

  m_BufferInputDump.WaitUntilDumped();

  size_t maxPerBuildUploadSize = 0;
  for (unsigned commandKey : commandKeys) {
    std::vector<BufferInputDump::InputBuffer>& inputBuffers =
        m_BufferInputDump.GetInputBuffers(commandKey);
    size_t uploadSize = 0;
    for (BufferInputDump::InputBuffer& inputBuffer : inputBuffers) {
      if (!inputBuffer.IsMappable) {
        uploadSize += inputBuffer.BufferData->size();
      }
    }
    if (uploadSize > maxPerBuildUploadSize) {
      maxPerBuildUploadSize = uploadSize;
    }
  }
  m_UploadBufferSize = maxPerBuildUploadSize;

  if (!m_UploadBufferSize) {
    return;
  }

  D3D12_HEAP_PROPERTIES heapPropertiesUpload{};
  heapPropertiesUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapPropertiesUpload.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapPropertiesUpload.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapPropertiesUpload.CreationNodeMask = 1;
  heapPropertiesUpload.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = m_UploadBufferSize;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  m_UploadBufferKey = m_StateService.GetUniqueObjectKey();

  ID3D12DeviceCreateCommittedResourceCommand createUploadResource;
  createUploadResource.Key = m_StateService.GetUniqueCommandKey();
  createUploadResource.m_Object.Key = deviceKey;
  createUploadResource.m_pHeapProperties.Value = &heapPropertiesUpload;
  createUploadResource.m_HeapFlags.Value = D3D12_HEAP_FLAG_NONE;
  createUploadResource.m_pDesc.Value = &resourceDesc;
  createUploadResource.m_InitialResourceState.Value = D3D12_RESOURCE_STATE_GENERIC_READ;
  createUploadResource.m_pOptimizedClearValue.Value = nullptr;
  createUploadResource.m_riidResource.Value = IID_ID3D12Resource;
  createUploadResource.m_ppvResource.Key = m_UploadBufferKey;
  m_Recorder.Record(ID3D12DeviceCreateCommittedResourceSerializer(createUploadResource));

  m_CommandQueueKey = m_StateService.GetUniqueObjectKey();
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
  ID3D12DeviceCreateCommandQueueCommand createCommandQueue;
  createCommandQueue.Key = m_StateService.GetUniqueCommandKey();
  createCommandQueue.m_Object.Key = deviceKey;
  createCommandQueue.m_pDesc.Value = &commandQueueDesc;
  createCommandQueue.m_riid.Value = IID_ID3D12CommandQueue;
  createCommandQueue.m_ppCommandQueue.Key = m_CommandQueueKey;
  m_StateService.GetRecorder().Record(ID3D12DeviceCreateCommandQueueSerializer(createCommandQueue));

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
}

void AccelerationStructuresInputBuffersService::MakeBuffersResident(
    unsigned commandKey, ResourceResidencyService& residencyService) {
  for (BufferInputDump::InputBuffer& inputBuffer : m_BufferInputDump.GetInputBuffers(commandKey)) {
    residencyService.AddResource(inputBuffer.BufferKey);
  }
}

void AccelerationStructuresInputBuffersService::RestoreBuffers(unsigned commandKey,
                                                               unsigned commandListBarriersKey) {
  std::vector<BufferInputDump::InputBuffer>& inputBufferDumps =
      m_BufferInputDump.GetInputBuffers(commandKey);
  InputBuffers* inputBuffers = m_InputBuffers[commandKey].get();

  std::unordered_set<unsigned> restoredBuffers;
  size_t uploadBufferOffset{};
  for (BufferInputDump::InputBuffer& info : inputBufferDumps) {
    auto itHash = m_BufferHashesByKeyOffset.find(std::pair(info.BufferKey, info.Offset));
    if (itHash != m_BufferHashesByKeyOffset.end() && itHash->second == info.BufferHash) {
      continue;
    }
    m_BufferHashesByKeyOffset[std::pair(info.BufferKey, info.Offset)] = info.BufferHash;
    restoredBuffers.insert(info.BufferKey);

    for (auto& itTiledResource : inputBuffers->TiledResources) {
      auto it = m_TiledResourceUpdatesRestored.find(info.BufferKey);
      if (it == m_TiledResourceUpdatesRestored.end() ||
          it->second.find(itTiledResource.second.UpdateId) == it->second.end()) {
        m_ReservedResourcesService.UpdateTileMappings(itTiledResource.second, m_CommandQueueKey,
                                                      nullptr);
        m_TiledResourceUpdatesRestored[info.BufferKey].insert(itTiledResource.second.UpdateId);
      }
    }

    uploadBufferOffset += RestoreBuffer(info, uploadBufferOffset);
  }

  if (m_UploadBufferSize) {
    ID3D12GraphicsCommandListCloseCommand commandListClose;
    commandListClose.Key = m_StateService.GetUniqueCommandKey();
    commandListClose.m_Object.Key = m_CommandListKey;
    m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListCloseSerializer(commandListClose));

    ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
    executeCommandLists.Key = m_StateService.GetUniqueCommandKey();
    executeCommandLists.m_Object.Key = m_CommandQueueKey;
    executeCommandLists.m_NumCommandLists.Value = 1;
    executeCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
    executeCommandLists.m_ppCommandLists.Size = 1;
    executeCommandLists.m_ppCommandLists.Keys.resize(1);
    executeCommandLists.m_ppCommandLists.Keys[0] = m_CommandListKey;
    m_StateService.GetRecorder().Record(
        ID3D12CommandQueueExecuteCommandListsSerializer(executeCommandLists));

    ID3D12CommandQueueSignalCommand commandQueueSignal;
    commandQueueSignal.Key = m_StateService.GetUniqueCommandKey();
    commandQueueSignal.m_Object.Key = m_CommandQueueKey;
    commandQueueSignal.m_pFence.Key = m_FenceKey;
    commandQueueSignal.m_Value.Value = ++m_RecordedFenceValue;
    m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(commandQueueSignal));

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

    ID3D12GraphicsCommandListResetCommand commandListReset;
    commandListReset.Key = m_StateService.GetUniqueCommandKey();
    commandListReset.m_Object.Key = m_CommandListKey;
    commandListReset.m_pAllocator.Key = m_CommandAllocatorKey;
    commandListReset.m_pInitialState.Key = 0;
    m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListResetSerializer(commandListReset));
  }

  for (auto& it : inputBuffers->Buffers) {
    if (!it.second->IsMappable && restoredBuffers.find(it.first) != restoredBuffers.end()) {
      ID3D12GraphicsCommandListResourceBarrierCommand barrierCommand;
      barrierCommand.Key = m_StateService.GetUniqueCommandKey();
      barrierCommand.m_Object.Key = commandListBarriersKey;
      barrierCommand.m_NumBarriers.Value = 1;
      D3D12_RESOURCE_BARRIER barrier{};
      barrierCommand.m_pBarriers.Value = &barrier;
      barrierCommand.m_pBarriers.Size = 1;
      barrierCommand.m_pBarriers.ResourceKeys.resize(1);
      barrierCommand.m_pBarriers.ResourceAfterKeys.resize(1);
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      barrier.Transition.StateAfter = it.second->CurrentState;
      barrierCommand.m_pBarriers.ResourceKeys[0] = it.first;
      m_StateService.GetRecorder().Record(
          ID3D12GraphicsCommandListResourceBarrierSerializer(barrierCommand));
    }
  }
}

void AccelerationStructuresInputBuffersService::RestoreBuffersCleanup() {
  IUnknownReleaseCommand releaseFence{};
  releaseFence.Key = m_StateService.GetUniqueCommandKey();
  releaseFence.m_Object.Key = m_FenceKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseFence));

  IUnknownReleaseCommand releaseCommandList{};
  releaseCommandList.Key = m_StateService.GetUniqueCommandKey();
  releaseCommandList.m_Object.Key = m_CommandListKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandList));

  IUnknownReleaseCommand releaseCommandAllocator{};
  releaseCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
  releaseCommandAllocator.m_Object.Key = m_CommandAllocatorKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandAllocator));

  IUnknownReleaseCommand releaseCommandQueue{};
  releaseCommandQueue.Key = m_StateService.GetUniqueCommandKey();
  releaseCommandQueue.m_Object.Key = m_CommandQueueKey;
  m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommandQueue));

  if (m_UploadBufferKey) {
    IUnknownReleaseCommand releaseUploadBuffer{};
    releaseUploadBuffer.Key = m_StateService.GetUniqueCommandKey();
    releaseUploadBuffer.m_Object.Key = m_UploadBufferKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseUploadBuffer));
  }
}

size_t AccelerationStructuresInputBuffersService::RestoreBuffer(
    const BufferInputDump::InputBuffer& restoreInfo, size_t uploadBufferOffset) {
  if (restoreInfo.IsMappable) {
    ID3D12ResourceMapCommand mapCommand;
    mapCommand.Key = m_StateService.GetUniqueCommandKey();
    mapCommand.m_Object.Key = restoreInfo.BufferKey;
    mapCommand.m_Subresource.Value = 0;
    mapCommand.m_pReadRange.Value = nullptr;
    mapCommand.m_ppData.CaptureValue = m_StateService.GetUniqueFakePointer();
    mapCommand.m_ppData.Value = &mapCommand.m_ppData.CaptureValue;
    m_Recorder.Record(ID3D12ResourceMapSerializer(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.Key = m_StateService.GetUniqueCommandKey();
    metaCommand.m_resource.Key = restoreInfo.BufferKey;
    metaCommand.m_mappedAddress.Value = mapCommand.m_ppData.CaptureValue;
    metaCommand.m_offset.Value = restoreInfo.Offset;
    metaCommand.m_data.Value = const_cast<char*>(restoreInfo.BufferData->data());
    metaCommand.m_data.Size = restoreInfo.BufferData->size();
    m_Recorder.Record(MappedDataMetaSerializer(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.Key = m_StateService.GetUniqueCommandKey();
    unmapCommand.m_Object.Key = restoreInfo.BufferKey;
    unmapCommand.m_Subresource.Value = 0;
    unmapCommand.m_pWrittenRange.Value = nullptr;
    m_Recorder.Record(ID3D12ResourceUnmapSerializer(unmapCommand));

    return 0;
  } else {
    GITS_ASSERT(uploadBufferOffset + restoreInfo.BufferData->size() <= m_UploadBufferSize);

    ID3D12ResourceMapCommand mapCommand;
    mapCommand.Key = m_StateService.GetUniqueCommandKey();
    mapCommand.m_Object.Key = m_UploadBufferKey;
    mapCommand.m_Subresource.Value = 0;
    mapCommand.m_pReadRange.Value = nullptr;
    mapCommand.m_ppData.CaptureValue = m_StateService.GetUniqueFakePointer();
    mapCommand.m_ppData.Value = &mapCommand.m_ppData.CaptureValue;
    m_Recorder.Record(ID3D12ResourceMapSerializer(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.Key = m_StateService.GetUniqueCommandKey();
    metaCommand.m_resource.Key = m_UploadBufferKey;
    metaCommand.m_mappedAddress.Value = mapCommand.m_ppData.CaptureValue;
    metaCommand.m_offset.Value = uploadBufferOffset;
    metaCommand.m_data.Value = const_cast<char*>(restoreInfo.BufferData->data());
    metaCommand.m_data.Size = restoreInfo.BufferData->size();
    m_Recorder.Record(MappedDataMetaSerializer(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.Key = m_StateService.GetUniqueCommandKey();
    unmapCommand.m_Object.Key = m_UploadBufferKey;
    unmapCommand.m_Subresource.Value = 0;
    unmapCommand.m_pWrittenRange.Value = nullptr;
    m_Recorder.Record(ID3D12ResourceUnmapSerializer(unmapCommand));

    ID3D12GraphicsCommandListCopyBufferRegionCommand copyBufferRegion;
    copyBufferRegion.Key = m_StateService.GetUniqueCommandKey();
    copyBufferRegion.m_Object.Key = m_CommandListKey;
    copyBufferRegion.m_pDstBuffer.Key = restoreInfo.BufferKey;
    copyBufferRegion.m_DstOffset.Value = restoreInfo.Offset;
    copyBufferRegion.m_pSrcBuffer.Key = m_UploadBufferKey;
    copyBufferRegion.m_SrcOffset.Value = uploadBufferOffset;
    copyBufferRegion.m_NumBytes.Value = restoreInfo.BufferData->size();
    m_Recorder.Record(ID3D12GraphicsCommandListCopyBufferRegionSerializer(copyBufferRegion));

    return restoreInfo.BufferData->size();
  }
}

void AccelerationStructuresInputBuffersService::BufferInputDump::DumpBuffer(
    ID3D12GraphicsCommandList* commandList,
    ID3D12Resource* resource,
    unsigned resourceKey,
    unsigned offset,
    unsigned size,
    BarrierState resourceState,
    unsigned buildCallKey,
    bool isMappable) {
  BufferInfo* info = new BufferInfo();
  info->Offset = offset;
  info->Size = size;
  info->BuildCallKey = buildCallKey;
  info->ResourceKey = resourceKey;
  info->IsMappable = isMappable;
  StageResource(commandList, resource, resourceState, *info);
}

void AccelerationStructuresInputBuffersService::BufferInputDump::DumpBuffer(DumpInfo& dumpInfo,
                                                                            void* data) {
  BufferInfo& info = static_cast<BufferInfo&>(dumpInfo);
  InputBuffer restoreInfo{};
  restoreInfo.BufferKey = info.ResourceKey;
  restoreInfo.Offset = info.Offset;
  restoreInfo.IsMappable = info.IsMappable;
  restoreInfo.BufferHash = XXH32(data, info.Size, 0);
  restoreInfo.BufferData = std::make_unique<std::vector<char>>(
      static_cast<char*>(data), static_cast<char*>(data) + info.Size);

  std::lock_guard<std::mutex> lock(m_Mutex);
  m_InputBuffersByBuildKey[info.BuildCallKey].push_back(std::move(restoreInfo));
}

} // namespace DirectX
} // namespace gits
