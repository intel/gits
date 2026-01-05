// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "executionSerializationLayerAuto.h"
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"
#include "intelExtensions.h"
#include "keyUtils.h"

namespace gits {
namespace DirectX {

void ExecutionSerializationLayer::post(StateRestoreBeginCommand& c) {
  recorder_.record(new CTokenMarker(CToken::ID_INIT_START));
}

void ExecutionSerializationLayer::post(StateRestoreEndCommand& c) {
  recorder_.record(new CTokenMarker(CToken::ID_INIT_END));
}

void ExecutionSerializationLayer::post(MarkerUInt64Command& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new CTokenMarkerUInt64(c.value_));
  }
}

void ExecutionSerializationLayer::pre(IDXGISwapChainPresentCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new IDXGISwapChainPresentWriter(c));
  }
  if (!(c.Flags_.value & DXGI_PRESENT_TEST) && !isStateRestoreKey(c.key)) {
    recorder_.frameEnd();
  }
}

void ExecutionSerializationLayer::pre(IDXGISwapChain1Present1Command& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new IDXGISwapChain1Present1Writer(c));
  }
  if (!(c.PresentFlags_.value & DXGI_PRESENT_TEST) && !isStateRestoreKey(c.key)) {
    recorder_.frameEnd();
  }
}

void ExecutionSerializationLayer::pre(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (recorder_.isRunning()) {
    executionService_.executeCommandLists(c.key, c.object_.key, c.ppCommandLists_.keys);
  }
}

void ExecutionSerializationLayer::pre(ID3D12DeviceCreateCommandListCommand& c) {
  if (recorder_.isRunning()) {
    cpuDescriptorsService_.createCommandList(c.object_.key);
    recorder_.record(new ID3D12DeviceCreateCommandListWriter(c));
    executionService_.createCommandList(c.ppCommandList_.key, c.pCommandAllocator_.key);
  }
}

void ExecutionSerializationLayer::pre(ID3D12Device4CreateCommandList1Command& c) {
  if (recorder_.isRunning()) {
    cpuDescriptorsService_.createCommandList(c.object_.key);
    recorder_.record(new ID3D12Device4CreateCommandList1Writer(c));
  }
}

void ExecutionSerializationLayer::pre(ID3D12GraphicsCommandListResetCommand& c) {
  if (recorder_.isRunning()) {
    executionService_.commandListReset(c.key, c.object_.key, c.pAllocator_.key);
    executionService_.commandListCommand(c.object_.key,
                                         new ID3D12GraphicsCommandListResetWriter(c));
  }
}

void ExecutionSerializationLayer::pre(ID3D12GraphicsCommandListCloseCommand& c) {}

void ExecutionSerializationLayer::pre(ID3D12CommandQueueWaitCommand& c) {
  if (recorder_.isRunning()) {
    executionService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  }
}

void ExecutionSerializationLayer::pre(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {}

void ExecutionSerializationLayer::post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (recorder_.isRunning()) {
    cpuDescriptorsService_.preserveDescriptor(c);
    executionService_.commandListCommand(c.object_.key,
                                         new ID3D12GraphicsCommandListOMSetRenderTargetsWriter(c));
  }
}

void ExecutionSerializationLayer::pre(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {}

void ExecutionSerializationLayer::post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (recorder_.isRunning()) {
    cpuDescriptorsService_.preserveDescriptor(c);
    executionService_.commandListCommand(
        c.object_.key, new ID3D12GraphicsCommandListClearDepthStencilViewWriter(c));
  }
}

void ExecutionSerializationLayer::pre(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {}

void ExecutionSerializationLayer::post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (recorder_.isRunning()) {
    cpuDescriptorsService_.preserveDescriptor(c);
    executionService_.commandListCommand(
        c.object_.key, new ID3D12GraphicsCommandListClearRenderTargetViewWriter(c));
  }
}

void ExecutionSerializationLayer::pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {}

void ExecutionSerializationLayer::post(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (recorder_.isRunning()) {
    cpuDescriptorsService_.preserveDescriptor(c);
    executionService_.commandListCommand(
        c.object_.key, new ID3D12GraphicsCommandListClearUnorderedAccessViewUintWriter(c));
  }
}

void ExecutionSerializationLayer::pre(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {}

void ExecutionSerializationLayer::post(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (recorder_.isRunning()) {
    cpuDescriptorsService_.preserveDescriptor(c);
    executionService_.commandListCommand(
        c.object_.key, new ID3D12GraphicsCommandListClearUnorderedAccessViewFloatWriter(c));
  }
}

void ExecutionSerializationLayer::pre(xessD3D12ExecuteCommand& c) {
  if (recorder_.isRunning()) {
    executionService_.commandListCommand(c.pCommandList_.key, new xessD3D12ExecuteWriter(c));
  }
}

void ExecutionSerializationLayer::pre(ID3D12CommandQueueSignalCommand& c) {
  if (recorder_.isRunning()) {
    executionService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  }
}

void ExecutionSerializationLayer::pre(ID3D12FenceSignalCommand& c) {
  if (recorder_.isRunning()) {
    executionService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  }
}

void ExecutionSerializationLayer::pre(ID3D12DeviceCreateFenceCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new ID3D12DeviceCreateFenceWriter(c));
    executionService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
  }
}

