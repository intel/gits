// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "nvapi.h"

namespace gits {
namespace DirectX {

void* nvapi_QueryInterfaceWrapper(unsigned int id);
NvAPI_Status NvAPI_InitializeWrapper();
NvAPI_Status NvAPI_UnloadWrapper();
NvAPI_Status NvAPI_D3D12_SetCreatePipelineStateOptionsWrapper(
    ID3D12Device5* pDevice, const NVAPI_D3D12_SET_CREATE_PIPELINE_STATE_OPTIONS_PARAMS* pState);
NvAPI_Status NvAPI_D3D12_SetNvShaderExtnSlotSpaceWrapper(IUnknown* pDev,
                                                         NvU32 uavSlot,
                                                         NvU32 uavSpace);
NvAPI_Status NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadWrapper(IUnknown* pDev,
                                                                    NvU32 uavSlot,
                                                                    NvU32 uavSpace);
NvAPI_Status NvAPI_D3D12_BuildRaytracingAccelerationStructureExWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    const NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS* pParams);
NvAPI_Status NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS* pParams);
NvAPI_Status NvAPI_D3D12_RelocateRaytracingOpacityMicromapArrayWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    const NVAPI_RELOCATE_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS* pParams);
NvAPI_Status NvAPI_D3D12_EmitRaytracingOpacityMicromapArrayPostbuildInfoWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    const NVAPI_EMIT_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_PARAMS* pParams);
NvAPI_Status NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    const NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS* pParams);
NvAPI_Status NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirectWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    const NVAPI_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PARAMS* pParams);

} // namespace DirectX
} // namespace gits
