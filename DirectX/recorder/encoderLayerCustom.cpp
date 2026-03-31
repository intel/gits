// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "encoderLayerAuto.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"

#include "captureManager.h"

namespace gits {
namespace DirectX {

void EncoderLayer::post(IUnknownQueryInterfaceCommand& command) {
  recorder_.Record(command.key, new IUnknownQueryInterfaceSerializer(command));
}

void EncoderLayer::post(IUnknownAddRefCommand& command) {
  recorder_.Record(command.key, new IUnknownAddRefSerializer(command));
}

void EncoderLayer::post(IUnknownReleaseCommand& command) {
  recorder_.Record(command.key, new IUnknownReleaseSerializer(command));
}

void EncoderLayer::post(INTC_D3D12_GetSupportedVersionsCommand& command) {
  recorder_.Record(command.key, new INTC_D3D12_GetSupportedVersionsSerializer(command));
}

void EncoderLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  recorder_.Record(command.key, new INTC_D3D12_CreateDeviceExtensionContextSerializer(command));
}

void EncoderLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  recorder_.Record(command.key, new INTC_D3D12_CreateDeviceExtensionContext1Serializer(command));
}

void EncoderLayer::post(INTC_D3D12_SetApplicationInfoCommand& command) {
  recorder_.Record(command.key, new INTC_D3D12_SetApplicationInfoSerializer(command));
}

void EncoderLayer::post(INTC_DestroyDeviceExtensionContextCommand& command) {
  recorder_.Record(command.key, new INTC_DestroyDeviceExtensionContextSerializer(command));
}

void EncoderLayer::post(INTC_D3D12_CheckFeatureSupportCommand& command) {
  recorder_.Record(command.key, new INTC_D3D12_CheckFeatureSupportSerializer(command));
}

void EncoderLayer::post(INTC_D3D12_CreateCommandQueueCommand& command) {
  recorder_.Record(command.key, new INTC_D3D12_CreateCommandQueueSerializer(command));
}

void EncoderLayer::post(INTC_D3D12_CreateReservedResourceCommand& command) {
  recorder_.Record(command.key, new INTC_D3D12_CreateReservedResourceSerializer(command));
}

void EncoderLayer::post(INTC_D3D12_SetFeatureSupportCommand& command) {
  recorder_.Record(command.key, new INTC_D3D12_SetFeatureSupportSerializer(command));
}

void EncoderLayer::post(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  recorder_.Record(command.key, new INTC_D3D12_GetResourceAllocationInfoSerializer(command));
}

void EncoderLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  recorder_.Record(command.key, new INTC_D3D12_CreateComputePipelineStateSerializer(command));
}

void EncoderLayer::post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  recorder_.Record(command.key, new INTC_D3D12_CreatePlacedResourceSerializer(command));
}

void EncoderLayer::post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  recorder_.Record(command.key, new INTC_D3D12_CreateCommittedResourceSerializer(command));
}

void EncoderLayer::post(INTC_D3D12_CreateHeapCommand& command) {
  recorder_.Record(command.key, new INTC_D3D12_CreateHeapSerializer(command));
}

void EncoderLayer::post(NvAPI_InitializeCommand& command) {
  recorder_.Record(command.key, new NvAPI_InitializeSerializer(command));
}

void EncoderLayer::post(NvAPI_UnloadCommand& command) {
  recorder_.Record(command.key, new NvAPI_UnloadSerializer(command));
}

void EncoderLayer::post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  recorder_.Record(command.key, new NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer(command));
}

void EncoderLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  recorder_.Record(command.key, new NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer(command));
}

void EncoderLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  recorder_.Record(command.key,
                   new NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer(command));
}

void EncoderLayer::post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  recorder_.Record(command.key,
                   new NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer(command));
}

void EncoderLayer::post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  recorder_.Record(command.key,
                   new NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer(command));
}

void EncoderLayer::post(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  recorder_.Record(
      command.key,
      new NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer(command));
}

void EncoderLayer::post(IDXGISwapChainPresentCommand& c) {
  const auto keys = CaptureManager::get().createCommandKeyRange(2);
  recorder_.Skip(c.key);
  c.key = keys.first;

  if (!c.skip) {
    recorder_.Record(c.key, new IDXGISwapChainPresentSerializer(c));
  } else {
    recorder_.Skip(c.key);
  }
  if (!(c.Flags_.value & DXGI_PRESENT_TEST)) {
    recorder_.Record(keys.second, new FrameEndSerializer(FrameEndCommand()));
  } else {
    recorder_.Skip(keys.second);
  }
}

void EncoderLayer::post(IDXGISwapChain1Present1Command& c) {
  const auto keys = CaptureManager::get().createCommandKeyRange(2);
  recorder_.Skip(c.key);
  c.key = keys.first;

  if (!c.skip) {
    recorder_.Record(c.key, new IDXGISwapChain1Present1Serializer(c));
  } else {
    recorder_.Skip(c.key);
  }
  if (!(c.PresentFlags_.value & DXGI_PRESENT_TEST)) {
    recorder_.Record(keys.second, new FrameEndSerializer(FrameEndCommand()));
  } else {
    recorder_.Skip(keys.second);
  }
}

} // namespace DirectX
} // namespace gits
