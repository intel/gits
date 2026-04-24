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

#include <mutex>

namespace gits {
namespace DirectX {

class GlobalSynchronizationLayer : public Layer {
public:
  GlobalSynchronizationLayer() : Layer("GlobalSynchronization") {}

  void Pre(IUnknownQueryInterfaceCommand& command) override;
  void Post(IUnknownQueryInterfaceCommand& command) override;
  void Pre(IUnknownAddRefCommand& command) override;
  void Post(IUnknownAddRefCommand& command) override;
  void Pre(IUnknownReleaseCommand& command) override;
  void Post(IUnknownReleaseCommand& command) override;
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
  void Pre(INTC_D3D12_CreateCommandQueueCommand& command) override;
  void Post(INTC_D3D12_CreateCommandQueueCommand& command) override;
  void Pre(INTC_D3D12_CreateReservedResourceCommand& command) override;
  void Post(INTC_D3D12_CreateReservedResourceCommand& command) override;
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

private:
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits
