// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "AccelerationStructuresSerializeService.h"
#include "stateTrackingService.h"
#include "arguments.h"
#include "argumentEncoders.h"
#include "argumentDecoders.h"
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"
#include "reservedResourcesService.h"
#include "gits.h"
#include "configurationLib.h"

namespace gits {
namespace DirectX {

AccelerationStructuresSerializeService::AccelerationStructuresSerializeService(
    StateTrackingService& stateService, SubcaptureRecorder& recorder)
    : stateService_(stateService), recorder_(recorder) {
  serializeMode_ = Configurator::Get().directx.features.subcapture.serializeAccelerationStructures;
}

void AccelerationStructuresSerializeService::buildAccelerationStructure(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (!serializeMode_ ||
      c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }
  AccelerationStructure as{};
  as.callKey = c.key;
  as.key = c.pDesc_.destAccelerationStructureKey;
  as.offset = c.pDesc_.destAccelerationStructureOffset;
  as.address = c.pDesc_.value->DestAccelerationStructureData;
  accelerationStructuresByCommandList_[c.object_.key]
                                      [c.pDesc_.value->DestAccelerationStructureData] = as;
  accelerationStructuresByResource_[c.pDesc_.destAccelerationStructureKey].insert(
      c.pDesc_.value->DestAccelerationStructureData);
}

void AccelerationStructuresSerializeService::copyAccelerationStructure(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (!serializeMode_) {
    return;
  }
  AccelerationStructure as{};
  as.callKey = c.key;
  as.key = c.DestAccelerationStructureData_.interfaceKey;
  as.offset = c.DestAccelerationStructureData_.offset;
  as.address = c.DestAccelerationStructureData_.value;
  accelerationStructuresByCommandList_[c.object_.key][c.DestAccelerationStructureData_.value] = as;
  accelerationStructuresByResource_[c.DestAccelerationStructureData_.interfaceKey].insert(
      c.DestAccelerationStructureData_.value);
}

void AccelerationStructuresSerializeService::executeCommandLists(
    ID3D12CommandQueueExecuteCommandListsCommand& c) {
  for (unsigned commandListKey : c.ppCommandLists_.keys) {
    auto itByKey = accelerationStructuresByCommandList_.find(commandListKey);
    if (itByKey != accelerationStructuresByCommandList_.end()) {
      for (auto& it : itByKey->second) {
        accelerationStructures_[it.first] = it.second;
      }
      accelerationStructuresByCommandList_.erase(itByKey);
    }
  }
}

void AccelerationStructuresSerializeService::destroyResource(unsigned resourceKey) {
  if (!serializeMode_) {
    return;
  }
  auto itResource = accelerationStructuresByResource_.find(resourceKey);
  if (itResource == accelerationStructuresByResource_.end()) {
    return;
  }
  for (D3D12_GPU_VIRTUAL_ADDRESS address : itResource->second) {
    accelerationStructures_.erase(address);
  }
  accelerationStructuresByResource_.erase(itResource);
}

void AccelerationStructuresSerializeService::restoreAccelerationStructures() {
  if (!serializeMode_) {
    return;
  }
  {
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

    commandQueueKey_ = stateService_.getUniqueObjectKey();
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    ID3D12DeviceCreateCommandQueueCommand createCommandQueue;
    createCommandQueue.key = stateService_.getUniqueCommandKey();
    createCommandQueue.object_.key = deviceKey_;
    createCommandQueue.pDesc_.value = &commandQueueDesc;
    createCommandQueue.riid_.value = IID_ID3D12CommandQueue;
    createCommandQueue.ppCommandQueue_.key = commandQueueKey_;
    stateService_.getRecorder().record(
        new ID3D12DeviceCreateCommandQueueWriter(createCommandQueue));

    commandAllocatorKey_ = stateService_.getUniqueObjectKey();
    ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
    createCommandAllocator.key = stateService_.getUniqueCommandKey();
    createCommandAllocator.object_.key = deviceKey_;
    createCommandAllocator.type_.value = D3D12_COMMAND_LIST_TYPE_COPY;
    createCommandAllocator.riid_.value = IID_ID3D12CommandAllocator;
    createCommandAllocator.ppCommandAllocator_.key = commandAllocatorKey_;
    stateService_.getRecorder().record(
        new ID3D12DeviceCreateCommandAllocatorWriter(createCommandAllocator));

    commandListKey_ = stateService_.getUniqueObjectKey();
    ID3D12DeviceCreateCommandListCommand createCommandList;
    createCommandList.key = stateService_.getUniqueCommandKey();
    createCommandList.object_.key = deviceKey_;
    createCommandList.nodeMask_.value = 0;
    createCommandList.pCommandAllocator_.key = createCommandAllocator.ppCommandAllocator_.key;
    createCommandList.type_.value = D3D12_COMMAND_LIST_TYPE_COPY;
    createCommandList.pInitialState_.value = nullptr;
    createCommandList.riid_.value = IID_ID3D12GraphicsCommandList4;
    createCommandList.ppCommandList_.key = commandListKey_;
    stateService_.getRecorder().record(new ID3D12DeviceCreateCommandListWriter(createCommandList));

    fenceKey_ = stateService_.getUniqueObjectKey();
    ID3D12DeviceCreateFenceCommand createFence;
    createFence.key = stateService_.getUniqueCommandKey();
    createFence.object_.key = deviceKey_;
    createFence.InitialValue_.value = 0;
    createFence.Flags_.value = D3D12_FENCE_FLAG_NONE;
    createFence.riid_.value = IID_ID3D12Fence;
    createFence.ppFence_.key = fenceKey_;
    stateService_.getRecorder().record(new ID3D12DeviceCreateFenceWriter(createFence));
  }

  unsigned infoBufferSize =
      sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION_DESC) *
      accelerationStructures_.size();
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
  heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProperties.CreationNodeMask = 1;
  heapProperties.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = infoBufferSize;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  Microsoft::WRL::ComPtr<ID3D12Resource> infoResource;
  HRESULT hr = device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
                                                &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                IID_PPV_ARGS(&infoResource));
  GITS_ASSERT(hr == S_OK);

