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
      recorder_(recorder),
      executionService_(recorder, cpuDescriptorsService_),
      cpuDescriptorsService_(recorder, executionService_) {

  const std::string& frames = Configurator::Get().directx.features.subcapture.frames;
  size_t pos = frames.find("-");
  if (pos != std::string::npos) {
    endFrame_ = std::stoi(frames.substr(pos + 1));
  } else {
    endFrame_ = std::stoi(frames);
  }
}

ExecutionSerializationLayer::~ExecutionSerializationLayer() {
  try {
    if (inRange()) {
      LOG_ERROR << "Execution serialization recording terminated prematurely";
    }
  } catch (...) {
    topmost_exception_handler("ExecutionSerializationLayer::~ExecutionSerializationLayer");
  }
}

bool ExecutionSerializationLayer::inRange() const {
  return currentFrame_ <= endFrame_;
}

void ExecutionSerializationLayer::post(StateRestoreBeginCommand& c) {
  if (inRange()) {
    recorder_.record(StateRestoreBeginSerializer(c));
  }
}

void ExecutionSerializationLayer::post(StateRestoreEndCommand& c) {
  if (inRange()) {
    recorder_.record(StateRestoreEndSerializer(c));
  }
}

void ExecutionSerializationLayer::post(FrameEndCommand& c) {
  if (inRange()) {
    recorder_.record(FrameEndSerializer(c));
  }
  if (currentFrame_ == endFrame_) {
    recorder_.finishRecording();
  }
  ++currentFrame_;
}

void ExecutionSerializationLayer::post(MarkerUInt64Command& c) {
  if (inRange()) {
    recorder_.record(MarkerUInt64Serializer(c));
  }
}

