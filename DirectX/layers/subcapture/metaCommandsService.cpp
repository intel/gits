// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "metaCommandsService.h"
#include "stateTrackingService.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"

namespace gits {
namespace DirectX {

void MetaCommandsService::InitializeMetaCommand(
    ID3D12GraphicsCommandList4InitializeMetaCommandCommand& command) {
  std::vector<uint8_t>& initializationData = m_MetaCommandData[command.m_pMetaCommand.Key];
  initializationData.resize(command.m_InitializationParametersDataSizeInBytes.Value);
  memcpy(initializationData.data(), command.m_pInitializationParametersData.Value,
         command.m_InitializationParametersDataSizeInBytes.Value);
}

void MetaCommandsService::SetDeviceKey(unsigned deviceKey) {
  if (m_DeviceKey && m_DeviceKey != deviceKey) {
    LOG_ERROR << "MetaCommandsService - multiple devices not supported!";
  }
  m_DeviceKey = deviceKey;
}

void MetaCommandsService::DestroyMetaCommand(unsigned key) {
  m_MetaCommandData.erase(key);
}

void MetaCommandsService::RestoreState() {
  RestoreStateInitialize();

  for (auto& [metaCommandKey, initializationData] : m_MetaCommandData) {
    ID3D12GraphicsCommandList4InitializeMetaCommandCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_Object.Key = m_CommandListKey;
    c.m_pMetaCommand.Key = metaCommandKey;
    c.m_pInitializationParametersData.Value = initializationData.data();
    c.m_pInitializationParametersData.Size = initializationData.size();
    c.m_InitializationParametersDataSizeInBytes.Value = initializationData.size();
    m_StateService.GetRecorder().Record(
        ID3D12GraphicsCommandList4InitializeMetaCommandSerializer(c));
  }
  m_MetaCommandData.clear();

  RestoreStateFinalize();
}

void MetaCommandsService::RestoreStateInitialize() {
  {
    m_CommandQueueKey = m_StateService.GetUniqueObjectKey();
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ID3D12DeviceCreateCommandQueueCommand createCommandQueue;
    createCommandQueue.Key = m_StateService.GetUniqueCommandKey();
    createCommandQueue.m_Object.Key = m_DeviceKey;
    createCommandQueue.m_pDesc.Value = &commandQueueDesc;
    createCommandQueue.m_riid.Value = IID_ID3D12CommandQueue;
    createCommandQueue.m_ppCommandQueue.Key = m_CommandQueueKey;
    m_StateService.GetRecorder().Record(
        ID3D12DeviceCreateCommandQueueSerializer(createCommandQueue));

    m_CommandAllocatorKey = m_StateService.GetUniqueObjectKey();
    ID3D12DeviceCreateCommandAllocatorCommand createCommandAllocator;
    createCommandAllocator.Key = m_StateService.GetUniqueCommandKey();
    createCommandAllocator.m_Object.Key = m_DeviceKey;
    createCommandAllocator.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
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
    createCommandList.m_type.Value = D3D12_COMMAND_LIST_TYPE_DIRECT;
    createCommandList.m_pInitialState.Value = nullptr;
    createCommandList.m_riid.Value = IID_ID3D12CommandList;
    createCommandList.m_ppCommandList.Key = m_CommandListKey;
    m_StateService.GetRecorder().Record(ID3D12DeviceCreateCommandListSerializer(createCommandList));
  }
  {
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
}

void MetaCommandsService::RestoreStateFinalize() {
  {
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
    commandQueueSignal.m_Value.Value = 1;
    m_StateService.GetRecorder().Record(ID3D12CommandQueueSignalSerializer(commandQueueSignal));

    ID3D12FenceGetCompletedValueCommand getCompletedValue;
    getCompletedValue.Key = m_StateService.GetUniqueCommandKey();
    getCompletedValue.m_Object.Key = m_FenceKey;
    getCompletedValue.m_Result.Value = 1;
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
  {
    IUnknownReleaseCommand releaseFence{};
    releaseFence.Key = m_StateService.GetUniqueCommandKey();
    releaseFence.m_Object.Key = m_FenceKey;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(releaseFence));
  }
  {
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
  }
}

} // namespace DirectX
} // namespace gits