  Microsoft::WRL::ComPtr<ID3D12Resource> infoStagingResource;
  heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  hr = device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                        D3D12_RESOURCE_STATE_COMMON, nullptr,
                                        IID_PPV_ARGS(&infoStagingResource));
  GITS_ASSERT(hr == S_OK);

  std::map<unsigned, AccelerationStructure> accelerationStructuresByCallKey;
  for (auto& it : accelerationStructures_) {
    accelerationStructuresByCallKey[it.second.callKey] = it.second;
  }

  std::vector<D3D12_GPU_VIRTUAL_ADDRESS> destAdresses(accelerationStructures_.size());
  unsigned index = 0;
  for (auto& it : accelerationStructuresByCallKey) {
    destAdresses[index] = it.second.address;
    ++index;
  }

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC desc{};
  desc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION;
  desc.DestBuffer = infoResource->GetGPUVirtualAddress();
  commandList_->EmitRaytracingAccelerationStructurePostbuildInfo(&desc, destAdresses.size(),
                                                                 destAdresses.data());
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = infoResource.Get();
  barrier.Transition.Subresource = 0;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  commandList_->ResourceBarrier(1, &barrier);

  commandList_->CopyResource(infoStagingResource.Get(), infoResource.Get());
  commandList_->Close();
  ID3D12CommandList* commandLists[] = {commandList_};
  commandQueue_->ExecuteCommandLists(1, commandLists);
  hr = commandQueue_->Signal(fence_, ++currentFenceValue_);
  GITS_ASSERT(hr == S_OK);
  hr = fence_->SetEventOnCompletion(currentFenceValue_, NULL);
  GITS_ASSERT(hr == S_OK);

  hr = commandAllocator_->Reset();
  GITS_ASSERT(hr == S_OK);
  hr = commandList_->Reset(commandAllocator_, nullptr);
  GITS_ASSERT(hr == S_OK);

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION_DESC* infoData{};
  hr = infoStagingResource->Map(0, nullptr, reinterpret_cast<void**>(&infoData));
  GITS_ASSERT(hr == S_OK);

  index = 0;
  for (auto& it : accelerationStructuresByCallKey) {

    if (infoData[index].NumBottomLevelAccelerationStructurePointers) {
      continue;
    }

    unsigned destKey = it.second.key;
    unsigned destOffset = it.second.offset;

    // serialize acceleration structure

    Microsoft::WRL::ComPtr<ID3D12Resource> serializedResource;
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    resourceDesc.Width = infoData[index].SerializedSizeInBytes;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                          D3D12_RESOURCE_STATE_COMMON, nullptr,
                                          IID_PPV_ARGS(&serializedResource));
    GITS_ASSERT(hr == S_OK);

    Microsoft::WRL::ComPtr<ID3D12Resource> serializedStagingResource;
    heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
    resourceDesc.Width = infoData[index].SerializedSizeInBytes;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    hr = device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                          D3D12_RESOURCE_STATE_COMMON, nullptr,
                                          IID_PPV_ARGS(&serializedStagingResource));
    GITS_ASSERT(hr == S_OK);

    commandList_->CopyRaytracingAccelerationStructure(
        serializedResource->GetGPUVirtualAddress(), destAdresses[index],
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = serializedResource.Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    commandList_->ResourceBarrier(1, &barrier);

    commandList_->CopyResource(serializedStagingResource.Get(), serializedResource.Get());
    commandList_->Close();
    ID3D12CommandList* commandLists[] = {commandList_};
    commandQueue_->ExecuteCommandLists(1, commandLists);
    hr = commandQueue_->Signal(fence_, ++currentFenceValue_);
    GITS_ASSERT(hr == S_OK);
    hr = fence_->SetEventOnCompletion(currentFenceValue_, NULL);
    GITS_ASSERT(hr == S_OK);

    hr = commandAllocator_->Reset();
    GITS_ASSERT(hr == S_OK);
    hr = commandList_->Reset(commandAllocator_, nullptr);
    GITS_ASSERT(hr == S_OK);

    // create upload resource with serialize acceleration structure in subcaptured stream

    unsigned uploadResourceKey = stateService_.getUniqueObjectKey();
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    ID3D12DeviceCreateCommittedResourceCommand createUploadResource;
    createUploadResource.key = stateService_.getUniqueCommandKey();
    createUploadResource.object_.key = deviceKey_;
    createUploadResource.pHeapProperties_.value = &heapProperties;
    createUploadResource.HeapFlags_.value = D3D12_HEAP_FLAG_NONE;
    createUploadResource.pDesc_.value = &resourceDesc;
    createUploadResource.InitialResourceState_.value = D3D12_RESOURCE_STATE_GENERIC_READ;
    createUploadResource.pOptimizedClearValue_.value = nullptr;
    createUploadResource.riidResource_.value = IID_ID3D12Resource;
    createUploadResource.ppvResource_.key = uploadResourceKey;
    stateService_.getRecorder().record(
        new ID3D12DeviceCreateCommittedResourceWriter(createUploadResource));

    void* mappedData{};
    hr = serializedStagingResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
    GITS_ASSERT(hr == S_OK);

    ID3D12ResourceMapCommand mapCommand;
    mapCommand.key = stateService_.getUniqueCommandKey();
    mapCommand.object_.key = uploadResourceKey;
    mapCommand.Subresource_.value = 0;
    mapCommand.pReadRange_.value = nullptr;
    mapCommand.ppData_.captureValue = stateService_.getUniqueFakePointer();
    mapCommand.ppData_.value = &mapCommand.ppData_.captureValue;
    stateService_.getRecorder().record(new ID3D12ResourceMapWriter(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.key = stateService_.getUniqueCommandKey();
    metaCommand.resource_.key = uploadResourceKey;
    metaCommand.mappedAddress_.value = mapCommand.ppData_.captureValue;
    metaCommand.offset_.value = 0;
    metaCommand.data_.value = mappedData;
    metaCommand.data_.size = resourceDesc.Width;
    stateService_.getRecorder().record(new MappedDataMetaWriter(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.key = stateService_.getUniqueCommandKey();
    unmapCommand.object_.key = uploadResourceKey;
    unmapCommand.Subresource_.value = 0;
    unmapCommand.pWrittenRange_.value = nullptr;
    stateService_.getRecorder().record(new ID3D12ResourceUnmapWriter(unmapCommand));

    serializedStagingResource->Unmap(0, nullptr);

    // deserialize acceleration structure in subcaptured stream

    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand copyASCommand;
    copyASCommand.key = stateService_.getUniqueCommandKey();
    copyASCommand.object_.key = commandListKey_;
    copyASCommand.DestAccelerationStructureData_.interfaceKey = destKey;
    copyASCommand.DestAccelerationStructureData_.offset = destOffset;
    copyASCommand.SourceAccelerationStructureData_.interfaceKey = uploadResourceKey;
    copyASCommand.SourceAccelerationStructureData_.offset = 0;
    copyASCommand.Mode_.value = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE;
    stateService_.getRecorder().record(
        new ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureWriter(copyASCommand));

    ID3D12GraphicsCommandListCloseCommand commandListClose;
    commandListClose.key = stateService_.getUniqueCommandKey();
    commandListClose.object_.key = commandListKey_;
    stateService_.getRecorder().record(new ID3D12GraphicsCommandListCloseWriter(commandListClose));

    ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
    executeCommandLists.key = stateService_.getUniqueCommandKey();
    executeCommandLists.object_.key = commandQueueKey_;
    executeCommandLists.NumCommandLists_.value = 1;
    executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
    executeCommandLists.ppCommandLists_.size = 1;
    executeCommandLists.ppCommandLists_.keys.resize(1);
    executeCommandLists.ppCommandLists_.keys[0] = commandListKey_;
    stateService_.getRecorder().record(
        new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));

    ID3D12CommandQueueSignalCommand commandQueueSignal;
    commandQueueSignal.key = stateService_.getUniqueCommandKey();
    commandQueueSignal.object_.key = commandQueueKey_;
    commandQueueSignal.pFence_.key = fenceKey_;
    commandQueueSignal.Value_.value = ++recordedFenceValue_;
    stateService_.getRecorder().record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));

    ID3D12FenceGetCompletedValueCommand getCompletedValue;
    getCompletedValue.key = stateService_.getUniqueCommandKey();
    getCompletedValue.object_.key = fenceKey_;
    getCompletedValue.result_.value = recordedFenceValue_;
    stateService_.getRecorder().record(new ID3D12FenceGetCompletedValueWriter(getCompletedValue));

    ID3D12CommandAllocatorResetCommand commandAllocatorReset;
    commandAllocatorReset.key = stateService_.getUniqueCommandKey();
    commandAllocatorReset.object_.key = commandAllocatorKey_;
    stateService_.getRecorder().record(
        new ID3D12CommandAllocatorResetWriter(commandAllocatorReset));

    ID3D12GraphicsCommandListResetCommand commandListReset;
    commandListReset.key = stateService_.getUniqueCommandKey();
    commandListReset.object_.key = commandListKey_;
    commandListReset.pAllocator_.key = commandAllocatorKey_;
    commandListReset.pInitialState_.key = 0;
    stateService_.getRecorder().record(new ID3D12GraphicsCommandListResetWriter(commandListReset));

    IUnknownReleaseCommand releaseCommand;
    releaseCommand.key = stateService_.getUniqueCommandKey();
    releaseCommand.object_.key = uploadResourceKey;
    stateService_.getRecorder().record(new IUnknownReleaseWriter(releaseCommand));

    ++index;
  }

  infoStagingResource->Unmap(0, nullptr);
}

} // namespace DirectX
} // namespace gits
