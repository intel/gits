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
    if (m_SubcaptureRange.InRange()) {
      LOG_WARNING << "Subcapture recording terminated prematurely";
    }
  } catch (...) {
    topmost_exception_handler("RecordingLayer::~RecordingLayer");
  }
}

void RecordingLayer::Post(IDXGISwapChainPresentCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(IDXGISwapChainPresentSerializer(Command));
    if (!(Command.m_Flags.Value & DXGI_PRESENT_TEST) && !IsStateRestoreKey(Command.Key)) {
      m_Recorder.Record(FrameEndSerializer(FrameEndCommand()));
    }
  }
  bool InRange = m_SubcaptureRange.InRange();
  if (!(Command.m_Flags.Value & DXGI_PRESENT_TEST)) {
    m_SubcaptureRange.FrameEnd(IsStateRestoreKey(Command.Key));
  }
  if (InRange && !m_SubcaptureRange.InRange()) {
    m_Recorder.FinishRecording();
  }
}

void RecordingLayer::Post(IDXGISwapChain1Present1Command& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(IDXGISwapChain1Present1Serializer(Command));
    if (!(Command.m_PresentFlags.Value & DXGI_PRESENT_TEST) && !IsStateRestoreKey(Command.Key)) {
      m_Recorder.Record(FrameEndSerializer(FrameEndCommand()));
    }
  }
  bool InRange = m_SubcaptureRange.InRange();
  if (!(Command.m_PresentFlags.Value & DXGI_PRESENT_TEST)) {
    m_SubcaptureRange.FrameEnd(IsStateRestoreKey(Command.Key));
  }
  if (InRange && !m_SubcaptureRange.InRange()) {
    m_Recorder.FinishRecording();
  }
}

void RecordingLayer::Post(ID3D12GraphicsCommandListResetCommand& Command) {
  m_SubcaptureRange.ExecutionStart();
  if (m_SubcaptureRange.InRange()) {
    if (m_SubcaptureRange.CommandListSubcapture()) {
      m_Recorder.Record(MarkerUInt64Serializer(
          MarkerUInt64Command(MarkerUInt64Command::Value::GPU_EXECUTION_BEGIN)));
    }
    m_Recorder.Record(ID3D12GraphicsCommandListResetSerializer(Command));
  }
}

void RecordingLayer::Post(ID3D12FenceGetCompletedValueCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(ID3D12FenceGetCompletedValueSerializer(Command));
    if (m_SubcaptureRange.CommandListSubcapture()) {
      m_Recorder.Record(MarkerUInt64Serializer(
          MarkerUInt64Command(MarkerUInt64Command::Value::GPU_EXECUTION_END)));
    }
  }
  bool InRange = m_SubcaptureRange.InRange();
  m_SubcaptureRange.ExecutionEnd();
  if (InRange && !m_SubcaptureRange.InRange()) {
    m_Recorder.FinishRecording();
  }
}

void RecordingLayer::Post(StateRestoreBeginCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(StateRestoreBeginSerializer(Command));
  }
}

void RecordingLayer::Post(StateRestoreEndCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(StateRestoreEndSerializer(Command));
  }
}

void RecordingLayer::Post(MarkerUInt64Command& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(MarkerUInt64Serializer(Command));
  }
}

void RecordingLayer::Post(CreateWindowMetaCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(CreateWindowMetaSerializer(Command));
  }
}

void RecordingLayer::Post(MappedDataMetaCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(MappedDataMetaSerializer(Command));
  }
}

void RecordingLayer::Post(CreateHeapAllocationMetaCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(CreateHeapAllocationMetaSerializer(Command));
  }
}

void RecordingLayer::Post(WaitForFenceSignaledCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(WaitForFenceSignaledSerializer(Command));
  }
}

void RecordingLayer::Post(DllContainerMetaCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(DllContainerMetaSerializer(Command));
  }
}

void RecordingLayer::Post(IUnknownQueryInterfaceCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(IUnknownQueryInterfaceSerializer(Command));
  }
}

void RecordingLayer::Post(IUnknownAddRefCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(IUnknownAddRefSerializer(Command));
  }
}

void RecordingLayer::Post(IUnknownReleaseCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(IUnknownReleaseSerializer(Command));
  }
}

void RecordingLayer::Post(INTC_D3D12_CreateDeviceExtensionContextCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateDeviceExtensionContextSerializer(Command));
  }
}

void RecordingLayer::Post(INTC_D3D12_CreateDeviceExtensionContext1Command& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateDeviceExtensionContext1Serializer(Command));
  }
}

void RecordingLayer::Post(INTC_D3D12_SetApplicationInfoCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(INTC_D3D12_SetApplicationInfoSerializer(Command));
  }
}

void RecordingLayer::Post(INTC_DestroyDeviceExtensionContextCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(INTC_DestroyDeviceExtensionContextSerializer(Command));
  }
}

void RecordingLayer::Post(INTC_D3D12_SetFeatureSupportCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(INTC_D3D12_SetFeatureSupportSerializer(Command));
  }
}

void RecordingLayer::Pre(INTC_D3D12_CreateComputePipelineStateCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    Command.m_pDesc.Cs = Command.m_pDesc.Value->CS.pShaderBytecode;
    Command.m_pDesc.CompileOptions = Command.m_pDesc.Value->CompileOptions;
    Command.m_pDesc.InternalOptions = Command.m_pDesc.Value->InternalOptions;
  }
}

void RecordingLayer::Post(INTC_D3D12_CreateComputePipelineStateCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateComputePipelineStateSerializer(Command));
  }
}

void RecordingLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(INTC_D3D12_CreatePlacedResourceSerializer(Command));
  }
}

void RecordingLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateCommittedResourceSerializer(Command));
  }
}

void RecordingLayer::Post(INTC_D3D12_CreateReservedResourceCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateReservedResourceSerializer(Command));
  }
}

void RecordingLayer::Post(INTC_D3D12_CreateCommandQueueCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateCommandQueueSerializer(Command));
  }
}

void RecordingLayer::Post(INTC_D3D12_CreateHeapCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(INTC_D3D12_CreateHeapSerializer(Command));
  }
}

void RecordingLayer::Post(IDXGIAdapter3QueryVideoMemoryInfoCommand& Command) {}

void RecordingLayer::Post(NvAPI_InitializeCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(NvAPI_InitializeSerializer(Command));
  }
}

void RecordingLayer::Post(NvAPI_UnloadCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(NvAPI_UnloadSerializer(Command));
  }
}

void RecordingLayer::Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer(Command));
  }
}

void RecordingLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer(Command));
  }
}

void RecordingLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer(Command));
  }
}

void RecordingLayer::Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer(Command));
  }
}

void RecordingLayer::Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& Command) {
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer(Command));
  }
}

void RecordingLayer::Post(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& Command) {
  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand not handled in "
                 "subcapture";
    logged = true;
  }
  if (m_SubcaptureRange.InRange()) {
    m_Recorder.Record(
        NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer(Command));
  }
}

} // namespace DirectX
} // namespace gits
