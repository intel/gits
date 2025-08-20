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

#include <mutex>

namespace gits {
namespace DirectX {

class GlobalSynchronizationLayer : public Layer {
public:
  GlobalSynchronizationLayer() : Layer("GlobalSynchronization") {}

  void pre(IUnknownQueryInterfaceCommand& command) override;
  void post(IUnknownQueryInterfaceCommand& command) override;
  void pre(IUnknownAddRefCommand& command) override;
  void post(IUnknownAddRefCommand& command) override;
  void pre(IUnknownReleaseCommand& command) override;
  void post(IUnknownReleaseCommand& command) override;
  void pre(INTC_D3D12_GetSupportedVersionsCommand& command) override;
  void post(INTC_D3D12_GetSupportedVersionsCommand& command) override;
  void pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  void post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  void pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  void post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  void pre(INTC_D3D12_SetApplicationInfoCommand& command) override;
  void post(INTC_D3D12_SetApplicationInfoCommand& command) override;
  void pre(INTC_DestroyDeviceExtensionContextCommand& command) override;
  void post(INTC_DestroyDeviceExtensionContextCommand& command) override;
  void pre(INTC_D3D12_CheckFeatureSupportCommand& command) override;
  void post(INTC_D3D12_CheckFeatureSupportCommand& command) override;
  void pre(INTC_D3D12_CreateCommandQueueCommand& command) override;
  void post(INTC_D3D12_CreateCommandQueueCommand& command) override;
  void pre(INTC_D3D12_CreateReservedResourceCommand& command) override;
  void post(INTC_D3D12_CreateReservedResourceCommand& command) override;
  void pre(INTC_D3D12_SetFeatureSupportCommand& command) override;
  void post(INTC_D3D12_SetFeatureSupportCommand& command) override;
  void pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) override;
  void post(INTC_D3D12_GetResourceAllocationInfoCommand& command) override;
  void pre(INTC_D3D12_CreateComputePipelineStateCommand& command) override;
  void post(INTC_D3D12_CreateComputePipelineStateCommand& command) override;
  void pre(INTC_D3D12_CreatePlacedResourceCommand& command) override;
  void post(INTC_D3D12_CreatePlacedResourceCommand& command) override;
  void pre(INTC_D3D12_CreateCommittedResourceCommand& command) override;
  void post(INTC_D3D12_CreateCommittedResourceCommand& command) override;
  void pre(INTC_D3D12_CreateHeapCommand& command) override;
  void post(INTC_D3D12_CreateHeapCommand& command) override;
  void pre(NvAPI_InitializeCommand& command) override;
  void post(NvAPI_InitializeCommand& command) override;
  void pre(NvAPI_UnloadCommand& command) override;
  void post(NvAPI_UnloadCommand& command) override;
  void pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) override;
  void post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) override;
  void pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  void post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  void pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  void post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  void pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  void post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  void pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  void post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  void pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;
  void post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;

  %for function in functions:
  void pre(${function.name}Command& command) override;
  void post(${function.name}Command& command) override;
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  void pre(${interface.name}${function.name}Command& command) override;
  void post(${interface.name}${function.name}Command& command) override;
  %endfor
  %endfor

private:
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
