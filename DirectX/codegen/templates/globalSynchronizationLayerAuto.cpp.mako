// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "globalSynchronizationLayerAuto.h"

namespace gits {
namespace DirectX {

void GlobalSynchronizationLayer::Pre(IUnknownQueryInterfaceCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(IUnknownQueryInterfaceCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(IUnknownAddRefCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(IUnknownAddRefCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(IUnknownReleaseCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(IUnknownReleaseCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_GetSupportedVersionsCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_GetSupportedVersionsCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_SetApplicationInfoCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_SetApplicationInfoCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_DestroyDeviceExtensionContextCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_CheckFeatureSupportCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_CreateCommandQueueCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_CreateCommandQueueCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_CreateReservedResourceCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_CreateReservedResourceCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_SetFeatureSupportCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(INTC_D3D12_CreateHeapCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(INTC_D3D12_CreateHeapCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(NvAPI_InitializeCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(NvAPI_InitializeCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(NvAPI_UnloadCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(NvAPI_UnloadCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  mutex_.unlock();
}

void GlobalSynchronizationLayer::Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  mutex_.unlock();
}
%for function in functions:

void GlobalSynchronizationLayer::Pre(${function.name}Command& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(${function.name}Command& command) {
  mutex_.unlock();
}
%endfor
%for interface in interfaces:
%for function in interface.functions:

void GlobalSynchronizationLayer::Pre(${interface.name}${function.name}Command& command) {
  mutex_.lock();
}

void GlobalSynchronizationLayer::Post(${interface.name}${function.name}Command& command) {
  mutex_.unlock();
}
%endfor
%endfor

} // namespace DirectX
} // namespace gits
