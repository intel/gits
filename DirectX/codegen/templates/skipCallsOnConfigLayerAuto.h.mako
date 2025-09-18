// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "config.h"
#include "configKeySet.h"

namespace gits {
namespace DirectX {

class SkipCallsOnConfigLayer : public Layer {
public:
  SkipCallsOnConfigLayer() : Layer("SkipCallsOnConfig"),
      configKeySet_(Configurator::Get().directx.features.skipCalls.commandKeys)
  {}

  virtual void pre(CreateWindowMetaCommand& command) override;
  virtual void pre(MappedDataMetaCommand& command) override;
  virtual void pre(CreateHeapAllocationMetaCommand& command) override;
  virtual void pre(WaitForFenceSignaledCommand& command) override;
  virtual void pre(DllContainerMetaCommand& command) override;
  virtual void pre(IUnknownQueryInterfaceCommand& command) override;
  virtual void pre(IUnknownAddRefCommand& command) override;
  virtual void pre(IUnknownReleaseCommand& command) override;

  virtual void pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  virtual void pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  virtual void pre(INTC_D3D12_SetApplicationInfoCommand& command) override;
  virtual void pre(INTC_DestroyDeviceExtensionContextCommand& command) override;
  virtual void pre(INTC_D3D12_CheckFeatureSupportCommand& command) override;
  virtual void pre(INTC_D3D12_SetFeatureSupportCommand& command) override;
  virtual void pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) override;
  virtual void pre(INTC_D3D12_CreateComputePipelineStateCommand& command) override;
  virtual void pre(INTC_D3D12_CreatePlacedResourceCommand& command) override;
  virtual void pre(INTC_D3D12_CreateCommittedResourceCommand& command) override;
  virtual void pre(INTC_D3D12_CreateReservedResourceCommand& command) override;
  virtual void pre(INTC_D3D12_CreateCommandQueueCommand& command) override;
  virtual void pre(INTC_D3D12_CreateHeapCommand& command) override;
  virtual void pre(NvAPI_InitializeCommand& command) override;
  virtual void pre(NvAPI_UnloadCommand& command) override;
  virtual void pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) override;
  virtual void pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  virtual void pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  virtual void pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  virtual void pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  virtual void pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;
  
  %for function in functions:
  virtual void pre(${function.name}Command& command) override;
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  virtual void pre(${interface.name}${function.name}Command& command) override;
  %endfor
  %endfor

private:
  ConfigKeySet configKeySet_;
};

} // namespace DirectX
} // namespace gits
