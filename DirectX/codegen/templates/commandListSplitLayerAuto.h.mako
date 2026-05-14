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
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "commandListSplitRecorder.h"
#include "commandListSplitService.h"

namespace gits {
namespace DirectX {

class CommandListSplitLayer : public Layer {
public:
  CommandListSplitLayer(CommandListSplitRecorder& recorder);
  ~CommandListSplitLayer();
  
  void Pre(StateRestoreBeginCommand& c) override;
  void Pre(StateRestoreEndCommand& c) override;
  void Pre(FrameEndCommand& c) override;
  void Pre(MarkerUInt64Command& c) override;
  void Pre(CreateWindowMetaCommand& c) override;
  void Pre(MappedDataMetaCommand& c) override;
  void Pre(CreateHeapAllocationMetaCommand& c) override;
  void Pre(WaitForFenceSignaledCommand& c) override;
  void Pre(DllContainerMetaCommand& c) override;
  void Pre(IUnknownQueryInterfaceCommand& c) override;
  void Pre(IUnknownAddRefCommand& c) override;
  void Pre(IUnknownReleaseCommand& c) override;
  %for function in functions:
  void Pre(${function.name}Command& c) override;
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  void Pre(${interface.name}${function.name}Command& c) override;
  %endfor
  %endfor
  void Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& c) override;
  void Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& c) override;
  void Pre(INTC_D3D12_SetApplicationInfoCommand& c) override;
  void Pre(INTC_DestroyDeviceExtensionContextCommand& c) override;
  void Pre(INTC_D3D12_SetFeatureSupportCommand& c) override;
  void Pre(INTC_D3D12_CreateComputePipelineStateCommand& c) override;
  void Pre(INTC_D3D12_CreatePlacedResourceCommand& c) override;
  void Pre(INTC_D3D12_CreateCommittedResourceCommand& c) override;
  void Pre(INTC_D3D12_CreateReservedResourceCommand& c) override;
  void Pre(INTC_D3D12_CreateCommandQueueCommand& c) override;
  void Pre(INTC_D3D12_CreateHeapCommand& c) override;
  void Pre(NvAPI_InitializeCommand& c) override;
  void Pre(NvAPI_UnloadCommand& c) override;
  void Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) override;
  void Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) override;
  void Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) override;
  void Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) override;
  void Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) override;
  void Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) override;

private:
  CommandListSplitRecorder& m_Recorder;
  CommandListSplitService m_SplitService;
};

} // namespace DirectX
} // namespace gits
