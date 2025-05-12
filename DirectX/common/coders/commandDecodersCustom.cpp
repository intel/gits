// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandDecodersCustom.h"
#include "argumentDecoders.h"

namespace gits {
namespace DirectX {

#pragma region IUnknown

void decode(char* src, IUnknownQueryInterfaceCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.object_.key);
  decode(src, offset, command.riid_);
  decode(src, offset, command.ppvObject_);
  decode(src, offset, command.result_);
}

void decode(char* src, IUnknownAddRefCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.object_.key);
  decode(src, offset, command.result_);
}

void decode(char* src, IUnknownReleaseCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.object_.key);
  decode(src, offset, command.result_);
}

#pragma endregion

void decode(char* src, CreateWindowMetaCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.hWnd_);
  decode(src, offset, command.width_);
  decode(src, offset, command.height_);
}

void decode(char* src, MappedDataMetaCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.resource_);
  decode(src, offset, command.mappedAddress_);
  decode(src, offset, command.offset_);
  decode(src, offset, command.data_);
}

void decode(char* src, CreateHeapAllocationMetaCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.heap_);
  decode(src, offset, command.address_);
  decode(src, offset, command.data_);
}

void decode(char* src, WaitForFenceSignaledCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.event_);
  decode(src, offset, command.fence_);
  decode(src, offset, command.value_);
}

void decode(char* src, INTC_D3D12_GetSupportedVersionsCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pDevice_);
  decode(src, offset, command.pSupportedExtVersions_);
  decode(src, offset, command.pSupportedExtVersionsCount_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pDevice_);
  decode(src, offset, command.ppExtensionContext_);
  decode(src, offset, command.pExtensionInfo_);
  decode(src, offset, command.pExtensionAppInfo_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pDevice_);
  decode(src, offset, command.ppExtensionContext_);
  decode(src, offset, command.pExtensionInfo_);
  decode(src, offset, command.pExtensionAppInfo_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_D3D12_SetApplicationInfoCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pExtensionAppInfo_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_DestroyDeviceExtensionContextCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.ppExtensionContext_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_D3D12_CheckFeatureSupportCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pExtensionContext_);
  decode(src, offset, command.Feature_);
  decode(src, offset, command.pFeatureSupportData_);
  decode(src, offset, command.FeatureSupportDataSize_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_D3D12_CreateCommandQueueCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pExtensionContext_);
  decode(src, offset, command.pDesc_);
  decode(src, offset, command.riid_);
  decode(src, offset, command.ppCommandQueue_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_D3D12_CreateReservedResourceCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pExtensionContext_);
  decode(src, offset, command.pDesc_);
  decode(src, offset, command.InitialState_);
  decode(src, offset, command.pOptimizedClearValue_);
  decode(src, offset, command.riid_);
  decode(src, offset, command.ppvResource_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_D3D12_SetFeatureSupportCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pExtensionContext_);
  decode(src, offset, command.pFeature_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pExtensionContext_);
  decode(src, offset, command.visibleMask_);
  decode(src, offset, command.numResourceDescs_);
  decode(src, offset, command.pResourceDescs_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_D3D12_CreateComputePipelineStateCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pExtensionContext_);
  decode(src, offset, command.pDesc_);
  decode(src, offset, command.riid_);
  decode(src, offset, command.ppPipelineState_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_D3D12_CreatePlacedResourceCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pExtensionContext_);
  decode(src, offset, command.pHeap_);
  decode(src, offset, command.HeapOffset_);
  decode(src, offset, command.pDesc_);
  decode(src, offset, command.InitialState_);
  decode(src, offset, command.pOptimizedClearValue_);
  decode(src, offset, command.riid_);
  decode(src, offset, command.ppvResource_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_D3D12_CreateCommittedResourceCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pExtensionContext_);
  decode(src, offset, command.pHeapProperties_);
  decode(src, offset, command.HeapFlags_);
  decode(src, offset, command.pDesc_);
  decode(src, offset, command.InitialResourceState_);
  decode(src, offset, command.pOptimizedClearValue_);
  decode(src, offset, command.riidResource_);
  decode(src, offset, command.ppvResource_);
  decode(src, offset, command.result_);
}

void decode(char* src, INTC_D3D12_CreateHeapCommand& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.pExtensionContext_);
  decode(src, offset, command.pDesc_);
  decode(src, offset, command.riid_);
  decode(src, offset, command.ppvHeap_);
  decode(src, offset, command.result_);
}

} // namespace DirectX
} // namespace gits
