// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandEncodersCustom.h"
#include "argumentEncoders.h"

namespace gits {
namespace DirectX {

unsigned GetSize(const MarkerUInt64Command& command) {
  return GetSize(command.m_Value);
}

void Encode(const MarkerUInt64Command& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.m_Value);
}

unsigned GetSize(const IUnknownQueryInterfaceCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_Object.Key) +
         GetSize(command.m_riid) + GetSize(command.m_ppvObject) + GetSize(command.m_Result);
}

void Encode(const IUnknownQueryInterfaceCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_Object.Key);
  Encode(dest, offset, command.m_riid);
  Encode(dest, offset, command.m_ppvObject);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const IUnknownAddRefCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_Object.Key) +
         GetSize(command.m_Result);
}

void Encode(const IUnknownAddRefCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_Object.Key);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const IUnknownReleaseCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_Object.Key) +
         GetSize(command.m_Result);
}

void Encode(const IUnknownReleaseCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_Object.Key);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const CreateWindowMetaCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_hWnd) +
         GetSize(command.m_width) + GetSize(command.m_height);
}

void Encode(const CreateWindowMetaCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_hWnd);
  Encode(dest, offset, command.m_width);
  Encode(dest, offset, command.m_height);
}

unsigned GetSize(const MappedDataMetaCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_resource) +
         GetSize(command.m_mappedAddress) + GetSize(command.m_offset) + GetSize(command.m_data);
}

void Encode(const MappedDataMetaCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_resource);
  Encode(dest, offset, command.m_mappedAddress);
  Encode(dest, offset, command.m_offset);
  Encode(dest, offset, command.m_data);
}

unsigned GetSize(const CreateHeapAllocationMetaCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_heap) +
         GetSize(command.m_address) + GetSize(command.m_data);
}

void Encode(const CreateHeapAllocationMetaCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_heap);
  Encode(dest, offset, command.m_address);
  Encode(dest, offset, command.m_data);
}

unsigned GetSize(const WaitForFenceSignaledCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_event) +
         GetSize(command.m_fence) + GetSize(command.m_Value);
}

void Encode(const WaitForFenceSignaledCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_event);
  Encode(dest, offset, command.m_fence);
  Encode(dest, offset, command.m_Value);
}

unsigned GetSize(const DllContainerMetaCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_dllName) +
         GetSize(command.m_dllData);
}

void Encode(const DllContainerMetaCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_dllName);
  Encode(dest, offset, command.m_dllData);
}

unsigned GetSize(const INTC_D3D12_GetSupportedVersionsCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pDevice) +
         GetSize(command.m_pSupportedExtVersions) + GetSize(command.m_pSupportedExtVersionsCount) +
         GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_GetSupportedVersionsCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pDevice);
  Encode(dest, offset, command.m_pSupportedExtVersions);
  Encode(dest, offset, command.m_pSupportedExtVersionsCount);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pDevice) +
         GetSize(command.m_ppExtensionContext) + GetSize(command.m_pExtensionInfo) +
         GetSize(command.m_pExtensionAppInfo) + GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_CreateDeviceExtensionContextCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pDevice);
  Encode(dest, offset, command.m_ppExtensionContext);
  Encode(dest, offset, command.m_pExtensionInfo);
  Encode(dest, offset, command.m_pExtensionAppInfo);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pDevice) +
         GetSize(command.m_ppExtensionContext) + GetSize(command.m_pExtensionInfo) +
         GetSize(command.m_pExtensionAppInfo) + GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_CreateDeviceExtensionContext1Command& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pDevice);
  Encode(dest, offset, command.m_ppExtensionContext);
  Encode(dest, offset, command.m_pExtensionInfo);
  Encode(dest, offset, command.m_pExtensionAppInfo);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_D3D12_SetApplicationInfoCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pExtensionAppInfo) +
         GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_SetApplicationInfoCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pExtensionAppInfo);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_DestroyDeviceExtensionContextCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_ppExtensionContext) +
         GetSize(command.m_Result);
}

