// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "accelerationStructuresSerializeService.h"
#include "stateTrackingService.h"
#include "arguments.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
#include "reservedResourcesService.h"
#include "log.h"
#include "configurationLib.h"

namespace gits {
namespace DirectX {

AccelerationStructuresSerializeService::AccelerationStructuresSerializeService(
    StateTrackingService& stateService, SubcaptureRecorder& recorder)
    : m_StateService(stateService), m_Recorder(recorder) {
  m_SerializeMode = Configurator::Get().directx.features.subcapture.serializeAccelerationStructures;
}

void AccelerationStructuresSerializeService::BuildAccelerationStructure(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (!m_SerializeMode ||
      c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }
  AccelerationStructure as{};
  as.CallKey = c.Key;
  as.Key = c.m_pDesc.DestAccelerationStructureKey;
  as.Offset = c.m_pDesc.DestAccelerationStructureOffset;
  as.Address = c.m_pDesc.Value->DestAccelerationStructureData;
  m_AccelerationStructuresByCommandList[c.m_Object.Key]
                                       [c.m_pDesc.Value->DestAccelerationStructureData] = as;
  m_AccelerationStructuresByResource[c.m_pDesc.DestAccelerationStructureKey].insert(
      c.m_pDesc.Value->DestAccelerationStructureData);
}

void AccelerationStructuresSerializeService::CopyAccelerationStructure(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (!m_SerializeMode) {
    return;
  }
  AccelerationStructure as{};
  as.CallKey = c.Key;
  as.Key = c.m_DestAccelerationStructureData.InterfaceKey;
  as.Offset = c.m_DestAccelerationStructureData.Offset;
  as.Address = c.m_DestAccelerationStructureData.Value;
  m_AccelerationStructuresByCommandList[c.m_Object.Key][c.m_DestAccelerationStructureData.Value] =
      as;
  m_AccelerationStructuresByResource[c.m_DestAccelerationStructureData.InterfaceKey].insert(
      c.m_DestAccelerationStructureData.Value);
}

void AccelerationStructuresSerializeService::ExecuteCommandLists(
    ID3D12CommandQueueExecuteCommandListsCommand& c) {
  for (unsigned commandListKey : c.m_ppCommandLists.Keys) {
    auto itByKey = m_AccelerationStructuresByCommandList.find(commandListKey);
    if (itByKey != m_AccelerationStructuresByCommandList.end()) {
      for (auto& it : itByKey->second) {
        m_AccelerationStructures[it.first] = it.second;
      }
      m_AccelerationStructuresByCommandList.erase(itByKey);
    }
  }
}

void AccelerationStructuresSerializeService::DestroyResource(unsigned resourceKey) {
  if (!m_SerializeMode) {
    return;
  }
  auto itResource = m_AccelerationStructuresByResource.find(resourceKey);
  if (itResource == m_AccelerationStructuresByResource.end()) {
    return;
  }
  for (D3D12_GPU_VIRTUAL_ADDRESS address : itResource->second) {
    m_AccelerationStructures.erase(address);
  }
  m_AccelerationStructuresByResource.erase(itResource);
}

void AccelerationStructuresSerializeService::RestoreAccelerationStructures() {
  if (!m_SerializeMode) {
    return;
  }
  {
    D3D12_COMMAND_QUEUE_DESC commandQueueDirectDesc{};
    commandQueueDirectDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    HRESULT hr =
        m_Device->CreateCommandQueue(&commandQueueDirectDesc, IID_PPV_ARGS(&m_CommandQueue));
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
    CreateCommandQueue.m_Object.Key = m_DeviceKey;
    CreateCommandQueue.m_pDesc.Value = &commandQueueDesc;
    CreateCommandQueue.m_riid.Value = IID_ID3D12CommandQueue;
    CreateCommandQueue.m_ppCommandQueue.Key = m_CommandQueueKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandQueueSerializer(CreateCommandQueue));

    m_CommandAllocatorKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
    createCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
    createCommandAllocator.m_Object.Key = m_DeviceKey;
    createCommandAllocator.m_type.Value = D3D12_COMMAND_LIST_TYPE_COPY;
    createCommandAllocator.m_riid.Value = IID_ID3D12CommandAllocator;
    createCommandAllocator.m_ppCommandAllocator.Key = m_CommandAllocatorKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandAllocatorSerializer(createCommandAllocator));

    m_CommandListKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandListCommand createCommandList;
    createCommandList.Key = m_StateService.GetUniqueCommandKey();
    createCommandList.m_Object.Key = m_DeviceKey;
    createCommandList.m_nodeMask.Value = 0;
    createCommandList.m_pCommandAllocator.Key = createCommandAllocator.m_ppCommandAllocator.Key;
    createCommandList.m_type.Value = D3D12_COMMAND_LIST_TYPE_COPY;
    createCommandList.m_pInitialState.Value = nullptr;
    createCommandList.m_riid.Value = IID_ID3D12GraphicsCommandList4;
    createCommandList.m_ppCommandList.Key = m_CommandListKey;
    m_StateService.GetRecorder().Record(ID3D12DeviceCreateCommandListSerializer(createCommandList));

