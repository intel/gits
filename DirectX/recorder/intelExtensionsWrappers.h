// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "intelExtensions.h"
#include "directx.h"

namespace gits {
namespace DirectX {

HRESULT INTC_D3D12_GetSupportedVersionsWrapper(
    PFNINTCDX12EXT_GETSUPPORTEDVERSIONS pfnGetSupportedVersions,
    const ID3D12Device* pDevice,
    INTCExtensionVersion* pSupportedExtVersions,
    uint32_t* pSupportedExtVersionsCount);

HRESULT INTC_D3D12_CreateDeviceExtensionContextWrapper(
    PFNINTCDX12EXT_CREATEDEVICEEXTENSIONCONTEXT pfnCreateDeviceExtensionContext,
    const ID3D12Device* pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo* pExtensionInfo,
    INTCExtensionAppInfo* pExtensionAppInfo);

HRESULT INTC_D3D12_CreateDeviceExtensionContext1Wrapper(
    PFNINTCDX12EXT_CREATEDEVICEEXTENSIONCONTEXT1 pfnCreateDeviceExtensionContext1,
    const ID3D12Device* pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo* pExtensionInfo,
    INTCExtensionAppInfo1* pExtensionAppInfo);

HRESULT INTC_DestroyDeviceExtensionContextWrapper(
    PFNINTCEXT_DESTROYDEVICEEXTENSIONCONTEXT pfnDestroyDeviceExtensionContext,
    INTCExtensionContext** ppExtensionContext);

HRESULT INTC_D3D12_CreateCommandQueueWrapper(
    PFNINTCDX12EXT_CREATECOMMANDQUEUE pfnCreateCommandQueue,
    INTCExtensionContext* pExtensionContext,
    const INTC_D3D12_COMMAND_QUEUE_DESC* pDesc,
    REFIID riid,
    void** ppCommandQueue);

HRESULT INTC_D3D12_CreateComputePipelineStateWrapper(
    PFNINTCDX12EXT_CREATECOMPUTEPIPELINESTATE pfnCreateComputePipelineState,
    INTCExtensionContext* pExtensionContext,
    const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
    REFIID riid,
    void** ppPipelineState);

HRESULT INTC_D3D12_CreateReservedResourceWrapper(
    PFNINTCDX12EXT_CREATERESERVEDRESOURCE pfnCreateReservedResource,
    INTCExtensionContext* pExtensionContext,
    const INTC_D3D12_RESOURCE_DESC* pDesc,
    D3D12_RESOURCE_STATES InitialState,
    const D3D12_CLEAR_VALUE* pOptimizedClearValue,
    REFIID riid,
    void** ppvResource);

HRESULT INTC_D3D12_CreateCommittedResourceWrapper(
    PFNINTCDX12EXT_CREATECOMMITTEDRESOURCE pfnCreateCommittedResource,
    INTCExtensionContext* pExtensionContext,
    const D3D12_HEAP_PROPERTIES* pHeapProperties,
    D3D12_HEAP_FLAGS HeapFlags,
    const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
    D3D12_RESOURCE_STATES InitialResourceState,
    const D3D12_CLEAR_VALUE* pOptimizedClearValue,
    REFIID riidResource,
    void** ppvResource);

HRESULT INTC_D3D12_CreateCommittedResource1Wrapper(
    PFNINTCDX12EXT_CREATECOMMITTEDRESOURCE1 pfnCreateCommittedResource1,
    INTCExtensionContext* pExtensionContext,
    const D3D12_HEAP_PROPERTIES* pHeapProperties,
    D3D12_HEAP_FLAGS HeapFlags,
    const INTC_D3D12_RESOURCE_DESC_0002* pDesc,
    D3D12_RESOURCE_STATES InitialResourceState,
    const D3D12_CLEAR_VALUE* pOptimizedClearValue,
    REFIID riidResource,
    void** ppvResource);

HRESULT INTC_D3D12_CreateHeapWrapper(PFNINTCDX12EXT_CREATEHEAP pfnCreateHeap,
                                     INTCExtensionContext* pExtensionContext,
                                     const INTC_D3D12_HEAP_DESC* pDesc,
                                     REFIID riid,
                                     void** ppvHeap);

HRESULT INTC_D3D12_CreatePlacedResourceWrapper(
    PFNINTCDX12EXT_CREATEPLACEDRESOURCE pfnCreatePlacedResource,
    INTCExtensionContext* pExtensionContext,
    ID3D12Heap* pHeap,
    UINT64 HeapOffset,
    const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
    D3D12_RESOURCE_STATES InitialState,
    const D3D12_CLEAR_VALUE* pOptimizedClearValue,
    REFIID riid,
    void** ppvResource);

HRESULT INTC_D3D12_CreateHostRTASResourceWrapper(
    PFNINTCDX12EXT_CREATEHOSTRTASRESOURCE pfnCreateHostRTASResource,
    INTCExtensionContext* pExtensionContext,
    size_t SizeInBytes,
    DWORD Flags,
    REFIID riidResource,
    void** ppvResource);

void INTC_D3D12_BuildRaytracingAccelerationStructure_HostWrapper(
    PFNINTCDX12EXT_BUILDRAYTRACINGACCELERATIONSTRUCTURE_HOST
        pfnBuildRaytracingAccelerationStructure_Host,
    INTCExtensionContext* pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* pDesc,
    const D3D12_GPU_VIRTUAL_ADDRESS* pInstanceGPUVAs,
    UINT NumInstances);

void INTC_D3D12_CopyRaytracingAccelerationStructure_HostWrapper(
    PFNINTCDX12EXT_COPYRAYTRACINGACCELERATIONSTRUCTURE_HOST
        pfnCopyRaytracingAccelerationStructure_Host,
    INTCExtensionContext* pExtensionContext,
    void* DestAccelerationStructureData,
    const void* SourceAccelerationStructureData,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode);

void INTC_D3D12_EmitRaytracingAccelerationStructurePostbuildInfo_HostWrapper(
    PFNINTCDX12EXT_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO_HOST
        pfnEmitRaytracingAccelerationStructurePostbuildInfo_Host,
    INTCExtensionContext* pExtensionContext,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE InfoType,
    void* DestBuffer,
    const void* SourceRTAS);

void INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo_HostWrapper(
    PFNINTCDX12EXT_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO_HOST
        pfnGetRaytracingAccelerationStructurePrebuildInfo_Host,
    INTCExtensionContext* pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pDesc,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO* pInfo);

void INTC_D3D12_TransferHostRTASWrapper(PFNINTCDX12EXT_TRANSFERHOSTRTAS pfnTransferHostRTAS,
                                        INTCExtensionContext* pExtensionContext,
                                        ID3D12GraphicsCommandList* pCommandList,
                                        D3D12_GPU_VIRTUAL_ADDRESS DestAccelerationStructureData,
                                        D3D12_GPU_VIRTUAL_ADDRESS SrcAccelerationStructureData,
                                        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode);

void INTC_D3D12_SetDriverEventMetadataWrapper(
    PFNINTCDX12EXT_SETDRIVEREVENTMETADATA pfnSetDriverEventMetadata,
    INTCExtensionContext* pExtensionContext,
    ID3D12GraphicsCommandList* pCommandList,
    UINT64 Metadata);

void INTC_D3D12_QueryCpuVisibleVidmemWrapper(
    PFNINTCDX12EXT_QUERYCPUVISIBLEVIDMEM pfnQueryCpuVisibleVidmem,
    INTCExtensionContext* pExtensionContext,
    UINT64* pTotalBytes,
    UINT64* pFreeBytes);

HRESULT INTC_D3D12_CreateStateObjectWrapper(PFNINTCDX12EXT_CREATESTATEOBJECT pfnCreateStateObject,
                                            INTCExtensionContext* pExtensionContext,
                                            const INTC_D3D12_STATE_OBJECT_DESC* pDesc,
                                            REFIID riid,
                                            void** ppPipelineState);

void INTC_D3D12_BuildRaytracingAccelerationStructureWrapper(
    PFNINTCDX12EXT_BUILDRAYTRACINGACCELERATIONSTRUCTURE pfnBuildRaytracingAccelerationStructure,
    INTCExtensionContext* pExtensionContext,
    ID3D12GraphicsCommandList* pCommandList,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* pDesc,
    UINT NumPostbuildInfoDescs,
    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* pPostbuildInfoDescs,
    const INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA*
        pComparisonDataDesc);

void INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfoWrapper(
    PFNINTCDX12EXT_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO
        pfnGetRaytracingAccelerationStructurePrebuildInfo,
    INTCExtensionContext* pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pDesc,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO* pInfo,
    const INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA*
        pComparisonDataDesc);

HRESULT INTC_D3D12_SetFeatureSupportWrapper(PFNINTCDX12EXT_SETFEATURESUPPORT pfnSetFeatureSupport,
                                            INTCExtensionContext* pExtensionContext,
                                            INTC_D3D12_FEATURE* pFeature);

D3D12_RESOURCE_ALLOCATION_INFO INTC_D3D12_GetResourceAllocationInfoWrapper(
    PFNINTCDX12EXT_GETRESOURCEALLOCATIONINFO pfnGetResourceAllocationInfo,
    INTCExtensionContext* pExtensionContext,
    UINT visibleMask,
    UINT numResourceDescs,
    const INTC_D3D12_RESOURCE_DESC_0001* pResourceDescs);

HRESULT INTC_D3D12_CheckFeatureSupportWrapper(
    PFNINTCDX12EXT_CHECKFEATURESUPPORT pfnCheckFeatureSupport,
    INTCExtensionContext* pExtensionContext,
    INTC_D3D12_FEATURES Feature,
    void* pFeatureSupportData,
    UINT FeatureSupportDataSize);

HRESULT INTC_D3D12_AddShaderBinariesPathWrapper(
    PFNINTCDX12EXT_ADDSHADERBINARIESPATH pfnAddShaderBinariesPath,
    INTCExtensionContext* pExtensionContext,
    const wchar_t* filePath);

HRESULT INTC_D3D12_RemoveShaderBinariesPathWrapper(
    PFNINTCDX12EXT_REMOVESHADERBINARIESPATH pfnRemoveShaderBinariesPath,
    INTCExtensionContext* pExtensionContext,
    const wchar_t* filePath);

HRESULT INTC_D3D12_SetApplicationInfoWrapper(
    PFNINTCDX12EXT_SETAPPLICATIONINFO pfnSetApplicationInfo,
    INTCExtensionAppInfo1* pExtensionAppInfo);

} // namespace DirectX
} // namespace gits