void Encode(const INTC_DestroyDeviceExtensionContextCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_ppExtensionContext);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_D3D12_CheckFeatureSupportCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pExtensionContext) +
         GetSize(command.m_Feature) + GetSize(command.m_pFeatureSupportData) +
         GetSize(command.m_FeatureSupportDataSize) + GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_CheckFeatureSupportCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pExtensionContext);
  Encode(dest, offset, command.m_Feature);
  Encode(dest, offset, command.m_pFeatureSupportData);
  Encode(dest, offset, command.m_FeatureSupportDataSize);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_D3D12_CreateCommandQueueCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pExtensionContext) +
         GetSize(command.m_pDesc) + GetSize(command.m_riid) + GetSize(command.m_ppCommandQueue) +
         GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_CreateCommandQueueCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pExtensionContext);
  Encode(dest, offset, command.m_pDesc);
  Encode(dest, offset, command.m_riid);
  Encode(dest, offset, command.m_ppCommandQueue);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_D3D12_CreateReservedResourceCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pExtensionContext) +
         GetSize(command.m_pDesc) + GetSize(command.m_InitialState) +
         GetSize(command.m_pOptimizedClearValue) + GetSize(command.m_riid) +
         GetSize(command.m_ppvResource) + GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_CreateReservedResourceCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pExtensionContext);
  Encode(dest, offset, command.m_pDesc);
  Encode(dest, offset, command.m_InitialState);
  Encode(dest, offset, command.m_pOptimizedClearValue);
  Encode(dest, offset, command.m_riid);
  Encode(dest, offset, command.m_ppvResource);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_D3D12_SetFeatureSupportCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pExtensionContext) +
         GetSize(command.m_pFeature) + GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_SetFeatureSupportCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pExtensionContext);
  Encode(dest, offset, command.m_pFeature);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pExtensionContext) +
         GetSize(command.m_visibleMask) + GetSize(command.m_numResourceDescs) +
         GetSize(command.m_pResourceDescs) + GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_GetResourceAllocationInfoCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pExtensionContext);
  Encode(dest, offset, command.m_visibleMask);
  Encode(dest, offset, command.m_numResourceDescs);
  Encode(dest, offset, command.m_pResourceDescs);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_D3D12_CreateComputePipelineStateCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pExtensionContext) +
         GetSize(command.m_pDesc) + GetSize(command.m_riid) + GetSize(command.m_ppPipelineState) +
         GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_CreateComputePipelineStateCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pExtensionContext);
  Encode(dest, offset, command.m_pDesc);
  Encode(dest, offset, command.m_riid);
  Encode(dest, offset, command.m_ppPipelineState);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_D3D12_CreatePlacedResourceCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pExtensionContext) +
         GetSize(command.m_pHeap) + GetSize(command.m_HeapOffset) + GetSize(command.m_pDesc) +
         GetSize(command.m_InitialState) + GetSize(command.m_pOptimizedClearValue) +
         GetSize(command.m_riid) + GetSize(command.m_ppvResource) + GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_CreatePlacedResourceCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pExtensionContext);
  Encode(dest, offset, command.m_pHeap);
  Encode(dest, offset, command.m_HeapOffset);
  Encode(dest, offset, command.m_pDesc);
  Encode(dest, offset, command.m_InitialState);
  Encode(dest, offset, command.m_pOptimizedClearValue);
  Encode(dest, offset, command.m_riid);
  Encode(dest, offset, command.m_ppvResource);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_D3D12_CreateCommittedResourceCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pExtensionContext) +
         GetSize(command.m_pHeapProperties) + GetSize(command.m_HeapFlags) +
         GetSize(command.m_pDesc) + GetSize(command.m_InitialResourceState) +
         GetSize(command.m_pOptimizedClearValue) + GetSize(command.m_riidResource) +
         GetSize(command.m_ppvResource) + GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_CreateCommittedResourceCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pExtensionContext);
  Encode(dest, offset, command.m_pHeapProperties);
  Encode(dest, offset, command.m_HeapFlags);
  Encode(dest, offset, command.m_pDesc);
  Encode(dest, offset, command.m_InitialResourceState);
  Encode(dest, offset, command.m_pOptimizedClearValue);
  Encode(dest, offset, command.m_riidResource);
  Encode(dest, offset, command.m_ppvResource);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const INTC_D3D12_CreateHeapCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pExtensionContext) +
         GetSize(command.m_pDesc) + GetSize(command.m_riid) + GetSize(command.m_ppvHeap) +
         GetSize(command.m_Result);
}

void Encode(const INTC_D3D12_CreateHeapCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pExtensionContext);
  Encode(dest, offset, command.m_pDesc);
  Encode(dest, offset, command.m_riid);
  Encode(dest, offset, command.m_ppvHeap);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const NvAPI_InitializeCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_Result);
}

void Encode(const NvAPI_InitializeCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const NvAPI_UnloadCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_Result);
}

void Encode(const NvAPI_UnloadCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pDevice) +
         GetSize(command.m_pState) + GetSize(command.m_Result);
}

void Encode(const NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pDevice);
  Encode(dest, offset, command.m_pState);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pDev) +
         GetSize(command.m_uavSlot) + GetSize(command.m_uavSpace) + GetSize(command.m_Result);
}

void Encode(const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pDev);
  Encode(dest, offset, command.m_uavSlot);
  Encode(dest, offset, command.m_uavSpace);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pDev) +
         GetSize(command.m_uavSlot) + GetSize(command.m_uavSpace) + GetSize(command.m_Result);
}

void Encode(const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pDev);
  Encode(dest, offset, command.m_uavSlot);
  Encode(dest, offset, command.m_uavSpace);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pCommandList) +
         GetSize(command.m_pParams) + GetSize(command.m_Result);
}

void Encode(const NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pCommandList);
  Encode(dest, offset, command.m_pParams);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pCommandList) +
         GetSize(command.m_pParams) + GetSize(command.m_Result);
}

void Encode(const NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pCommandList);
  Encode(dest, offset, command.m_pParams);
  Encode(dest, offset, command.m_Result);
}

unsigned GetSize(const NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  return GetSize(command.Key) + GetSize(command.ThreadId) + GetSize(command.m_pCommandList) +
         GetSize(command.m_pParams) + GetSize(command.m_Result);
}

void Encode(const NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command,
            char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_pCommandList);
  Encode(dest, offset, command.m_pParams);
  Encode(dest, offset, command.m_Result);
}

} // namespace DirectX
} // namespace gits
