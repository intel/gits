// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandsAuto.h"
#include "commandsCustom.h"

#include <string>

namespace gits {
namespace DirectX {

class Layer {
public:
  Layer(const std::string& name) : m_Name(name) {}
  virtual ~Layer() {}

  const std::string& GetName() const {
    return m_Name;
  }

  virtual void Pre(StateRestoreBeginCommand& command) {}
  virtual void Post(StateRestoreBeginCommand& command) {}

  virtual void Pre(StateRestoreEndCommand& command) {}
  virtual void Post(StateRestoreEndCommand& command) {}

  virtual void Pre(FrameEndCommand& command) {}
  virtual void Post(FrameEndCommand& command) {}

  virtual void Pre(MarkerUInt64Command& command) {}
  virtual void Post(MarkerUInt64Command& command) {}

  virtual void Pre(CreateWindowMetaCommand& command) {}
  virtual void Post(CreateWindowMetaCommand& command) {}

  virtual void Pre(MappedDataMetaCommand& command) {}
  virtual void Post(MappedDataMetaCommand& command) {}

  virtual void Pre(CreateHeapAllocationMetaCommand& command) {}
  virtual void Post(CreateHeapAllocationMetaCommand& command) {}

  virtual void Pre(WaitForFenceSignaledCommand& command) {}
  virtual void Post(WaitForFenceSignaledCommand& command) {}

  virtual void Pre(DllContainerMetaCommand& command) {}
  virtual void Post(DllContainerMetaCommand& command) {}

  virtual void Pre(IUnknownQueryInterfaceCommand& command) {}
  virtual void Post(IUnknownQueryInterfaceCommand& command) {}

  virtual void Pre(IUnknownAddRefCommand& command) {}
  virtual void Post(IUnknownAddRefCommand& command) {}

  virtual void Pre(IUnknownReleaseCommand& command) {}
  virtual void Post(IUnknownReleaseCommand& command) {}

  %for function in functions:
  virtual void Pre(${function.name}Command& command) {}
  virtual void Post(${function.name}Command& command) {}

  %endfor

  %for interface in interfaces:
  %for function in interface.functions:
  virtual void Pre(${interface.name}${function.name}Command& command) {}
  virtual void Post(${interface.name}${function.name}Command& command) {}

  %endfor
  %endfor
  virtual void Pre(INTC_D3D12_GetSupportedVersionsCommand& command) {}
  virtual void Post(INTC_D3D12_GetSupportedVersionsCommand& command) {}

  virtual void Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {}
  virtual void Post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {}

  virtual void Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {}
  virtual void Post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {}

  virtual void Pre(INTC_D3D12_SetApplicationInfoCommand& command) {}
  virtual void Post(INTC_D3D12_SetApplicationInfoCommand& command) {}

  virtual void Pre(INTC_DestroyDeviceExtensionContextCommand& command) {}
  virtual void Post(INTC_DestroyDeviceExtensionContextCommand& command) {}

  virtual void Pre(INTC_D3D12_CheckFeatureSupportCommand& command) {}
  virtual void Post(INTC_D3D12_CheckFeatureSupportCommand& command) {}

  virtual void Pre(INTC_D3D12_CreateCommandQueueCommand& command) {}
  virtual void Post(INTC_D3D12_CreateCommandQueueCommand& command) {}

  virtual void Pre(INTC_D3D12_CreateReservedResourceCommand& command) {}
  virtual void Post(INTC_D3D12_CreateReservedResourceCommand& command) {}

  virtual void Pre(INTC_D3D12_SetFeatureSupportCommand& command) {}
  virtual void Post(INTC_D3D12_SetFeatureSupportCommand& command) {}

  virtual void Pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) {}
  virtual void Post(INTC_D3D12_GetResourceAllocationInfoCommand& command) {}

  virtual void Pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {}
  virtual void Post(INTC_D3D12_CreateComputePipelineStateCommand& command) {}

  virtual void Pre(INTC_D3D12_CreatePlacedResourceCommand& command) {}
  virtual void Post(INTC_D3D12_CreatePlacedResourceCommand& command) {}

  virtual void Pre(INTC_D3D12_CreateCommittedResourceCommand& command) {}
  virtual void Post(INTC_D3D12_CreateCommittedResourceCommand& command) {}
  
  virtual void Pre(INTC_D3D12_CreateHeapCommand& command) {}
  virtual void Post(INTC_D3D12_CreateHeapCommand& command) {}

  virtual void Pre(NvAPI_InitializeCommand& command) {}
  virtual void Post(NvAPI_InitializeCommand& command) {}

  virtual void Pre(NvAPI_UnloadCommand& command) {}
  virtual void Post(NvAPI_UnloadCommand& command) {}

  virtual void Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {}
  virtual void Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {}

  virtual void Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {}
  virtual void Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {}
  
  virtual void Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {}
  virtual void Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {}

  virtual void Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {}
  virtual void Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {}

  virtual void Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {}
  virtual void Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {}

  virtual void Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {}
  virtual void Post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {}

private:
  std::string m_Name;
};

} // namespace DirectX
} // namespace gits
