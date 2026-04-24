// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "commandPrinter.h"

#include <mutex>

namespace gits {
namespace DirectX {

class TraceLayer : public Layer {
private:
  CommandPrinterState m_StatePre;
  CommandPrinterState m_StatePost;
  FastOStream& m_StreamPre;
  FastOStream& m_StreamPost;
  bool m_PrintPre;
  bool m_PrintPost;
  bool m_Flush;

public:
  TraceLayer(FastOStream& streamPre, FastOStream& streamPost, std::mutex& mutex, bool flush)
      : Layer("Trace"),
        m_StatePre(mutex),
        m_StatePost(mutex),
        m_StreamPre(streamPre),
        m_StreamPost(streamPost),
        m_PrintPre(streamPre.IsOpen()),
        m_PrintPost(streamPost.IsOpen()),
        m_Flush(flush) {}

  void Pre(StateRestoreBeginCommand& command) override;
  void Post(StateRestoreBeginCommand& command) override;

  void Pre(StateRestoreEndCommand& command) override;
  void Post(StateRestoreEndCommand& command) override;

  void Pre(MarkerUInt64Command& command) override;
  void Post(MarkerUInt64Command& command) override;

  void Pre(CreateWindowMetaCommand& command) override;
  void Post(CreateWindowMetaCommand& command) override;

  void Pre(MappedDataMetaCommand& command) override;
  void Post(MappedDataMetaCommand& command) override;

  void Pre(CreateHeapAllocationMetaCommand& command) override;
  void Post(CreateHeapAllocationMetaCommand& command) override;

  void Pre(WaitForFenceSignaledCommand& command) override;
  void Post(WaitForFenceSignaledCommand& command) override;

  void Pre(DllContainerMetaCommand& command) override;
  void Post(DllContainerMetaCommand& command) override;

  void Pre(IUnknownQueryInterfaceCommand& command) override;
  void Post(IUnknownQueryInterfaceCommand& command) override;

  void Pre(IUnknownAddRefCommand& command) override;
  void Post(IUnknownAddRefCommand& command) override;

  void Pre(IUnknownReleaseCommand& command) override;
  void Post(IUnknownReleaseCommand& command) override;

  %for function in functions:
  void Pre(${function.name}Command& command) override;
  void Post(${function.name}Command& command) override;

  %endfor

  %for interface in interfaces:
  %for function in interface.functions:
  void Pre(${interface.name}${function.name}Command& command) override;
  void Post(${interface.name}${function.name}Command& command) override;

  %endfor
  %endfor
  void Pre(INTC_D3D12_GetSupportedVersionsCommand& command) override;
  void Post(INTC_D3D12_GetSupportedVersionsCommand& command) override;

  void Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  void Post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;

  void Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  void Post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;

  void Pre(INTC_D3D12_SetApplicationInfoCommand& command) override;
  void Post(INTC_D3D12_SetApplicationInfoCommand& command) override;
  
  void Pre(INTC_DestroyDeviceExtensionContextCommand& command) override;
  void Post(INTC_DestroyDeviceExtensionContextCommand& command) override;

  void Pre(INTC_D3D12_CheckFeatureSupportCommand& command) override;
  void Post(INTC_D3D12_CheckFeatureSupportCommand& command) override;

  void Pre(INTC_D3D12_SetFeatureSupportCommand& command) override;
  void Post(INTC_D3D12_SetFeatureSupportCommand& command) override;

  void Pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) override;
  void Post(INTC_D3D12_GetResourceAllocationInfoCommand& command) override;

  void Pre(INTC_D3D12_CreateComputePipelineStateCommand& command) override;
  void Post(INTC_D3D12_CreateComputePipelineStateCommand& command) override;

  void Pre(INTC_D3D12_CreatePlacedResourceCommand& command) override;
  void Post(INTC_D3D12_CreatePlacedResourceCommand& command) override;

  void Pre(INTC_D3D12_CreateCommittedResourceCommand& command) override;
  void Post(INTC_D3D12_CreateCommittedResourceCommand& command) override;

  void Pre(INTC_D3D12_CreateCommandQueueCommand& command) override;
  void Post(INTC_D3D12_CreateCommandQueueCommand& command) override;

  void Pre(INTC_D3D12_CreateReservedResourceCommand& command) override;
  void Post(INTC_D3D12_CreateReservedResourceCommand& command) override;

  void Pre(INTC_D3D12_CreateHeapCommand& command) override;
  void Post(INTC_D3D12_CreateHeapCommand& command) override;

  void Pre(NvAPI_InitializeCommand& command) override;
  void Post(NvAPI_InitializeCommand& command) override;

  void Pre(NvAPI_UnloadCommand& command) override;
  void Post(NvAPI_UnloadCommand& command) override;

  void Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) override;
  void Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) override;

  void Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  void Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;

  void Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  void Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;

  void Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  void Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;

  void Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  void Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;

  void Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;
  void Post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;
};

} // namespace DirectX
} // namespace gits
