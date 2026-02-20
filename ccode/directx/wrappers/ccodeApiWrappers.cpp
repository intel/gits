// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directx/wrappers/ccodeApiWrappers.h"

HRESULT CC_INTC_D3D12_CreateCommandQueue(INTCExtensionContext* pExtensionContext,
                                         const INTC_D3D12_COMMAND_QUEUE_DESC* pDesc,
                                         REFIID riid,
                                         void** ppCommandQueue) {
  return INTC_D3D12_CreateCommandQueue(pExtensionContext, pDesc, riid, ppCommandQueue);
}

HRESULT CC_INTC_D3D12_CreateComputePipelineState(
    INTCExtensionContext* pExtensionContext,
    const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
    REFIID riid,
    void** ppPipelineState) {
  return INTC_D3D12_CreateComputePipelineState(pExtensionContext, pDesc, riid, ppPipelineState);
}

HRESULT CC_INTC_D3D12_CreateReservedResource(INTCExtensionContext* pExtensionContext,
                                             const INTC_D3D12_RESOURCE_DESC* pDesc,
                                             D3D12_RESOURCE_STATES InitialState,
                                             const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                             REFIID riid,
                                             void** ppvResource) {
  return INTC_D3D12_CreateReservedResource(pExtensionContext, pDesc, InitialState,
                                           pOptimizedClearValue, riid, ppvResource);
}

HRESULT CC_INTC_D3D12_CreateCommittedResource(INTCExtensionContext* pExtensionContext,
                                              const D3D12_HEAP_PROPERTIES* pHeapProperties,
                                              D3D12_HEAP_FLAGS HeapFlags,
                                              const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
                                              D3D12_RESOURCE_STATES InitialResourceState,
                                              const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                              REFIID riidResource,
                                              void** ppvResource) {
  return INTC_D3D12_CreateCommittedResource(pExtensionContext, pHeapProperties, HeapFlags, pDesc,
                                            InitialResourceState, pOptimizedClearValue,
                                            riidResource, ppvResource);
}

HRESULT CC_INTC_D3D12_CreateHeap(INTCExtensionContext* pExtensionContext,
                                 const INTC_D3D12_HEAP_DESC* pDesc,
                                 REFIID riid,
                                 void** ppvHeap) {
  return INTC_D3D12_CreateHeap(pExtensionContext, pDesc, riid, ppvHeap);
}

HRESULT CC_INTC_D3D12_CreatePlacedResource(INTCExtensionContext* pExtensionContext,
                                           ID3D12Heap* pHeap,
                                           UINT64 HeapOffset,
                                           const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
                                           D3D12_RESOURCE_STATES InitialState,
                                           const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                           REFIID riid,
                                           void** ppvResource) {
  return INTC_D3D12_CreatePlacedResource(pExtensionContext, pHeap, HeapOffset, pDesc, InitialState,
                                         pOptimizedClearValue, riid, ppvResource);
}

HRESULT CC_INTC_D3D12_SetFeatureSupport(INTCExtensionContext* pExtensionContext,
                                        INTC_D3D12_FEATURE* pFeature) {
  return INTC_D3D12_SetFeatureSupport(pExtensionContext, pFeature);
}

HRESULT CC_INTC_D3D12_CheckFeatureSupport(INTCExtensionContext* pExtensionContext,
                                          INTC_D3D12_FEATURES Feature,
                                          void* pFeatureSupportData,
                                          UINT FeatureSupportDataSize) {
  return INTC_D3D12_CheckFeatureSupport(pExtensionContext, Feature, pFeatureSupportData,
                                        FeatureSupportDataSize);
}

HRESULT CC_INTC_D3D12_SetApplicationInfo(INTCExtensionAppInfo1* pExtensionAppInfo) {
  return INTC_D3D12_SetApplicationInfo(pExtensionAppInfo);
}

HRESULT CC_INTC_D3D12_GetSupportedVersions(const ID3D12Device* pDevice,
                                           INTCExtensionVersion* pSupportedExtVersions,
                                           uint32_t* pSupportedExtVersionsCount) {
  return INTC_D3D12_GetSupportedVersions(pDevice, pSupportedExtVersions,
                                         pSupportedExtVersionsCount);
}

HRESULT CC_INTC_D3D12_CreateDeviceExtensionContext(const ID3D12Device* pDevice,
                                                   INTCExtensionContext** ppExtensionContext,
                                                   INTCExtensionInfo* pExtensionInfo,
                                                   INTCExtensionAppInfo* pExtensionAppInfo) {
  return INTC_D3D12_CreateDeviceExtensionContext(pDevice, ppExtensionContext, pExtensionInfo,
                                                 pExtensionAppInfo);
}

HRESULT CC_INTC_D3D12_CreateDeviceExtensionContext1(const ID3D12Device* pDevice,
                                                    INTCExtensionContext** ppExtensionContext,
                                                    INTCExtensionInfo* pExtensionInfo,
                                                    INTCExtensionAppInfo1* pExtensionAppInfo) {
  return INTC_D3D12_CreateDeviceExtensionContext1(pDevice, ppExtensionContext, pExtensionInfo,
                                                  pExtensionAppInfo);
}

D3D12_RESOURCE_ALLOCATION_INFO CC_INTC_D3D12_GetResourceAllocationInfo(
    INTCExtensionContext* pExtensionContext,
    UINT visibleMask,
    UINT numResourceDescs,
    const INTC_D3D12_RESOURCE_DESC_0001* pResourceDescs) {
  return INTC_D3D12_GetResourceAllocationInfo(pExtensionContext, visibleMask, numResourceDescs,
                                              pResourceDescs);
}

HRESULT CC_INTC_DestroyDeviceExtensionContext(INTCExtensionContext** ppExtensionContext) {
  return INTC_DestroyDeviceExtensionContext(ppExtensionContext);
}
