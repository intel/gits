// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandDecodersCustom.h"
#include "argumentDecoders.h"

namespace gits {
namespace DirectX {

void Decode(char* src, MarkerUInt64Command& command) {
  unsigned offset = 0;
  Decode(src, offset, command.m_Value);
}

void Decode(char* src, IUnknownQueryInterfaceCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_Object.Key);
  Decode(src, offset, command.m_riid);
  Decode(src, offset, command.m_ppvObject);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, IUnknownAddRefCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_Object.Key);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, IUnknownReleaseCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_Object.Key);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, CreateWindowMetaCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_hWnd);
  Decode(src, offset, command.m_width);
  Decode(src, offset, command.m_height);
}

void Decode(char* src, MappedDataMetaCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_resource);
  Decode(src, offset, command.m_mappedAddress);
  Decode(src, offset, command.m_offset);
  Decode(src, offset, command.m_data);
}

void Decode(char* src, CreateHeapAllocationMetaCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_heap);
  Decode(src, offset, command.m_address);
  Decode(src, offset, command.m_data);
}

void Decode(char* src, WaitForFenceSignaledCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_event);
  Decode(src, offset, command.m_fence);
  Decode(src, offset, command.m_Value);
}

void Decode(char* src, DllContainerMetaCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_dllName);
  Decode(src, offset, command.m_dllData);
}

void Decode(char* src, INTC_D3D12_GetSupportedVersionsCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pDevice);
  Decode(src, offset, command.m_pSupportedExtVersions);
  Decode(src, offset, command.m_pSupportedExtVersionsCount);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pDevice);
  Decode(src, offset, command.m_ppExtensionContext);
  Decode(src, offset, command.m_pExtensionInfo);
  Decode(src, offset, command.m_pExtensionAppInfo);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pDevice);
  Decode(src, offset, command.m_ppExtensionContext);
  Decode(src, offset, command.m_pExtensionInfo);
  Decode(src, offset, command.m_pExtensionAppInfo);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_D3D12_SetApplicationInfoCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pExtensionAppInfo);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_DestroyDeviceExtensionContextCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_ppExtensionContext);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_D3D12_CheckFeatureSupportCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pExtensionContext);
  Decode(src, offset, command.m_Feature);
  Decode(src, offset, command.m_pFeatureSupportData);
  Decode(src, offset, command.m_FeatureSupportDataSize);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_D3D12_CreateCommandQueueCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pExtensionContext);
  Decode(src, offset, command.m_pDesc);
  Decode(src, offset, command.m_riid);
  Decode(src, offset, command.m_ppCommandQueue);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_D3D12_CreateReservedResourceCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pExtensionContext);
  Decode(src, offset, command.m_pDesc);
  Decode(src, offset, command.m_InitialState);
  Decode(src, offset, command.m_pOptimizedClearValue);
  Decode(src, offset, command.m_riid);
  Decode(src, offset, command.m_ppvResource);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_D3D12_SetFeatureSupportCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pExtensionContext);
  Decode(src, offset, command.m_pFeature);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pExtensionContext);
  Decode(src, offset, command.m_visibleMask);
  Decode(src, offset, command.m_numResourceDescs);
  Decode(src, offset, command.m_pResourceDescs);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_D3D12_CreateComputePipelineStateCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pExtensionContext);
  Decode(src, offset, command.m_pDesc);
  Decode(src, offset, command.m_riid);
  Decode(src, offset, command.m_ppPipelineState);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_D3D12_CreatePlacedResourceCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pExtensionContext);
  Decode(src, offset, command.m_pHeap);
  Decode(src, offset, command.m_HeapOffset);
  Decode(src, offset, command.m_pDesc);
  Decode(src, offset, command.m_InitialState);
  Decode(src, offset, command.m_pOptimizedClearValue);
  Decode(src, offset, command.m_riid);
  Decode(src, offset, command.m_ppvResource);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_D3D12_CreateCommittedResourceCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pExtensionContext);
  Decode(src, offset, command.m_pHeapProperties);
  Decode(src, offset, command.m_HeapFlags);
  Decode(src, offset, command.m_pDesc);
  Decode(src, offset, command.m_InitialResourceState);
  Decode(src, offset, command.m_pOptimizedClearValue);
  Decode(src, offset, command.m_riidResource);
  Decode(src, offset, command.m_ppvResource);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, INTC_D3D12_CreateHeapCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pExtensionContext);
  Decode(src, offset, command.m_pDesc);
  Decode(src, offset, command.m_riid);
  Decode(src, offset, command.m_ppvHeap);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, NvAPI_InitializeCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, NvAPI_UnloadCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pDevice);
  Decode(src, offset, command.m_pState);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pDev);
  Decode(src, offset, command.m_uavSlot);
  Decode(src, offset, command.m_uavSpace);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pDev);
  Decode(src, offset, command.m_uavSlot);
  Decode(src, offset, command.m_uavSpace);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pCommandList);
  Decode(src, offset, command.m_pParams);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pCommandList);
  Decode(src, offset, command.m_pParams);
  Decode(src, offset, command.m_Result);
}

void Decode(char* src, NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_pCommandList);
  Decode(src, offset, command.m_pParams);
  Decode(src, offset, command.m_Result);
}

} // namespace DirectX
} // namespace gits