void ExecutionSerializationLayer::pre(IDXGISwapChainPresentCommand& c) {
  if (inRange()) {
    recorder_.record(IDXGISwapChainPresentSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(IDXGISwapChain1Present1Command& c) {
  if (inRange()) {
    recorder_.record(IDXGISwapChain1Present1Serializer(c));
  }
}

void ExecutionSerializationLayer::pre(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (inRange()) {
    executionService_.executeCommandLists(c.key, c.object_.key, c.ppCommandLists_.keys);
  }
}

void ExecutionSerializationLayer::pre(ID3D12DeviceCreateCommandListCommand& c) {
  if (inRange()) {
    cpuDescriptorsService_.createCommandList(c.object_.key);
    recorder_.record(ID3D12DeviceCreateCommandListSerializer(c));
    executionService_.createCommandList(c.ppCommandList_.key, c.pCommandAllocator_.key);
  }
}

void ExecutionSerializationLayer::pre(ID3D12Device4CreateCommandList1Command& c) {
  if (inRange()) {
    cpuDescriptorsService_.createCommandList(c.object_.key);
    recorder_.record(ID3D12Device4CreateCommandList1Serializer(c));
  }
}

void ExecutionSerializationLayer::pre(ID3D12GraphicsCommandListResetCommand& c) {
  if (inRange()) {
    executionService_.commandListReset(c.key, c.object_.key, c.pAllocator_.key);
    executionService_.commandListCommand(c.object_.key, c);
  }
}

void ExecutionSerializationLayer::pre(ID3D12GraphicsCommandListCloseCommand& c) {}

void ExecutionSerializationLayer::pre(ID3D12CommandQueueWaitCommand& c) {
  if (inRange()) {
    executionService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  }
}

void ExecutionSerializationLayer::pre(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {}

void ExecutionSerializationLayer::post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (inRange()) {
    cpuDescriptorsService_.preserveDescriptor(c);
    executionService_.commandListCommand(c.object_.key, c);
  }
}

void ExecutionSerializationLayer::pre(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {}

void ExecutionSerializationLayer::post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (inRange()) {
    cpuDescriptorsService_.preserveDescriptor(c);
    executionService_.commandListCommand(c.object_.key, c);
  }
}

void ExecutionSerializationLayer::pre(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {}

void ExecutionSerializationLayer::post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (inRange()) {
    cpuDescriptorsService_.preserveDescriptor(c);
    executionService_.commandListCommand(c.object_.key, c);
  }
}

void ExecutionSerializationLayer::pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {}

void ExecutionSerializationLayer::post(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (inRange()) {
    cpuDescriptorsService_.preserveDescriptor(c);
    executionService_.commandListCommand(c.object_.key, c);
  }
}

void ExecutionSerializationLayer::pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {}

void ExecutionSerializationLayer::post(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (inRange()) {
    cpuDescriptorsService_.preserveDescriptor(c);
    executionService_.commandListCommand(c.object_.key, c);
  }
}

void ExecutionSerializationLayer::pre(xessD3D12ExecuteCommand& c) {
  if (inRange()) {
    executionService_.commandListCommand(c.pCommandList_.key, c);
  }
}

void ExecutionSerializationLayer::pre(ID3D12CommandQueueSignalCommand& c) {
  if (inRange()) {
    executionService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  }
}

void ExecutionSerializationLayer::pre(ID3D12FenceSignalCommand& c) {
  if (inRange()) {
    executionService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  }
}

void ExecutionSerializationLayer::pre(ID3D12DeviceCreateFenceCommand& c) {
  if (inRange()) {
    recorder_.record(ID3D12DeviceCreateFenceSerializer(c));
    executionService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
  }
}

void ExecutionSerializationLayer::pre(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (inRange()) {
    recorder_.record(ID3D12Device3EnqueueMakeResidentSerializer(c));
    executionService_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);

    ID3D12FenceGetCompletedValueCommand getCompletedValue;
    getCompletedValue.key = executionService_.getUniqueCommandKey();
    getCompletedValue.object_.key = c.pFenceToSignal_.key;
    getCompletedValue.result_.value = c.FenceValueToSignal_.value;
    recorder_.record(ID3D12FenceGetCompletedValueSerializer(getCompletedValue));
  }
}

void ExecutionSerializationLayer::pre(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (inRange()) {
    recorder_.record(ID3D12DeviceCreateCommandQueueSerializer(c));
    executionService_.createCommandQueue(c.object_.key, c.ppCommandQueue_.key);
  }
}

void ExecutionSerializationLayer::pre(ID3D12Device9CreateCommandQueue1Command& c) {
  if (inRange()) {
    recorder_.record(ID3D12Device9CreateCommandQueue1Serializer(c));
    executionService_.createCommandQueue(c.object_.key, c.ppCommandQueue_.key);
  }
}

void ExecutionSerializationLayer::pre(ID3D12FenceGetCompletedValueCommand& c) {}

void ExecutionSerializationLayer::pre(ID3D12FenceSetEventOnCompletionCommand& c) {}

void ExecutionSerializationLayer::pre(CreateWindowMetaCommand& c) {
  if (inRange()) {
    recorder_.record(CreateWindowMetaSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(MappedDataMetaCommand& c) {
  if (inRange()) {
    recorder_.record(MappedDataMetaSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(CreateHeapAllocationMetaCommand& c) {
  if (inRange()) {
    recorder_.record(CreateHeapAllocationMetaSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(WaitForFenceSignaledCommand& c) {}

void ExecutionSerializationLayer::pre(DllContainerMetaCommand& c) {
  if (inRange()) {
    recorder_.record(DllContainerMetaSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(IUnknownQueryInterfaceCommand& c) {
  if (inRange()) {
    recorder_.record(IUnknownQueryInterfaceSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(IUnknownAddRefCommand& c) {
  if (inRange()) {
    recorder_.record(IUnknownAddRefSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(IUnknownReleaseCommand& c) {
  if (inRange()) {
    recorder_.record(IUnknownReleaseSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  if (inRange()) {
    recorder_.record(INTC_D3D12_CreateDeviceExtensionContextSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  if (inRange()) {
    recorder_.record(INTC_D3D12_CreateDeviceExtensionContext1Serializer(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_SetApplicationInfoCommand& c) {
  if (inRange()) {
    recorder_.record(INTC_D3D12_SetApplicationInfoSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_DestroyDeviceExtensionContextCommand& c) {
  if (inRange()) {
    recorder_.record(INTC_DestroyDeviceExtensionContextSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_SetFeatureSupportCommand& c) {
  if (inRange()) {
    recorder_.record(INTC_D3D12_SetFeatureSupportSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  if (inRange()) {
    recorder_.record(INTC_D3D12_CreateComputePipelineStateSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreatePlacedResourceCommand& c) {
  if (inRange()) {
    recorder_.record(INTC_D3D12_CreatePlacedResourceSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateCommittedResourceCommand& c) {
  if (inRange()) {
    recorder_.record(INTC_D3D12_CreateCommittedResourceSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateReservedResourceCommand& c) {
  if (inRange()) {
    recorder_.record(INTC_D3D12_CreateReservedResourceSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateCommandQueueCommand& c) {
  if (inRange()) {
    recorder_.record(INTC_D3D12_CreateCommandQueueSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateHeapCommand& c) {
  if (inRange()) {
    recorder_.record(INTC_D3D12_CreateHeapSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(IDXGIAdapter3QueryVideoMemoryInfoCommand& c) {}

void ExecutionSerializationLayer::pre(NvAPI_InitializeCommand& c) {
  if (inRange()) {
    recorder_.record(NvAPI_InitializeSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(NvAPI_UnloadCommand& c) {
  if (inRange()) {
    recorder_.record(NvAPI_UnloadSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) {
  if (inRange()) {
    recorder_.record(NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) {
  if (inRange()) {
    recorder_.record(NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) {
  if (inRange()) {
    recorder_.record(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer(c));
  }
}

void ExecutionSerializationLayer::pre(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (inRange()) {
    executionService_.commandListCommand(c.pCommandList_.key, c);
  }
}

void ExecutionSerializationLayer::pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (inRange()) {
    executionService_.commandListCommand(c.pCommandList_.key, c);
  }
}

void ExecutionSerializationLayer::pre(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) {
  if (inRange()) {
    executionService_.commandListCommand(c.pCommandList_.key, c);
  }
}

} // namespace DirectX
} // namespace gits
