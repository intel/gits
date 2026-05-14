// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandListSplitLayerAuto.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
#include "log.h"

namespace gits {
namespace DirectX {

CommandListSplitLayer::CommandListSplitLayer(CommandListSplitRecorder& recorder)
    : Layer("CommandListSplit"), m_Recorder(recorder), m_SplitService(recorder) {}

CommandListSplitLayer::~CommandListSplitLayer() = default;

void CommandListSplitLayer::Pre(StateRestoreBeginCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(StateRestoreBeginSerializer(c));
}

void CommandListSplitLayer::Pre(StateRestoreEndCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(StateRestoreEndSerializer(c));
}

void CommandListSplitLayer::Pre(FrameEndCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(FrameEndSerializer(c));
}

void CommandListSplitLayer::Pre(MarkerUInt64Command& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(MarkerUInt64Serializer(c));
}

void CommandListSplitLayer::Pre(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_SplitService.ExecuteCommandLists(c.m_Object.Key, c.m_ppCommandLists.Keys);
}

void CommandListSplitLayer::Pre(ID3D12DeviceCreateCommandListCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(ID3D12DeviceCreateCommandListSerializer(c));
  m_SplitService.CreateCommandList(c.m_ppCommandList.Key, c.m_pCommandAllocator.Key,
                                   c.m_pInitialState.Key);
}

void CommandListSplitLayer::Pre(ID3D12GraphicsCommandListResetCommand& c) {
  m_SplitService.CommandListReset(c.m_Object.Key, c.m_pAllocator.Key, c.m_pInitialState.Key);
}

void CommandListSplitLayer::Pre(ID3D12GraphicsCommandListCloseCommand& c) {}

void CommandListSplitLayer::Pre(ID3D12CommandAllocatorResetCommand& c) {}

void CommandListSplitLayer::Pre(ID3D12CommandQueueWaitCommand& c) {
  GITS_ASSERT(false, "Invalid command type");
}

void CommandListSplitLayer::Pre(xessD3D12ExecuteCommand& c) {
  m_SplitService.CommandListCommand(c.m_pCommandList.Key, c);
}

void CommandListSplitLayer::Pre(ID3D12CommandQueueSignalCommand& c) {
  m_SplitService.CommandQueueSignal(c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void CommandListSplitLayer::Pre(ID3D12FenceSignalCommand& c) {
  GITS_ASSERT(false, "Invalid command type");
}

void CommandListSplitLayer::Pre(ID3D12FenceGetCompletedValueCommand& c) {}

void CommandListSplitLayer::Pre(ID3D12FenceSetEventOnCompletionCommand& c) {
  GITS_ASSERT(false, "Invalid command type");
}

void CommandListSplitLayer::Pre(CreateWindowMetaCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(CreateWindowMetaSerializer(c));
}

void CommandListSplitLayer::Pre(MappedDataMetaCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(MappedDataMetaSerializer(c));
}

void CommandListSplitLayer::Pre(CreateHeapAllocationMetaCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(CreateHeapAllocationMetaSerializer(c));
}

void CommandListSplitLayer::Pre(WaitForFenceSignaledCommand& c) {}

void CommandListSplitLayer::Pre(DllContainerMetaCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(DllContainerMetaSerializer(c));
}

void CommandListSplitLayer::Pre(IUnknownQueryInterfaceCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(IUnknownQueryInterfaceSerializer(c));
}

void CommandListSplitLayer::Pre(IUnknownAddRefCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(IUnknownAddRefSerializer(c));
}

void CommandListSplitLayer::Pre(IUnknownReleaseCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(IUnknownReleaseSerializer(c));
}

void CommandListSplitLayer::Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(INTC_D3D12_CreateDeviceExtensionContextSerializer(c));
}

void CommandListSplitLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(INTC_D3D12_CreateDeviceExtensionContext1Serializer(c));
}

void CommandListSplitLayer::Pre(INTC_D3D12_SetApplicationInfoCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(INTC_D3D12_SetApplicationInfoSerializer(c));
}

void CommandListSplitLayer::Pre(INTC_DestroyDeviceExtensionContextCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(INTC_DestroyDeviceExtensionContextSerializer(c));
}

void CommandListSplitLayer::Pre(INTC_D3D12_SetFeatureSupportCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(INTC_D3D12_SetFeatureSupportSerializer(c));
}

void CommandListSplitLayer::Pre(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(INTC_D3D12_CreateComputePipelineStateSerializer(c));
}

void CommandListSplitLayer::Pre(INTC_D3D12_CreatePlacedResourceCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(INTC_D3D12_CreatePlacedResourceSerializer(c));
}

void CommandListSplitLayer::Pre(INTC_D3D12_CreateCommittedResourceCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(INTC_D3D12_CreateCommittedResourceSerializer(c));
}

void CommandListSplitLayer::Pre(INTC_D3D12_CreateReservedResourceCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(INTC_D3D12_CreateReservedResourceSerializer(c));
}

void CommandListSplitLayer::Pre(INTC_D3D12_CreateCommandQueueCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(INTC_D3D12_CreateCommandQueueSerializer(c));
}

void CommandListSplitLayer::Pre(INTC_D3D12_CreateHeapCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(INTC_D3D12_CreateHeapSerializer(c));
}

void CommandListSplitLayer::Pre(NvAPI_InitializeCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(NvAPI_InitializeSerializer(c));
}

void CommandListSplitLayer::Pre(NvAPI_UnloadCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(NvAPI_UnloadSerializer(c));
}

void CommandListSplitLayer::Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer(c));
}

void CommandListSplitLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer(c));
}

void CommandListSplitLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) {
  m_SplitService.GetKeyAllocator().RemapCommandKey(c.Key);
  m_Recorder.Record(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer(c));
}

void CommandListSplitLayer::Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  m_SplitService.CommandListCommand(c.m_pCommandList.Key, c);
}

void CommandListSplitLayer::Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  m_SplitService.CommandListCommand(c.m_pCommandList.Key, c);
}

void CommandListSplitLayer::Pre(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) {
  m_SplitService.CommandListCommand(c.m_pCommandList.Key, c);
}

} // namespace DirectX
} // namespace gits