    m_FenceKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateFenceCommand createFence;
    createFence.Key = m_StateService.GetUniqueCommandKey();
    createFence.m_Object.Key = m_DeviceKey;
    createFence.m_InitialValue.Value = 0;
    createFence.m_Flags.Value = D3D12_FENCE_FLAG_NONE;
    createFence.m_riid.Value = IID_ID3D12Fence;
    createFence.m_ppFence.Key = m_FenceKey;
    m_StateService.GetRecorder().Record(ID3D12DeviceCreateFenceSerializer(createFence));
  }

  unsigned infoBufferSize =
      sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION_DESC) *
      m_AccelerationStructures.size();
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
  HRESULT hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
                                                 &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
                                                 nullptr, IID_PPV_ARGS(&infoResource));
  GITS_ASSERT(hr == S_OK);

  Microsoft::WRL::ComPtr<ID3D12Resource> infoStagingResource;
  heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_COMMON, nullptr,
                                         IID_PPV_ARGS(&infoStagingResource));
  GITS_ASSERT(hr == S_OK);

  std::map<unsigned, AccelerationStructure> accelerationStructuresByCallKey;
  for (auto& it : m_AccelerationStructures) {
    accelerationStructuresByCallKey[it.second.CallKey] = it.second;
  }

  std::vector<D3D12_GPU_VIRTUAL_ADDRESS> destAdresses(m_AccelerationStructures.size());
  unsigned index = 0;
  for (auto& it : accelerationStructuresByCallKey) {
    destAdresses[index] = it.second.Address;
    ++index;
  }

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC desc{};
  desc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION;
  desc.DestBuffer = infoResource->GetGPUVirtualAddress();
  m_CommandList->EmitRaytracingAccelerationStructurePostbuildInfo(&desc, destAdresses.size(),
                                                                  destAdresses.data());
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = infoResource.Get();
  barrier.Transition.Subresource = 0;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  m_CommandList->ResourceBarrier(1, &barrier);

  m_CommandList->CopyResource(infoStagingResource.Get(), infoResource.Get());
  m_CommandList->Close();
  ID3D12CommandList* commandLists[] = {m_CommandList};
  m_CommandQueue->ExecuteCommandLists(1, commandLists);
  hr = m_CommandQueue->Signal(m_Fence, ++m_CurrentFenceValue);
  GITS_ASSERT(hr == S_OK);
  hr = m_Fence->SetEventOnCompletion(m_CurrentFenceValue, NULL);
  GITS_ASSERT(hr == S_OK);

  hr = m_CommandAllocator->Reset();
  GITS_ASSERT(hr == S_OK);
  hr = m_CommandList->Reset(m_CommandAllocator, nullptr);
  GITS_ASSERT(hr == S_OK);

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION_DESC* infoData{};
  hr = infoStagingResource->Map(0, nullptr, reinterpret_cast<void**>(&infoData));
  GITS_ASSERT(hr == S_OK);

  index = 0;
  for (auto& it : accelerationStructuresByCallKey) {

    if (infoData[index].NumBottomLevelAccelerationStructurePointers) {
      continue;
    }

    unsigned DestKey = it.second.Key;
    unsigned DestOffset = it.second.Offset;

    // serialize acceleration structure

    Microsoft::WRL::ComPtr<ID3D12Resource> serializedResource;
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    resourceDesc.Width = infoData[index].SerializedSizeInBytes;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                           D3D12_RESOURCE_STATE_COMMON, nullptr,
                                           IID_PPV_ARGS(&serializedResource));
    GITS_ASSERT(hr == S_OK);

    Microsoft::WRL::ComPtr<ID3D12Resource> serializedStagingResource;
    heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
    resourceDesc.Width = infoData[index].SerializedSizeInBytes;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                           D3D12_RESOURCE_STATE_COMMON, nullptr,
                                           IID_PPV_ARGS(&serializedStagingResource));
    GITS_ASSERT(hr == S_OK);

    m_CommandList->CopyRaytracingAccelerationStructure(
        serializedResource->GetGPUVirtualAddress(), destAdresses[index],
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = serializedResource.Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    m_CommandList->ResourceBarrier(1, &barrier);

    m_CommandList->CopyResource(serializedStagingResource.Get(), serializedResource.Get());
    m_CommandList->Close();
    ID3D12CommandList* commandLists[] = {m_CommandList};
    m_CommandQueue->ExecuteCommandLists(1, commandLists);
    hr = m_CommandQueue->Signal(m_Fence, ++m_CurrentFenceValue);
    GITS_ASSERT(hr == S_OK);
    hr = m_Fence->SetEventOnCompletion(m_CurrentFenceValue, NULL);
    GITS_ASSERT(hr == S_OK);

    hr = m_CommandAllocator->Reset();
    GITS_ASSERT(hr == S_OK);
    hr = m_CommandList->Reset(m_CommandAllocator, nullptr);
    GITS_ASSERT(hr == S_OK);

    // create upload resource with serialize acceleration structure in subcaptured stream

    unsigned uploadResourceKey = m_StateService.GetUniqueObjectKey();
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    ID3D12DeviceCreateCommittedResourceCommand createUploadResource;
    createUploadResource.Key = m_StateService.GetUniqueCommandKey();
    createUploadResource.m_Object.Key = m_DeviceKey;
    createUploadResource.m_pHeapProperties.Value = &heapProperties;
    createUploadResource.m_HeapFlags.Value = D3D12_HEAP_FLAG_NONE;
    createUploadResource.m_pDesc.Value = &resourceDesc;
    createUploadResource.m_InitialResourceState.Value = D3D12_RESOURCE_STATE_GENERIC_READ;
    createUploadResource.m_pOptimizedClearValue.Value = nullptr;
    createUploadResource.m_riidResource.Value = IID_ID3D12Resource;
    createUploadResource.m_ppvResource.Key = uploadResourceKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommittedResourceSerializer(createUploadResource));

    void* mappedData{};
    hr = serializedStagingResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
    GITS_ASSERT(hr == S_OK);

    ID3D12ResourceMapCommand mapCommand;
    mapCommand.Key = m_StateService.GetUniqueCommandKey();
    mapCommand.m_Object.Key = uploadResourceKey;
    mapCommand.m_Subresource.Value = 0;
    mapCommand.m_pReadRange.Value = nullptr;
    mapCommand.m_ppData.CaptureValue = m_StateService.GetUniqueFakePointer();
    mapCommand.m_ppData.Value = &mapCommand.m_ppData.CaptureValue;
    m_StateService.GetRecorder().Record(ID3D12ResourceMapSerializer(mapCommand));

    MappedDataMetaCommand metaCommand;
    metaCommand.Key = m_StateService.GetUniqueCommandKey();
    metaCommand.m_resource.Key = uploadResourceKey;
    metaCommand.m_mappedAddress.Value = mapCommand.m_ppData.CaptureValue;
    metaCommand.m_offset.Value = 0;
    metaCommand.m_data.Value = mappedData;
    metaCommand.m_data.Size = resourceDesc.Width;
    m_StateService.GetRecorder().Record(MappedDataMetaSerializer(metaCommand));

    ID3D12ResourceUnmapCommand unmapCommand;
    unmapCommand.Key = m_StateService.GetUniqueCommandKey();
    unmapCommand.m_Object.Key = uploadResourceKey;
    unmapCommand.m_Subresource.Value = 0;
    unmapCommand.m_pWrittenRange.Value = nullptr;
    m_StateService.GetRecorder().Record(ID3D12ResourceUnmapSerializer(unmapCommand));

    serializedStagingResource->Unmap(0, nullptr);

    // deserialize acceleration structure in subcaptured stream

    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand copyASCommand;
    copyASCommand.Key = m_StateService.GetUniqueCommandKey();
    copyASCommand.m_Object.Key = m_CommandListKey;
    copyASCommand.m_DestAccelerationStructureData.InterfaceKey = DestKey;
    copyASCommand.m_DestAccelerationStructureData.Offset = DestOffset;
    copyASCommand.m_SourceAccelerationStructureData.InterfaceKey = uploadResourceKey;
    copyASCommand.m_SourceAccelerationStructureData.Offset = 0;
    copyASCommand.m_Mode.Value = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE;
    m_StateService.GetRecorder().Record(
        ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureSerializer(copyASCommand));

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
    m_StateService.GetRecorder().Record(
        ID3D12CommandAllocatorResetSerializer(commandAllocatorReset));

    ID3D12GraphicsCommandListResetCommand CommandListReset;
    CommandListReset.Key = m_StateService.GetUniqueCommandKey();
    CommandListReset.m_Object.Key = m_CommandListKey;
    CommandListReset.m_pAllocator.Key = m_CommandAllocatorKey;
    CommandListReset.m_pInitialState.Key = 0;
    m_StateService.GetRecorder().Record(ID3D12GraphicsCommandListResetSerializer(CommandListReset));

    IUnknownReleaseCommand releaseCommand;
    releaseCommand.Key = m_StateService.GetUniqueCommandKey();
    releaseCommand.m_Object.Key = uploadResourceKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseCommand));

    ++index;
  }

  infoStagingResource->Unmap(0, nullptr);
}

} // namespace DirectX
} // namespace gits
