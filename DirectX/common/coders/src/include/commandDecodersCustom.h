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

void Decode(char* src, MarkerUInt64Command& command);
void Decode(char* src, IUnknownQueryInterfaceCommand& command);
void Decode(char* src, IUnknownAddRefCommand& command);
void Decode(char* src, IUnknownReleaseCommand& command);
void Decode(char* src, CreateWindowMetaCommand& command);
void Decode(char* src, MappedDataMetaCommand& command);
void Decode(char* src, CreateHeapAllocationMetaCommand& command);
void Decode(char* src, WaitForFenceSignaledCommand& command);
void Decode(char* src, DllContainerMetaCommand& command);
void Decode(char* src, INTC_D3D12_GetSupportedVersionsCommand& command);
void Decode(char* src, INTC_D3D12_CreateDeviceExtensionContextCommand& command);
void Decode(char* src, INTC_D3D12_CreateDeviceExtensionContext1Command& command);
void Decode(char* src, INTC_D3D12_SetApplicationInfoCommand& command);
void Decode(char* src, INTC_DestroyDeviceExtensionContextCommand& command);
void Decode(char* src, INTC_D3D12_CheckFeatureSupportCommand& command);
void Decode(char* src, INTC_D3D12_CreateCommandQueueCommand& command);
void Decode(char* src, INTC_D3D12_CreateReservedResourceCommand& command);
void Decode(char* src, INTC_D3D12_SetFeatureSupportCommand& command);
void Decode(char* src, INTC_D3D12_GetResourceAllocationInfoCommand& command);
void Decode(char* src, INTC_D3D12_CreateComputePipelineStateCommand& command);
void Decode(char* src, INTC_D3D12_CreatePlacedResourceCommand& command);
void Decode(char* src, INTC_D3D12_CreateCommittedResourceCommand& command);
void Decode(char* src, INTC_D3D12_CreateHeapCommand& command);
void Decode(char* src, NvAPI_InitializeCommand& command);
void Decode(char* src, NvAPI_UnloadCommand& command);
void Decode(char* src, NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command);
void Decode(char* src, NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command);
void Decode(char* src, NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command);
void Decode(char* src, NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command);
void Decode(char* src, NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command);
void Decode(char* src, NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command);

} // namespace DirectX
} // namespace gits
