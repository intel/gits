// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
NvAPI_Status NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadWrapper(IUnknown* pDev,
                                                                    NvU32 uavSlot,
                                                                    NvU32 uavSpace);
NvAPI_Status NvAPI_D3D12_BuildRaytracingAccelerationStructureExWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    const NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS* pParams);
NvAPI_Status NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS* pParams);

} // namespace DirectX
} // namespace gits
