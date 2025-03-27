// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandEncodersCustom.h"
#include "argumentEncoders.h"

namespace gits {
namespace DirectX {

unsigned getSize(const IUnknownQueryInterfaceCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.object_.key) +
         getSize(command.riid_) + getSize(command.ppvObject_) + getSize(command.result_);
}

void encode(const IUnknownQueryInterfaceCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.object_.key);
  encode(dest, offset, command.riid_);
  encode(dest, offset, command.ppvObject_);
  encode(dest, offset, command.result_);
}

unsigned getSize(const IUnknownAddRefCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.object_.key) +
         getSize(command.result_);
}

void encode(const IUnknownAddRefCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.object_.key);
  encode(dest, offset, command.result_);
}

unsigned getSize(const IUnknownReleaseCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.object_.key) +
         getSize(command.result_);
}

void encode(const IUnknownReleaseCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.object_.key);
  encode(dest, offset, command.result_);
}

unsigned getSize(const CreateWindowMetaCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.hWnd_) +
         getSize(command.width_) + getSize(command.height_);
}

void encode(const CreateWindowMetaCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.hWnd_);
  encode(dest, offset, command.width_);
  encode(dest, offset, command.height_);
}

unsigned getSize(const MappedDataMetaCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.resource_) +
         getSize(command.mappedAddress_) + getSize(command.offset_) + getSize(command.data_);
}

void encode(const MappedDataMetaCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.resource_);
  encode(dest, offset, command.mappedAddress_);
  encode(dest, offset, command.offset_);
  encode(dest, offset, command.data_);
}

unsigned getSize(const CreateHeapAllocationMetaCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.heap_) +
         getSize(command.address_) + getSize(command.data_);
}

void encode(const CreateHeapAllocationMetaCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.heap_);
  encode(dest, offset, command.address_);
  encode(dest, offset, command.data_);
}

unsigned getSize(const WaitForFenceSignaledCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.event_) +
         getSize(command.fence_) + getSize(command.value_);
}

void encode(const WaitForFenceSignaledCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.event_);
  encode(dest, offset, command.fence_);
  encode(dest, offset, command.value_);
}

unsigned getSize(const INTC_D3D12_GetSupportedVersionsCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.pDevice_) +
         getSize(command.pSupportedExtVersions_) + getSize(command.pSupportedExtVersionsCount_) +
         getSize(command.result_);
}

void encode(const INTC_D3D12_GetSupportedVersionsCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.pDevice_);
  encode(dest, offset, command.pSupportedExtVersions_);
  encode(dest, offset, command.pSupportedExtVersionsCount_);
  encode(dest, offset, command.result_);
}

unsigned getSize(const INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.pDevice_) +
         getSize(command.ppExtensionContext_) + getSize(command.pExtensionInfo_) +
         getSize(command.pExtensionAppInfo_) + getSize(command.result_);
}

void encode(const INTC_D3D12_CreateDeviceExtensionContextCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.pDevice_);
  encode(dest, offset, command.ppExtensionContext_);
  encode(dest, offset, command.pExtensionInfo_);
  encode(dest, offset, command.pExtensionAppInfo_);
  encode(dest, offset, command.result_);
}

unsigned getSize(const INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.pDevice_) +
         getSize(command.ppExtensionContext_) + getSize(command.pExtensionInfo_) +
         getSize(command.pExtensionAppInfo_) + getSize(command.result_);
}

void encode(const INTC_D3D12_CreateDeviceExtensionContext1Command& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.pDevice_);
  encode(dest, offset, command.ppExtensionContext_);
  encode(dest, offset, command.pExtensionInfo_);
  encode(dest, offset, command.pExtensionAppInfo_);
  encode(dest, offset, command.result_);
}

unsigned getSize(const INTC_DestroyDeviceExtensionContextCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.ppExtensionContext_) +
         getSize(command.result_);
}

void encode(const INTC_DestroyDeviceExtensionContextCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.ppExtensionContext_);
  encode(dest, offset, command.result_);
}

unsigned getSize(const INTC_D3D12_CheckFeatureSupportCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.pExtensionContext_) +
         getSize(command.Feature_) + getSize(command.pFeatureSupportData_) +
         getSize(command.FeatureSupportDataSize_) + getSize(command.result_);
}

void encode(const INTC_D3D12_CheckFeatureSupportCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.pExtensionContext_);
  encode(dest, offset, command.Feature_);
  encode(dest, offset, command.pFeatureSupportData_);
  encode(dest, offset, command.FeatureSupportDataSize_);
  encode(dest, offset, command.result_);
}

