// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandsAuto.h"
#include "commandsCustom.h"

namespace gits {
namespace DirectX {

void decode(char* src, IUnknownQueryInterfaceCommand& command);
void decode(char* src, IUnknownAddRefCommand& command);
void decode(char* src, IUnknownReleaseCommand& command);
void decode(char* src, CreateWindowMetaCommand& command);
void decode(char* src, MappedDataMetaCommand& command);
void decode(char* src, CreateHeapAllocationMetaCommand& command);
void decode(char* src, WaitForFenceSignaledCommand& command);
void decode(char* src, DllContainerMetaCommand& command);
void decode(char* src, INTC_D3D12_GetSupportedVersionsCommand& command);
void decode(char* src, INTC_D3D12_CreateDeviceExtensionContextCommand& command);
void decode(char* src, INTC_D3D12_CreateDeviceExtensionContext1Command& command);
void decode(char* src, INTC_D3D12_SetApplicationInfoCommand& command);
void decode(char* src, INTC_DestroyDeviceExtensionContextCommand& command);
void decode(char* src, INTC_D3D12_CheckFeatureSupportCommand& command);
void decode(char* src, INTC_D3D12_CreateCommandQueueCommand& command);
void decode(char* src, INTC_D3D12_CreateReservedResourceCommand& command);
void decode(char* src, INTC_D3D12_SetFeatureSupportCommand& command);
void decode(char* src, INTC_D3D12_GetResourceAllocationInfoCommand& command);
void decode(char* src, INTC_D3D12_CreateComputePipelineStateCommand& command);
void decode(char* src, INTC_D3D12_CreatePlacedResourceCommand& command);
void decode(char* src, INTC_D3D12_CreateCommittedResourceCommand& command);
void decode(char* src, INTC_D3D12_CreateHeapCommand& command);
void decode(char* src, NvAPI_InitializeCommand& command);
void decode(char* src, NvAPI_UnloadCommand& command);
void decode(char* src, NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command);
void decode(char* src, NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command);
void decode(char* src, NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command);
void decode(char* src, NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command);
void decode(char* src, NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command);
void decode(char* src, NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command);

} // namespace DirectX
} // namespace gits
