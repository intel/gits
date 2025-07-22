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

RecordingLayer::~RecordingLayer() {
  try {
    if (subcaptureRange_.inRange()) {
      Log(ERR) << "Subcapture recording terminated prematurely";
    }
  } catch (...) {
    topmost_exception_handler("RecordingLayer::~RecordingLayer");
  }
}

void RecordingLayer::post(IDXGISwapChainPresentCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new IDXGISwapChainPresentWriter(command));
  }
  if (!(command.Flags_.value & DXGI_PRESENT_TEST)) {
    subcaptureRange_.frameEnd(command.key & Command::stateRestoreKeyMask);
  }
  if (!(command.Flags_.value & DXGI_PRESENT_TEST) &&
      !(command.key & Command::stateRestoreKeyMask)) {
    recorder_.frameEnd();
  }
}

void RecordingLayer::post(IDXGISwapChain1Present1Command& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new IDXGISwapChain1Present1Writer(command));
  }
  if (!(command.PresentFlags_.value & DXGI_PRESENT_TEST)) {
    subcaptureRange_.frameEnd(command.key & Command::stateRestoreKeyMask);
  }
  if (!(command.PresentFlags_.value & DXGI_PRESENT_TEST) &&
      !(command.key & Command::stateRestoreKeyMask)) {
    recorder_.frameEnd();
  }
}

void RecordingLayer::post(ID3D12GraphicsCommandListResetCommand& command) {
  subcaptureRange_.executionStart();
  if (subcaptureRange_.inRange()) {
    recorder_.record(new ID3D12GraphicsCommandListResetWriter(command));
  }
}

void RecordingLayer::post(ID3D12FenceGetCompletedValueCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new ID3D12FenceGetCompletedValueWriter(command));
  }
  subcaptureRange_.executionEnd();
}

void RecordingLayer::post(CreateWindowMetaCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new CreateWindowMetaWriter(command));
  }
}

void RecordingLayer::post(MappedDataMetaCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new MappedDataMetaWriter(command));
  }
}

void RecordingLayer::post(CreateHeapAllocationMetaCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new CreateHeapAllocationMetaWriter(command));
  }
}

void RecordingLayer::post(WaitForFenceSignaledCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new WaitForFenceSignaledWriter(command));
  }
}

void RecordingLayer::post(IUnknownQueryInterfaceCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new IUnknownQueryInterfaceWriter(command));
  }
}

void RecordingLayer::post(IUnknownAddRefCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new IUnknownAddRefWriter(command));
  }
}

void RecordingLayer::post(IUnknownReleaseCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new IUnknownReleaseWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new INTC_D3D12_CreateDeviceExtensionContextWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new INTC_D3D12_CreateDeviceExtensionContext1Writer(command));
  }
}

void RecordingLayer::post(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new INTC_D3D12_SetApplicationInfoWriter(command));
  }
}

void RecordingLayer::post(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new INTC_DestroyDeviceExtensionContextWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new INTC_D3D12_SetFeatureSupportWriter(command));
  }
}

void RecordingLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (subcaptureRange_.inRange()) {
    command.pDesc_.cs = command.pDesc_.value->CS.pShaderBytecode;
    command.pDesc_.compileOptions = command.pDesc_.value->CompileOptions;
    command.pDesc_.internalOptions = command.pDesc_.value->InternalOptions;
  }
}

void RecordingLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new INTC_D3D12_CreateComputePipelineStateWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new INTC_D3D12_CreatePlacedResourceWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new INTC_D3D12_CreateCommittedResourceWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new INTC_D3D12_CreateReservedResourceWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new INTC_D3D12_CreateCommandQueueWriter(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateHeapCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new INTC_D3D12_CreateHeapWriter(command));
  }
}

void RecordingLayer::post(IDXGIAdapter3QueryVideoMemoryInfoCommand& command) {}

void RecordingLayer::post(NvAPI_InitializeCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new NvAPI_InitializeWriter(command));
  }
}

void RecordingLayer::post(NvAPI_UnloadCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(new NvAPI_UnloadWriter(command));
  }
}
void RecordingLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand not handled in subcapture";
    logged = true;
  }
  if (subcaptureRange_.inRange()) {
    recorder_.record(new NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadWriter(command));
  }
}
void RecordingLayer::post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  static bool logged = false;
  if (!logged) {
    Log(ERR)
        << "NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand not handled in subcapture";
    logged = true;
  }
  if (subcaptureRange_.inRange()) {
    recorder_.record(new NvAPI_D3D12_BuildRaytracingAccelerationStructureExWriter(command));
  }
}
void RecordingLayer::post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  static bool logged = false;
  if (!logged) {
    Log(ERR) << "NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand not handled in subcapture";
    logged = true;
  }
  if (subcaptureRange_.inRange()) {
    recorder_.record(new NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayWriter(command));
  }
}

} // namespace DirectX
} // namespace gits