unsigned getSize(const INTC_D3D12_CreateCommandQueueCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.pExtensionContext_) +
         getSize(command.pDesc_) + getSize(command.riid_) + getSize(command.ppCommandQueue_) +
         getSize(command.result_);
}

void encode(const INTC_D3D12_CreateCommandQueueCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.pExtensionContext_);
  encode(dest, offset, command.pDesc_);
  encode(dest, offset, command.riid_);
  encode(dest, offset, command.ppCommandQueue_);
  encode(dest, offset, command.result_);
}

unsigned getSize(const INTC_D3D12_CreateReservedResourceCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.pExtensionContext_) +
         getSize(command.pDesc_) + getSize(command.InitialState_) +
         getSize(command.pOptimizedClearValue_) + getSize(command.riid_) +
         getSize(command.ppvResource_) + getSize(command.result_);
}

void encode(const INTC_D3D12_CreateReservedResourceCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.pExtensionContext_);
  encode(dest, offset, command.pDesc_);
  encode(dest, offset, command.InitialState_);
  encode(dest, offset, command.pOptimizedClearValue_);
  encode(dest, offset, command.riid_);
  encode(dest, offset, command.ppvResource_);
  encode(dest, offset, command.result_);
}

unsigned getSize(const INTC_D3D12_SetFeatureSupportCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.pExtensionContext_) +
         getSize(command.pFeature_) + getSize(command.result_);
}

void encode(const INTC_D3D12_SetFeatureSupportCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.pExtensionContext_);
  encode(dest, offset, command.pFeature_);
  encode(dest, offset, command.result_);
}

unsigned getSize(const INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.pExtensionContext_) +
         getSize(command.visibleMask_) + getSize(command.numResourceDescs_) +
         getSize(command.pResourceDescs_) + getSize(command.result_);
}

void encode(const INTC_D3D12_GetResourceAllocationInfoCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.pExtensionContext_);
  encode(dest, offset, command.visibleMask_);
  encode(dest, offset, command.numResourceDescs_);
  encode(dest, offset, command.pResourceDescs_);
  encode(dest, offset, command.result_);
}

unsigned getSize(const INTC_D3D12_CreateComputePipelineStateCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.pExtensionContext_) +
         getSize(command.pDesc_) + getSize(command.riid_) + getSize(command.ppPipelineState_) +
         getSize(command.result_);
}

void encode(const INTC_D3D12_CreateComputePipelineStateCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.pExtensionContext_);
  encode(dest, offset, command.pDesc_);
  encode(dest, offset, command.riid_);
  encode(dest, offset, command.ppPipelineState_);
  encode(dest, offset, command.result_);
}

unsigned getSize(const INTC_D3D12_CreatePlacedResourceCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.pExtensionContext_) +
         getSize(command.pHeap_) + getSize(command.HeapOffset_) + getSize(command.pDesc_) +
         getSize(command.InitialState_) + getSize(command.pOptimizedClearValue_) +
         getSize(command.riid_) + getSize(command.ppvResource_) + getSize(command.result_);
}

void encode(const INTC_D3D12_CreatePlacedResourceCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.pExtensionContext_);
  encode(dest, offset, command.pHeap_);
  encode(dest, offset, command.HeapOffset_);
  encode(dest, offset, command.pDesc_);
  encode(dest, offset, command.InitialState_);
  encode(dest, offset, command.pOptimizedClearValue_);
  encode(dest, offset, command.riid_);
  encode(dest, offset, command.ppvResource_);
  encode(dest, offset, command.result_);
}

unsigned getSize(const INTC_D3D12_CreateCommittedResourceCommand& command) {
  return getSize(command.key) + getSize(command.threadId) + getSize(command.pExtensionContext_) +
         getSize(command.pHeapProperties_) + getSize(command.HeapFlags_) + getSize(command.pDesc_) +
         getSize(command.InitialResourceState_) + getSize(command.pOptimizedClearValue_) +
         getSize(command.riidResource_) + getSize(command.ppvResource_) + getSize(command.result_);
}

void encode(const INTC_D3D12_CreateCommittedResourceCommand& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.pExtensionContext_);
  encode(dest, offset, command.pHeapProperties_);
  encode(dest, offset, command.HeapFlags_);
  encode(dest, offset, command.pDesc_);
  encode(dest, offset, command.InitialResourceState_);
  encode(dest, offset, command.pOptimizedClearValue_);
  encode(dest, offset, command.riidResource_);
  encode(dest, offset, command.ppvResource_);
  encode(dest, offset, command.result_);
}

} // namespace DirectX
} // namespace gits
