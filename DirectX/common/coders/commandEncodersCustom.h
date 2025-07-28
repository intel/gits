// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandsCustom.h"

namespace gits {
namespace DirectX {

unsigned getSize(const IUnknownQueryInterfaceCommand& command);
void encode(const IUnknownQueryInterfaceCommand& command, char* dest);

unsigned getSize(const IUnknownAddRefCommand& command);
void encode(const IUnknownAddRefCommand& command, char* dest);

unsigned getSize(const IUnknownReleaseCommand& command);
void encode(const IUnknownReleaseCommand& command, char* dest);

unsigned getSize(const CreateWindowMetaCommand& command);
void encode(const CreateWindowMetaCommand& command, char* dest);

unsigned getSize(const MappedDataMetaCommand& command);
void encode(const MappedDataMetaCommand& command, char* dest);

unsigned getSize(const CreateHeapAllocationMetaCommand& command);
void encode(const CreateHeapAllocationMetaCommand& command, char* dest);

unsigned getSize(const WaitForFenceSignaledCommand& command);
void encode(const WaitForFenceSignaledCommand& command, char* dest);

unsigned getSize(const INTC_D3D12_GetSupportedVersionsCommand& command);
void encode(const INTC_D3D12_GetSupportedVersionsCommand& command, char* dest);

unsigned getSize(const INTC_D3D12_CreateDeviceExtensionContextCommand& command);
void encode(const INTC_D3D12_CreateDeviceExtensionContextCommand& command, char* dest);

unsigned getSize(const INTC_D3D12_CreateDeviceExtensionContext1Command& command);
void encode(const INTC_D3D12_CreateDeviceExtensionContext1Command& command, char* dest);

unsigned getSize(const INTC_D3D12_SetApplicationInfoCommand& command);
void encode(const INTC_D3D12_SetApplicationInfoCommand& command, char* dest);

unsigned getSize(const INTC_DestroyDeviceExtensionContextCommand& command);
void encode(const INTC_DestroyDeviceExtensionContextCommand& command, char* dest);

unsigned getSize(const INTC_D3D12_CheckFeatureSupportCommand& command);
void encode(const INTC_D3D12_CheckFeatureSupportCommand& command, char* dest);

unsigned getSize(const INTC_D3D12_CreateCommandQueueCommand& command);
void encode(const INTC_D3D12_CreateCommandQueueCommand& command, char* dest);

unsigned getSize(const INTC_D3D12_CreateReservedResourceCommand& command);
void encode(const INTC_D3D12_CreateReservedResourceCommand& command, char* dest);

unsigned getSize(const INTC_D3D12_SetFeatureSupportCommand& command);
void encode(const INTC_D3D12_SetFeatureSupportCommand& command, char* dest);

unsigned getSize(const INTC_D3D12_GetResourceAllocationInfoCommand& command);
void encode(const INTC_D3D12_GetResourceAllocationInfoCommand& command, char* dest);

unsigned getSize(const INTC_D3D12_CreateComputePipelineStateCommand& command);
void encode(const INTC_D3D12_CreateComputePipelineStateCommand& command, char* dest);

unsigned getSize(const INTC_D3D12_CreatePlacedResourceCommand& command);
void encode(const INTC_D3D12_CreatePlacedResourceCommand& command, char* dest);

unsigned getSize(const INTC_D3D12_CreateCommittedResourceCommand& command);
void encode(const INTC_D3D12_CreateCommittedResourceCommand& command, char* dest);

unsigned getSize(const INTC_D3D12_CreateHeapCommand& command);
void encode(const INTC_D3D12_CreateHeapCommand& command, char* dest);

unsigned getSize(const NvAPI_InitializeCommand& command);
void encode(const NvAPI_InitializeCommand& command, char* dest);

unsigned getSize(const NvAPI_UnloadCommand& command);
void encode(const NvAPI_UnloadCommand& command, char* dest);

unsigned getSize(const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command);
void encode(const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command, char* dest);

unsigned getSize(const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command);
void encode(const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command, char* dest);

unsigned getSize(const NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command);
void encode(const NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command, char* dest);

unsigned getSize(const NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command);
void encode(const NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command, char* dest);

unsigned getSize(const NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command);
void encode(const NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command,
            char* dest);

} // namespace DirectX
} // namespace gits
