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

struct NvAPIDispatchTable {
  void* (*nvapi_QueryInterface)(unsigned int);
  decltype(NvAPI_Initialize)* NvAPI_Initialize;
  decltype(NvAPI_Unload)* NvAPI_Unload;
  decltype(NvAPI_D3D12_SetCreatePipelineStateOptions)* NvAPI_D3D12_SetCreatePipelineStateOptions;
  decltype(NvAPI_D3D12_SetNvShaderExtnSlotSpace)* NvAPI_D3D12_SetNvShaderExtnSlotSpace;
  decltype(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread)*
      NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread;
  decltype(NvAPI_D3D12_BuildRaytracingOpacityMicromapArray)*
      NvAPI_D3D12_BuildRaytracingOpacityMicromapArray;
  decltype(NvAPI_D3D12_RelocateRaytracingOpacityMicromapArray)*
      NvAPI_D3D12_RelocateRaytracingOpacityMicromapArray;
  decltype(NvAPI_D3D12_EmitRaytracingOpacityMicromapArrayPostbuildInfo)*
      NvAPI_D3D12_EmitRaytracingOpacityMicromapArrayPostbuildInfo;
  decltype(NvAPI_D3D12_BuildRaytracingAccelerationStructureEx)*
      NvAPI_D3D12_BuildRaytracingAccelerationStructureEx;
  decltype(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation)*
      NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation;
  decltype(NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirect)*
      NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirect;
};

} // namespace DirectX
} // namespace gits
