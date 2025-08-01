// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresBufferContentRestore.h"
#include "commandsAuto.h"
#include "commandWritersAuto.h"
#include "commandsCustom.h"
#include "commandWritersCustom.h"
#include "stateTrackingService.h"

namespace gits {
namespace DirectX {

void AccelerationStructuresBufferContentRestore::storeBuffer(ID3D12GraphicsCommandList* commandList,
                                                             unsigned commandListKey,
                                                             ID3D12Resource* resource,
                                                             unsigned resourceKey,
                                                             unsigned offset,
                                                             unsigned size,
                                                             D3D12_RESOURCE_STATES resourceState,
                                                             unsigned buildCallKey,
                                                             bool isMappable,
                                                             unsigned uploadResourceKey) {
  BufferInfo* info = new BufferInfo();
  info->offset = offset;
  info->size = size;
  info->buildCallKey = buildCallKey;
  info->resourceKey = resourceKey;
  info->commandListKey = commandListKey;
  info->isMappable = isMappable;
  info->uploadResourceKey = uploadResourceKey;
  info->dumpName = L"BLAS build " + std::to_wstring(buildCallKey) + L" resource O" +
                   std::to_wstring(resourceKey);

  stageResource(commandList, resource, resourceState, *info);

  std::lock_guard<std::mutex> lock(mutex_);
  restoreBuilds_.insert(buildCallKey);
}

void AccelerationStructuresBufferContentRestore::dumpBuffer(DumpInfo& dumpInfo, void* data) {

  BufferInfo& info = static_cast<BufferInfo&>(dumpInfo);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (restoreBuilds_.find(info.buildCallKey) == restoreBuilds_.end()) {
      return;
    }
  }

  BufferRestoreInfo restoreInfo{};
  restoreInfo.bufferKey = info.resourceKey;
  restoreInfo.uploadResourceKey = info.uploadResourceKey;
  restoreInfo.offset = info.offset;
  restoreInfo.bufferHash = ComputeHash(data, info.size, THashType::XX);

  std::lock_guard<std::mutex> lock(mutex_);

  if (info.isMappable) {

    ID3D12ResourceMapCommand mapCommand;
    mapCommand.key = stateService_.getUniqueCommandKey();
    mapCommand.object_.key = info.resourceKey;
    mapCommand.Subresource_.value = 0;
    mapCommand.pReadRange_.value = nullptr;
    mapCommand.ppData_.captureValue = stateService_.getUniqueFakePointer();
    mapCommand.ppData_.value = &mapCommand.ppData_.captureValue;
    restoreInfo.restoreCommands.push_back(new ID3D12ResourceMapWriter(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.key = stateService_.getUniqueCommandKey();
    metaCommand.resource_.key = info.resourceKey;
    metaCommand.mappedAddress_.value = mapCommand.ppData_.captureValue;
    metaCommand.offset_.value = info.offset;
    metaCommand.data_.value = data;
    metaCommand.data_.size = info.size;
    restoreInfo.restoreCommands.push_back(new MappedDataMetaWriter(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.key = stateService_.getUniqueCommandKey();
    unmapCommand.object_.key = info.resourceKey;
    unmapCommand.Subresource_.value = 0;
    unmapCommand.pWrittenRange_.value = nullptr;
    restoreInfo.restoreCommands.push_back(new ID3D12ResourceUnmapWriter(unmapCommand));

  } else {

    // create upload resource with resources contents in subcaptured stream

    D3D12_HEAP_PROPERTIES heapPropertiesUpload{};
    heapPropertiesUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapPropertiesUpload.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapPropertiesUpload.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapPropertiesUpload.CreationNodeMask = 1;
    heapPropertiesUpload.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = info.size;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12DeviceCreateCommittedResourceCommand createUploadResource;
    createUploadResource.key = stateService_.getUniqueCommandKey();
    createUploadResource.object_.key = deviceKey_;
    createUploadResource.pHeapProperties_.value = &heapPropertiesUpload;
    createUploadResource.HeapFlags_.value = D3D12_HEAP_FLAG_NONE;
    createUploadResource.pDesc_.value = &resourceDesc;
    createUploadResource.InitialResourceState_.value = D3D12_RESOURCE_STATE_GENERIC_READ;
    createUploadResource.pOptimizedClearValue_.value = nullptr;
    createUploadResource.riidResource_.value = IID_ID3D12Resource;
    createUploadResource.ppvResource_.key = info.uploadResourceKey;
    restoreInfo.restoreCommands.push_back(
        new ID3D12DeviceCreateCommittedResourceWriter(createUploadResource));

    ID3D12ResourceMapCommand mapCommand;
    mapCommand.key = stateService_.getUniqueCommandKey();
    mapCommand.object_.key = info.uploadResourceKey;
    mapCommand.Subresource_.value = 0;
    mapCommand.pReadRange_.value = nullptr;
    mapCommand.ppData_.captureValue = stateService_.getUniqueFakePointer();
    mapCommand.ppData_.value = &mapCommand.ppData_.captureValue;
    restoreInfo.restoreCommands.push_back(new ID3D12ResourceMapWriter(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.key = stateService_.getUniqueCommandKey();
    metaCommand.resource_.key = info.uploadResourceKey;
    metaCommand.mappedAddress_.value = mapCommand.ppData_.captureValue;
    metaCommand.offset_.value = 0;
    metaCommand.data_.value = data;
    metaCommand.data_.size = info.size;
    restoreInfo.restoreCommands.push_back(new MappedDataMetaWriter(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.key = stateService_.getUniqueCommandKey();
    unmapCommand.object_.key = info.uploadResourceKey;
    unmapCommand.Subresource_.value = 0;
    unmapCommand.pWrittenRange_.value = nullptr;
    restoreInfo.restoreCommands.push_back(new ID3D12ResourceUnmapWriter(unmapCommand));

    // restore resources contents from readback resource in subcaptured stream

    ID3D12GraphicsCommandListCopyBufferRegionCommand copyBufferRegion;
    copyBufferRegion.key = stateService_.getUniqueCommandKey();
    copyBufferRegion.object_.key = info.commandListKey;
    copyBufferRegion.pDstBuffer_.key = info.resourceKey;
    copyBufferRegion.DstOffset_.value = info.offset;
    copyBufferRegion.pSrcBuffer_.key = info.uploadResourceKey;
    copyBufferRegion.SrcOffset_.value = 0;
    copyBufferRegion.NumBytes_.value = info.size;
    restoreInfo.restoreCommands.push_back(
        new ID3D12GraphicsCommandListCopyBufferRegionWriter(copyBufferRegion));
  }

  restoreBuildCommands_[info.buildCallKey].push_back(std::move(restoreInfo));
}

void AccelerationStructuresBufferContentRestore::removeBuild(unsigned buildCallKey) {
  std::lock_guard<std::mutex> lock(mutex_);
  restoreBuilds_.erase(buildCallKey);
  auto it = restoreBuildCommands_.find(buildCallKey);
  if (it != restoreBuildCommands_.end()) {
    for (BufferRestoreInfo& restoreInfo : it->second) {
      for (CommandWriter* writer : restoreInfo.restoreCommands) {
        delete writer;
      }
    }
    restoreBuildCommands_.erase(buildCallKey);
  }
}

} // namespace DirectX
} // namespace gits