void ExecutionSerializationLayer::pre(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new ID3D12Device3EnqueueMakeResidentWriter(c));
    executionService_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);

    ID3D12FenceGetCompletedValueCommand getCompletedValue;
    getCompletedValue.key = executionService_.getUniqueCommandKey();
    getCompletedValue.object_.key = c.pFenceToSignal_.key;
    getCompletedValue.result_.value = c.FenceValueToSignal_.value;
    recorder_.record(new ID3D12FenceGetCompletedValueWriter(getCompletedValue));
  }
}

void ExecutionSerializationLayer::pre(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new ID3D12DeviceCreateCommandQueueWriter(c));
    executionService_.createCommandQueue(c.object_.key, c.ppCommandQueue_.key);
  }
}

void ExecutionSerializationLayer::pre(ID3D12Device9CreateCommandQueue1Command& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new ID3D12Device9CreateCommandQueue1Writer(c));
    executionService_.createCommandQueue(c.object_.key, c.ppCommandQueue_.key);
  }
}

void ExecutionSerializationLayer::pre(ID3D12FenceGetCompletedValueCommand& c) {}

void ExecutionSerializationLayer::pre(ID3D12FenceSetEventOnCompletionCommand& c) {}

void ExecutionSerializationLayer::pre(CreateWindowMetaCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new CreateWindowMetaWriter(c));
  }
}

void ExecutionSerializationLayer::pre(MappedDataMetaCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new MappedDataMetaWriter(c));
  }
}

void ExecutionSerializationLayer::pre(CreateHeapAllocationMetaCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new CreateHeapAllocationMetaWriter(c));
  }
}

void ExecutionSerializationLayer::pre(WaitForFenceSignaledCommand& c) {}

void ExecutionSerializationLayer::pre(DllContainerMetaCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new DllContainerMetaWriter(c));
  }
}

void ExecutionSerializationLayer::pre(IUnknownQueryInterfaceCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new IUnknownQueryInterfaceWriter(c));
  }
}

void ExecutionSerializationLayer::pre(IUnknownAddRefCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new IUnknownAddRefWriter(c));
  }
}

void ExecutionSerializationLayer::pre(IUnknownReleaseCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new IUnknownReleaseWriter(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateDeviceExtensionContextWriter(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateDeviceExtensionContext1Writer(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_SetApplicationInfoCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_SetApplicationInfoWriter(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_DestroyDeviceExtensionContextCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_DestroyDeviceExtensionContextWriter(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_SetFeatureSupportCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_SetFeatureSupportWriter(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateComputePipelineStateWriter(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreatePlacedResourceCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreatePlacedResourceWriter(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateCommittedResourceCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateCommittedResourceWriter(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateReservedResourceCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateReservedResourceWriter(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateCommandQueueCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateCommandQueueWriter(c));
  }
}

void ExecutionSerializationLayer::pre(INTC_D3D12_CreateHeapCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateHeapWriter(c));
  }
}

void ExecutionSerializationLayer::pre(IDXGIAdapter3QueryVideoMemoryInfoCommand& c) {}

void ExecutionSerializationLayer::pre(NvAPI_InitializeCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new NvAPI_InitializeWriter(c));
  }
}

void ExecutionSerializationLayer::pre(NvAPI_UnloadCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new NvAPI_UnloadWriter(c));
  }
}

void ExecutionSerializationLayer::pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new NvAPI_D3D12_SetCreatePipelineStateOptionsWriter(c));
  }
}

void ExecutionSerializationLayer::pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new NvAPI_D3D12_SetNvShaderExtnSlotSpaceWriter(c));
  }
}

void ExecutionSerializationLayer::pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) {
  if (recorder_.isRunning()) {
    recorder_.record(new NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadWriter(c));
  }
}

void ExecutionSerializationLayer::pre(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (recorder_.isRunning()) {
    executionService_.commandListCommand(
        c.pCommandList_.key, new NvAPI_D3D12_BuildRaytracingAccelerationStructureExWriter(c));
  }
}

void ExecutionSerializationLayer::pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (recorder_.isRunning()) {
    executionService_.commandListCommand(
        c.pCommandList_.key, new NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayWriter(c));
  }
}

void ExecutionSerializationLayer::pre(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) {
  if (recorder_.isRunning()) {
    executionService_.commandListCommand(
        c.pCommandList_.key,
        new NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationWriter(c));
  }
}

} // namespace DirectX
} // namespace gits
