// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandsCustom.h"

namespace gits {
namespace DirectX {

unsigned GetSize(const MarkerUInt64Command& command);
void Encode(const MarkerUInt64Command& command, char* dest);

unsigned GetSize(const IUnknownQueryInterfaceCommand& command);
void Encode(const IUnknownQueryInterfaceCommand& command, char* dest);

unsigned GetSize(const IUnknownAddRefCommand& command);
void Encode(const IUnknownAddRefCommand& command, char* dest);

unsigned GetSize(const IUnknownReleaseCommand& command);
void Encode(const IUnknownReleaseCommand& command, char* dest);

unsigned GetSize(const CreateWindowMetaCommand& command);
void Encode(const CreateWindowMetaCommand& command, char* dest);

unsigned GetSize(const MappedDataMetaCommand& command);
void Encode(const MappedDataMetaCommand& command, char* dest);

unsigned GetSize(const CreateHeapAllocationMetaCommand& command);
void Encode(const CreateHeapAllocationMetaCommand& command, char* dest);

unsigned GetSize(const WaitForFenceSignaledCommand& command);
void Encode(const WaitForFenceSignaledCommand& command, char* dest);

unsigned GetSize(const DllContainerMetaCommand& command);
void Encode(const DllContainerMetaCommand& command, char* dest);

unsigned GetSize(const INTC_D3D12_GetSupportedVersionsCommand& command);
void Encode(const INTC_D3D12_GetSupportedVersionsCommand& command, char* dest);

unsigned GetSize(const INTC_D3D12_CreateDeviceExtensionContextCommand& command);
void Encode(const INTC_D3D12_CreateDeviceExtensionContextCommand& command, char* dest);

unsigned GetSize(const INTC_D3D12_CreateDeviceExtensionContext1Command& command);
void Encode(const INTC_D3D12_CreateDeviceExtensionContext1Command& command, char* dest);

unsigned GetSize(const INTC_D3D12_SetApplicationInfoCommand& command);
void Encode(const INTC_D3D12_SetApplicationInfoCommand& command, char* dest);

unsigned GetSize(const INTC_DestroyDeviceExtensionContextCommand& command);
void Encode(const INTC_DestroyDeviceExtensionContextCommand& command, char* dest);

unsigned GetSize(const INTC_D3D12_CheckFeatureSupportCommand& command);
void Encode(const INTC_D3D12_CheckFeatureSupportCommand& command, char* dest);

unsigned GetSize(const INTC_D3D12_CreateCommandQueueCommand& command);
void Encode(const INTC_D3D12_CreateCommandQueueCommand& command, char* dest);

unsigned GetSize(const INTC_D3D12_CreateReservedResourceCommand& command);
void Encode(const INTC_D3D12_CreateReservedResourceCommand& command, char* dest);

unsigned GetSize(const INTC_D3D12_SetFeatureSupportCommand& command);
void Encode(const INTC_D3D12_SetFeatureSupportCommand& command, char* dest);

unsigned GetSize(const INTC_D3D12_GetResourceAllocationInfoCommand& command);
void Encode(const INTC_D3D12_GetResourceAllocationInfoCommand& command, char* dest);

unsigned GetSize(const INTC_D3D12_CreateComputePipelineStateCommand& command);
void Encode(const INTC_D3D12_CreateComputePipelineStateCommand& command, char* dest);

unsigned GetSize(const INTC_D3D12_CreatePlacedResourceCommand& command);
void Encode(const INTC_D3D12_CreatePlacedResourceCommand& command, char* dest);

unsigned GetSize(const INTC_D3D12_CreateCommittedResourceCommand& command);
void Encode(const INTC_D3D12_CreateCommittedResourceCommand& command, char* dest);

unsigned GetSize(const INTC_D3D12_CreateHeapCommand& command);
void Encode(const INTC_D3D12_CreateHeapCommand& command, char* dest);

unsigned GetSize(const NvAPI_InitializeCommand& command);
void Encode(const NvAPI_InitializeCommand& command, char* dest);

unsigned GetSize(const NvAPI_UnloadCommand& command);
void Encode(const NvAPI_UnloadCommand& command, char* dest);

unsigned GetSize(const NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command);
void Encode(const NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command, char* dest);

unsigned GetSize(const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command);
void Encode(const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command, char* dest);

unsigned GetSize(const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command);
void Encode(const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command, char* dest);

unsigned GetSize(const NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command);
void Encode(const NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command, char* dest);

unsigned GetSize(const NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command);
void Encode(const NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command, char* dest);

unsigned GetSize(const NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command);
void Encode(const NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command,
            char* dest);

} // namespace DirectX
} // namespace gits
