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

void EncoderLayer::Post(IUnknownQueryInterfaceCommand& command) {
  m_Recorder.Record(command.Key, new IUnknownQueryInterfaceSerializer(command));
}

void EncoderLayer::Post(IUnknownAddRefCommand& command) {
  m_Recorder.Record(command.Key, new IUnknownAddRefSerializer(command));
}

void EncoderLayer::Post(IUnknownReleaseCommand& command) {
  m_Recorder.Record(command.Key, new IUnknownReleaseSerializer(command));
}

void EncoderLayer::Post(INTC_D3D12_GetSupportedVersionsCommand& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_GetSupportedVersionsSerializer(command));
}

void EncoderLayer::Post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_CreateDeviceExtensionContextSerializer(command));
}

void EncoderLayer::Post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_CreateDeviceExtensionContext1Serializer(command));
}

void EncoderLayer::Post(INTC_D3D12_SetApplicationInfoCommand& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_SetApplicationInfoSerializer(command));
}

void EncoderLayer::Post(INTC_DestroyDeviceExtensionContextCommand& command) {
  m_Recorder.Record(command.Key, new INTC_DestroyDeviceExtensionContextSerializer(command));
}

void EncoderLayer::Post(INTC_D3D12_CheckFeatureSupportCommand& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_CheckFeatureSupportSerializer(command));
}

void EncoderLayer::Post(INTC_D3D12_CreateCommandQueueCommand& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_CreateCommandQueueSerializer(command));
}

void EncoderLayer::Post(INTC_D3D12_CreateReservedResourceCommand& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_CreateReservedResourceSerializer(command));
}

void EncoderLayer::Post(INTC_D3D12_SetFeatureSupportCommand& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_SetFeatureSupportSerializer(command));
}

void EncoderLayer::Post(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_GetResourceAllocationInfoSerializer(command));
}

void EncoderLayer::Post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_CreateComputePipelineStateSerializer(command));
}

void EncoderLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_CreatePlacedResourceSerializer(command));
}

void EncoderLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_CreateCommittedResourceSerializer(command));
}

void EncoderLayer::Post(INTC_D3D12_CreateHeapCommand& command) {
  m_Recorder.Record(command.Key, new INTC_D3D12_CreateHeapSerializer(command));
}

void EncoderLayer::Post(NvAPI_InitializeCommand& command) {
  m_Recorder.Record(command.Key, new NvAPI_InitializeSerializer(command));
}

void EncoderLayer::Post(NvAPI_UnloadCommand& command) {
  m_Recorder.Record(command.Key, new NvAPI_UnloadSerializer(command));
}

void EncoderLayer::Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  m_Recorder.Record(command.Key, new NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer(command));
}

void EncoderLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  m_Recorder.Record(command.Key, new NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer(command));
}

void EncoderLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  m_Recorder.Record(command.Key,
                    new NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer(command));
}

void EncoderLayer::Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  m_Recorder.Record(command.Key,
                    new NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer(command));
}

void EncoderLayer::Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  m_Recorder.Record(command.Key,
                    new NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer(command));
}

void EncoderLayer::Post(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  m_Recorder.Record(
      command.Key,
      new NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer(command));
}

void EncoderLayer::Post(IDXGISwapChainPresentCommand& c) {
  const auto keys = CaptureManager::get().createCommandKeyRange(2);
  m_Recorder.Skip(c.Key);
  c.Key = keys.first;

  if (!c.Skip) {
    m_Recorder.Record(c.Key, new IDXGISwapChainPresentSerializer(c));
  } else {
    m_Recorder.Skip(c.Key);
  }
  if (!(c.m_Flags.Value & DXGI_PRESENT_TEST)) {
    m_Recorder.Record(keys.second, new FrameEndSerializer(FrameEndCommand()));
  } else {
    m_Recorder.Skip(keys.second);
  }
}

void EncoderLayer::Post(IDXGISwapChain1Present1Command& c) {
  const auto keys = CaptureManager::get().createCommandKeyRange(2);
  m_Recorder.Skip(c.Key);
  c.Key = keys.first;

  if (!c.Skip) {
    m_Recorder.Record(c.Key, new IDXGISwapChain1Present1Serializer(c));
  } else {
    m_Recorder.Skip(c.Key);
  }
  if (!(c.m_PresentFlags.Value & DXGI_PRESENT_TEST)) {
    m_Recorder.Record(keys.second, new FrameEndSerializer(FrameEndCommand()));
  } else {
    m_Recorder.Skip(keys.second);
  }
}

} // namespace DirectX
} // namespace gits
