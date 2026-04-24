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
#include "configurationLib.h"
#include "keyUtils.h"
#include "bit_range.h"

namespace gits {
namespace DirectX {

class SkipCallsOnConfigLayer : public Layer {
public:
  SkipCallsOnConfigLayer() : Layer("SkipCallsOnConfig"),
      keyRange_(ParseConfigKeys(Configurator::Get().directx.features.skipCalls.commandKeys)) {}

  virtual void Pre(CreateWindowMetaCommand& command) override;
  virtual void Pre(MappedDataMetaCommand& command) override;
  virtual void Pre(CreateHeapAllocationMetaCommand& command) override;
  virtual void Pre(WaitForFenceSignaledCommand& command) override;
  virtual void Pre(DllContainerMetaCommand& command) override;
  virtual void Pre(IUnknownQueryInterfaceCommand& command) override;
  virtual void Pre(IUnknownAddRefCommand& command) override;
  virtual void Pre(IUnknownReleaseCommand& command) override;

  virtual void Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  virtual void Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  virtual void Pre(INTC_D3D12_SetApplicationInfoCommand& command) override;
  virtual void Pre(INTC_DestroyDeviceExtensionContextCommand& command) override;
  virtual void Pre(INTC_D3D12_CheckFeatureSupportCommand& command) override;
  virtual void Pre(INTC_D3D12_SetFeatureSupportCommand& command) override;
  virtual void Pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) override;
  virtual void Pre(INTC_D3D12_CreateComputePipelineStateCommand& command) override;
  virtual void Pre(INTC_D3D12_CreatePlacedResourceCommand& command) override;
  virtual void Pre(INTC_D3D12_CreateCommittedResourceCommand& command) override;
  virtual void Pre(INTC_D3D12_CreateReservedResourceCommand& command) override;
  virtual void Pre(INTC_D3D12_CreateCommandQueueCommand& command) override;
  virtual void Pre(INTC_D3D12_CreateHeapCommand& command) override;
  virtual void Pre(NvAPI_InitializeCommand& command) override;
  virtual void Pre(NvAPI_UnloadCommand& command) override;
  virtual void Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) override;
  virtual void Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  virtual void Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  virtual void Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  virtual void Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  virtual void Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;
  
  %for function in functions:
  virtual void Pre(${function.name}Command& command) override;
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  virtual void Pre(${interface.name}${function.name}Command& command) override;
  %endfor
  %endfor

private:
  BitRange keyRange_;
};

} // namespace DirectX
} // namespace gits
