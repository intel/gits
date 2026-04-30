// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "executionSerializationLayerAuto.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
#include "intelExtensions.h"
#include "keyUtils.h"
#include "configurator.h"
#include "log.h"
#include "exception.h"

#include <string>

namespace gits {
namespace DirectX {

ExecutionSerializationLayer::ExecutionSerializationLayer(ExecutionSerializationRecorder& recorder)
    : Layer("ExecutionSerialization"),
      m_Recorder(recorder),
      m_ExecutionService(recorder, m_CpuDescriptorsService),
      m_CpuDescriptorsService(recorder, m_ExecutionService) {

  const std::string& frames = Configurator::Get().directx.features.subcapture.frames;
  size_t pos = frames.find("-");
  if (pos != std::string::npos) {
    m_EndFrame = std::stoi(frames.substr(pos + 1));
  } else {
    m_EndFrame = std::stoi(frames);
  }
}

ExecutionSerializationLayer::~ExecutionSerializationLayer() {
  try {
    if (InRange()) {
      LOG_ERROR << "Execution serialization recording terminated prematurely";
    }
  } catch (...) {
    topmost_exception_handler("ExecutionSerializationLayer::~ExecutionSerializationLayer");
  }
}

bool ExecutionSerializationLayer::InRange() const {
  return m_CurrentFrame <= m_EndFrame;
}

void ExecutionSerializationLayer::Post(StateRestoreBeginCommand& c) {
  if (InRange()) {
    m_Recorder.Record(StateRestoreBeginSerializer(c));
  }
}

void ExecutionSerializationLayer::Post(StateRestoreEndCommand& c) {
  if (InRange()) {
    m_Recorder.Record(StateRestoreEndSerializer(c));
  }
}

void ExecutionSerializationLayer::Post(FrameEndCommand& c) {
  if (InRange()) {
    m_Recorder.Record(FrameEndSerializer(c));
  }
  if (m_CurrentFrame == m_EndFrame) {
    m_Recorder.FinishRecording();
  }
  ++m_CurrentFrame;
}

void ExecutionSerializationLayer::Post(MarkerUInt64Command& c) {
  if (InRange()) {
    m_Recorder.Record(MarkerUInt64Serializer(c));
  }
}

void ExecutionSerializationLayer::Pre(IDXGISwapChainPresentCommand& c) {
  if (InRange()) {
    m_Recorder.Record(IDXGISwapChainPresentSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(IDXGISwapChain1Present1Command& c) {
  if (InRange()) {
    m_Recorder.Record(IDXGISwapChain1Present1Serializer(c));
  }
}

void ExecutionSerializationLayer::Pre(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (InRange()) {
    m_ExecutionService.ExecuteCommandLists(c.Key, c.m_Object.Key, c.m_ppCommandLists.Keys);
  }
}

void ExecutionSerializationLayer::Pre(ID3D12DeviceCreateCommandListCommand& c) {
  if (InRange()) {
    m_CpuDescriptorsService.createCommandList(c.m_Object.Key);
    m_Recorder.Record(ID3D12DeviceCreateCommandListSerializer(c));
    m_ExecutionService.CreateCommandList(c.m_ppCommandList.Key, c.m_pCommandAllocator.Key);
  }
}

void ExecutionSerializationLayer::Pre(ID3D12Device4CreateCommandList1Command& c) {
  if (InRange()) {
    m_CpuDescriptorsService.createCommandList(c.m_Object.Key);
    m_Recorder.Record(ID3D12Device4CreateCommandList1Serializer(c));
  }
}

void ExecutionSerializationLayer::Pre(ID3D12GraphicsCommandListResetCommand& c) {
  if (InRange()) {
    m_ExecutionService.CommandListReset(c.Key, c.m_Object.Key, c.m_pAllocator.Key);
    m_ExecutionService.CommandListCommand(c.m_Object.Key, c);
  }
}

void ExecutionSerializationLayer::Pre(ID3D12GraphicsCommandListCloseCommand& c) {}

void ExecutionSerializationLayer::Pre(ID3D12CommandQueueWaitCommand& c) {
  if (InRange()) {
    m_ExecutionService.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
  }
}

void ExecutionSerializationLayer::Pre(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {}

void ExecutionSerializationLayer::Post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (InRange()) {
    m_CpuDescriptorsService.preserveDescriptor(c);
    m_ExecutionService.CommandListCommand(c.m_Object.Key, c);
  }
}

void ExecutionSerializationLayer::Pre(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {}

void ExecutionSerializationLayer::Post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (InRange()) {
    m_CpuDescriptorsService.preserveDescriptor(c);
    m_ExecutionService.CommandListCommand(c.m_Object.Key, c);
  }
}

void ExecutionSerializationLayer::Pre(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {}

void ExecutionSerializationLayer::Post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (InRange()) {
    m_CpuDescriptorsService.preserveDescriptor(c);
    m_ExecutionService.CommandListCommand(c.m_Object.Key, c);
  }
}

void ExecutionSerializationLayer::Pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {}

void ExecutionSerializationLayer::Post(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (InRange()) {
    m_CpuDescriptorsService.preserveDescriptor(c);
    m_ExecutionService.CommandListCommand(c.m_Object.Key, c);
  }
}

void ExecutionSerializationLayer::Pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {}

void ExecutionSerializationLayer::Post(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (InRange()) {
    m_CpuDescriptorsService.preserveDescriptor(c);
    m_ExecutionService.CommandListCommand(c.m_Object.Key, c);
  }
}

void ExecutionSerializationLayer::Pre(xessD3D12ExecuteCommand& c) {
  if (InRange()) {
    m_ExecutionService.CommandListCommand(c.m_pCommandList.Key, c);
  }
}

void ExecutionSerializationLayer::Pre(ID3D12CommandQueueSignalCommand& c) {
  if (InRange()) {
    m_ExecutionService.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
  }
}

void ExecutionSerializationLayer::Pre(ID3D12FenceSignalCommand& c) {
  if (InRange()) {
    m_ExecutionService.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
  }
}

void ExecutionSerializationLayer::Pre(ID3D12DeviceCreateFenceCommand& c) {
  if (InRange()) {
    m_Recorder.Record(ID3D12DeviceCreateFenceSerializer(c));
    m_ExecutionService.FenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
  }
}

void ExecutionSerializationLayer::Pre(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (InRange()) {
    m_Recorder.Record(ID3D12Device3EnqueueMakeResidentSerializer(c));
    m_ExecutionService.FenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);

    ID3D12FenceGetCompletedValueCommand getCompletedValue;
    getCompletedValue.Key = m_ExecutionService.GetUniqueCommandKey();
    getCompletedValue.m_Object.Key = c.m_pFenceToSignal.Key;
    getCompletedValue.m_Result.Value = c.m_FenceValueToSignal.Value;
    m_Recorder.Record(ID3D12FenceGetCompletedValueSerializer(getCompletedValue));
  }
}

void ExecutionSerializationLayer::Pre(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (InRange()) {
    m_Recorder.Record(ID3D12DeviceCreateCommandQueueSerializer(c));
    m_ExecutionService.CreateCommandQueue(c.m_Object.Key, c.m_ppCommandQueue.Key);
  }
}

void ExecutionSerializationLayer::Pre(ID3D12Device9CreateCommandQueue1Command& c) {
  if (InRange()) {
    m_Recorder.Record(ID3D12Device9CreateCommandQueue1Serializer(c));
    m_ExecutionService.CreateCommandQueue(c.m_Object.Key, c.m_ppCommandQueue.Key);
  }
}

void ExecutionSerializationLayer::Pre(ID3D12FenceGetCompletedValueCommand& c) {}

void ExecutionSerializationLayer::Pre(ID3D12FenceSetEventOnCompletionCommand& c) {}

void ExecutionSerializationLayer::Pre(CreateWindowMetaCommand& c) {
  if (InRange()) {
    m_Recorder.Record(CreateWindowMetaSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(MappedDataMetaCommand& c) {
  if (InRange()) {
    m_Recorder.Record(MappedDataMetaSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(CreateHeapAllocationMetaCommand& c) {
  if (InRange()) {
    m_Recorder.Record(CreateHeapAllocationMetaSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(WaitForFenceSignaledCommand& c) {}

void ExecutionSerializationLayer::Pre(DllContainerMetaCommand& c) {
  if (InRange()) {
    m_Recorder.Record(DllContainerMetaSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(IUnknownQueryInterfaceCommand& c) {
  if (InRange()) {
    m_Recorder.Record(IUnknownQueryInterfaceSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(IUnknownAddRefCommand& c) {
  if (InRange()) {
    m_Recorder.Record(IUnknownAddRefSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(IUnknownReleaseCommand& c) {
  if (InRange()) {
    m_Recorder.Record(IUnknownReleaseSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  if (InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateDeviceExtensionContextSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  if (InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateDeviceExtensionContext1Serializer(c));
  }
}

void ExecutionSerializationLayer::Pre(INTC_D3D12_SetApplicationInfoCommand& c) {
  if (InRange()) {
    m_Recorder.Record(INTC_D3D12_SetApplicationInfoSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(INTC_DestroyDeviceExtensionContextCommand& c) {
  if (InRange()) {
    m_Recorder.Record(INTC_DestroyDeviceExtensionContextSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(INTC_D3D12_SetFeatureSupportCommand& c) {
  if (InRange()) {
    m_Recorder.Record(INTC_D3D12_SetFeatureSupportSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  if (InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateComputePipelineStateSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(INTC_D3D12_CreatePlacedResourceCommand& c) {
  if (InRange()) {
    m_Recorder.Record(INTC_D3D12_CreatePlacedResourceSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(INTC_D3D12_CreateCommittedResourceCommand& c) {
  if (InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateCommittedResourceSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(INTC_D3D12_CreateReservedResourceCommand& c) {
  if (InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateReservedResourceSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(INTC_D3D12_CreateCommandQueueCommand& c) {
  if (InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateCommandQueueSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(INTC_D3D12_CreateHeapCommand& c) {
  if (InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateHeapSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(IDXGIAdapter3QueryVideoMemoryInfoCommand& c) {}

void ExecutionSerializationLayer::Pre(NvAPI_InitializeCommand& c) {
  if (InRange()) {
    m_Recorder.Record(NvAPI_InitializeSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(NvAPI_UnloadCommand& c) {
  if (InRange()) {
    m_Recorder.Record(NvAPI_UnloadSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) {
  if (InRange()) {
    m_Recorder.Record(NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) {
  if (InRange()) {
    m_Recorder.Record(NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) {
  if (InRange()) {
    m_Recorder.Record(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer(c));
  }
}

void ExecutionSerializationLayer::Pre(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (InRange()) {
    m_ExecutionService.CommandListCommand(c.m_pCommandList.Key, c);
  }
}

void ExecutionSerializationLayer::Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (InRange()) {
    m_ExecutionService.CommandListCommand(c.m_pCommandList.Key, c);
  }
}

void ExecutionSerializationLayer::Pre(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) {
  if (InRange()) {
    m_ExecutionService.CommandListCommand(c.m_pCommandList.Key, c);
  }
}

} // namespace DirectX
} // namespace gits
