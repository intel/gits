// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "recordingLayerAuto.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
#include "intelExtensions.h"
#include "keyUtils.h"
#include "log.h"
#include "exception.h"

namespace gits {
namespace DirectX {

RecordingLayer::~RecordingLayer() {
  try {
    if (subcaptureRange_.inRange()) {
      LOG_WARNING << "Subcapture recording terminated prematurely";
    }
  } catch (...) {
    topmost_exception_handler("RecordingLayer::~RecordingLayer");
  }
}

void RecordingLayer::post(IDXGISwapChainPresentCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(IDXGISwapChainPresentSerializer(command));
    if (!(command.Flags_.value & DXGI_PRESENT_TEST) && !isStateRestoreKey(command.key)) {
      recorder_.record(FrameEndSerializer(FrameEndCommand()));
    }
  }
  bool inRange = subcaptureRange_.inRange();
  if (!(command.Flags_.value & DXGI_PRESENT_TEST)) {
    subcaptureRange_.frameEnd(isStateRestoreKey(command.key));
  }
  if (inRange && !subcaptureRange_.inRange()) {
    recorder_.finishRecording();
  }
}

void RecordingLayer::post(IDXGISwapChain1Present1Command& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(IDXGISwapChain1Present1Serializer(command));
    if (!(command.PresentFlags_.value & DXGI_PRESENT_TEST) && !isStateRestoreKey(command.key)) {
      recorder_.record(FrameEndSerializer(FrameEndCommand()));
    }
  }
  bool inRange = subcaptureRange_.inRange();
  if (!(command.PresentFlags_.value & DXGI_PRESENT_TEST)) {
    subcaptureRange_.frameEnd(isStateRestoreKey(command.key));
  }
  if (inRange && !subcaptureRange_.inRange()) {
    recorder_.finishRecording();
  }
}

void RecordingLayer::post(ID3D12GraphicsCommandListResetCommand& command) {
  subcaptureRange_.executionStart();
  if (subcaptureRange_.inRange()) {
    if (subcaptureRange_.commandListSubcapture()) {
      recorder_.record(MarkerUInt64Serializer(
          MarkerUInt64Command(MarkerUInt64Command::Value::GPU_EXECUTION_BEGIN)));
    }
    recorder_.record(ID3D12GraphicsCommandListResetSerializer(command));
  }
}

void RecordingLayer::post(ID3D12FenceGetCompletedValueCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(ID3D12FenceGetCompletedValueSerializer(command));
    if (subcaptureRange_.commandListSubcapture()) {
      recorder_.record(MarkerUInt64Serializer(
          MarkerUInt64Command(MarkerUInt64Command::Value::GPU_EXECUTION_END)));
    }
  }
  bool inRange = subcaptureRange_.inRange();
  subcaptureRange_.executionEnd();
  if (inRange && !subcaptureRange_.inRange()) {
    recorder_.finishRecording();
  }
}

void RecordingLayer::post(StateRestoreBeginCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(StateRestoreBeginSerializer(command));
  }
}

void RecordingLayer::post(StateRestoreEndCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(StateRestoreEndSerializer(command));
  }
}

void RecordingLayer::post(MarkerUInt64Command& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(MarkerUInt64Serializer(command));
  }
}

void RecordingLayer::post(CreateWindowMetaCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(CreateWindowMetaSerializer(command));
  }
}

void RecordingLayer::post(MappedDataMetaCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(MappedDataMetaSerializer(command));
  }
}

void RecordingLayer::post(CreateHeapAllocationMetaCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(CreateHeapAllocationMetaSerializer(command));
  }
}

void RecordingLayer::post(WaitForFenceSignaledCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(WaitForFenceSignaledSerializer(command));
  }
}

void RecordingLayer::post(DllContainerMetaCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(DllContainerMetaSerializer(command));
  }
}

void RecordingLayer::post(IUnknownQueryInterfaceCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(IUnknownQueryInterfaceSerializer(command));
  }
}

void RecordingLayer::post(IUnknownAddRefCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(IUnknownAddRefSerializer(command));
  }
}

void RecordingLayer::post(IUnknownReleaseCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(IUnknownReleaseSerializer(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(INTC_D3D12_CreateDeviceExtensionContextSerializer(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(INTC_D3D12_CreateDeviceExtensionContext1Serializer(command));
  }
}

void RecordingLayer::post(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(INTC_D3D12_SetApplicationInfoSerializer(command));
  }
}

void RecordingLayer::post(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(INTC_DestroyDeviceExtensionContextSerializer(command));
  }
}

void RecordingLayer::post(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(INTC_D3D12_SetFeatureSupportSerializer(command));
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
    recorder_.record(INTC_D3D12_CreateComputePipelineStateSerializer(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(INTC_D3D12_CreatePlacedResourceSerializer(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(INTC_D3D12_CreateCommittedResourceSerializer(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(INTC_D3D12_CreateReservedResourceSerializer(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(INTC_D3D12_CreateCommandQueueSerializer(command));
  }
}

void RecordingLayer::post(INTC_D3D12_CreateHeapCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(INTC_D3D12_CreateHeapSerializer(command));
  }
}

void RecordingLayer::post(IDXGIAdapter3QueryVideoMemoryInfoCommand& command) {}

void RecordingLayer::post(NvAPI_InitializeCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(NvAPI_InitializeSerializer(command));
  }
}

void RecordingLayer::post(NvAPI_UnloadCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(NvAPI_UnloadSerializer(command));
  }
}

void RecordingLayer::post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer(command));
  }
}

void RecordingLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer(command));
  }
}

void RecordingLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer(command));
  }
}

void RecordingLayer::post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer(command));
  }
}

void RecordingLayer::post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  if (subcaptureRange_.inRange()) {
    recorder_.record(NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer(command));
  }
}

void RecordingLayer::post(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand not handled in "
                 "subcapture";
    logged = true;
  }
  if (subcaptureRange_.inRange()) {
    recorder_.record(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer(command));
  }
}

} // namespace DirectX
} // namespace gits
