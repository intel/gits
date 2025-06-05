// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "recordingLayerAuto.h"
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"
#include "intelExtensions.h"

namespace gits {
namespace DirectX {

void RecordingLayer::post(IDXGISwapChainPresentCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new IDXGISwapChainPresentWriter(command));
  }
  if (!(command.Flags_.value & DXGI_PRESENT_TEST)) {
    recorder_.frameEnd(command.key & Command::stateRestoreKeyMask);
  }
}

void RecordingLayer::post(IDXGISwapChain1Present1Command& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new IDXGISwapChain1Present1Writer(command));
  }
  if (!(command.PresentFlags_.value & DXGI_PRESENT_TEST)) {
    recorder_.frameEnd(command.key & Command::stateRestoreKeyMask);
  }
}

void RecordingLayer::post(ID3D12GraphicsCommandListResetCommand& command) {
  recorder_.executionStart();
  if (recorder_.isRunning()) {
    recorder_.record(new ID3D12GraphicsCommandListResetWriter(command));
  }
}

void RecordingLayer::post(ID3D12FenceGetCompletedValueCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new ID3D12FenceGetCompletedValueWriter(command));
  }
  recorder_.executionEnd();
}

void RecordingLayer::post(CreateWindowMetaCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new CreateWindowMetaWriter(command));
  }
}

void RecordingLayer::post(MappedDataMetaCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new MappedDataMetaWriter(command));
  }
}

void RecordingLayer::post(CreateHeapAllocationMetaCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new CreateHeapAllocationMetaWriter(command));
  }
}

void RecordingLayer::post(WaitForFenceSignaledCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new WaitForFenceSignaledWriter(command));
  }
}

void RecordingLayer::post(IUnknownQueryInterfaceCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new IUnknownQueryInterfaceWriter(command));
  }
}

void RecordingLayer::post(IUnknownAddRefCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new IUnknownAddRefWriter(command));
  }
}

void RecordingLayer::post(IUnknownReleaseCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new IUnknownReleaseWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateDeviceExtensionContextWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateDeviceExtensionContext1Writer(command));
  }
}

void RecordingLayer::post(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_SetApplicationInfoWriter(command));
  }
}

void RecordingLayer::post(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_DestroyDeviceExtensionContextWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_SetFeatureSupportWriter(command));
  }
}

void RecordingLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (recorder_.isRunning()) {
    command.pDesc_.cs = command.pDesc_.value->CS.pShaderBytecode;
    command.pDesc_.compileOptions = command.pDesc_.value->CompileOptions;
    command.pDesc_.internalOptions = command.pDesc_.value->InternalOptions;
  }
}

void RecordingLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateComputePipelineStateWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreatePlacedResourceWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateCommittedResourceWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateReservedResourceWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateCommandQueueWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateHeapCommand& command) {
  if (recorder_.isRunning()) {
    recorder_.record(new INTC_D3D12_CreateHeapWriter(command));
  }
}

void RecordingLayer::post(IDXGIAdapter3QueryVideoMemoryInfoCommand& command) {}

} // namespace DirectX
} // namespace gits
