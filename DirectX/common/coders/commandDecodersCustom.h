// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
void decode(char* src, INTC_D3D12_GetSupportedVersionsCommand& command);
void decode(char* src, INTC_D3D12_CreateDeviceExtensionContextCommand& command);
void decode(char* src, INTC_D3D12_CreateDeviceExtensionContext1Command& command);
void decode(char* src, INTC_DestroyDeviceExtensionContextCommand& command);
void decode(char* src, INTC_D3D12_CheckFeatureSupportCommand& command);
void decode(char* src, INTC_D3D12_CreateCommandQueueCommand& command);
void decode(char* src, INTC_D3D12_CreateReservedResourceCommand& command);
void decode(char* src, INTC_D3D12_SetFeatureSupportCommand& command);
void decode(char* src, INTC_D3D12_GetResourceAllocationInfoCommand& command);
void decode(char* src, INTC_D3D12_CreateComputePipelineStateCommand& command);
void decode(char* src, INTC_D3D12_CreatePlacedResourceCommand& command);
void decode(char* src, INTC_D3D12_CreateCommittedResourceCommand& command);

} // namespace DirectX
} // namespace gits
