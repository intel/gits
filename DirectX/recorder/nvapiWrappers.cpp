// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureManager.h"
#include "nvapiWrappers.h"
#include "commandsCustom.h"
#include "interfaceArgumentUpdaters.h"
#include "log2.h"

namespace gits {
namespace DirectX {

void* nvapi_QueryInterfaceWrapper(unsigned int id) {
  auto& manager = CaptureManager::get();

  const auto& nvapiFunctionIds = manager.getNvAPIFunctionIds();
  for (const auto& [functionName, functionId] : nvapiFunctionIds) {
    if (functionId == id) {
      LOG_VERBOSE << functionName;
      break;
    }
  }

  auto result = manager.getNvAPIDispatchTable().nvapi_QueryInterface(id);

  return result;
}

NvAPI_Status NvAPI_InitializeWrapper() {
  NvAPI_Status result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    NvAPI_InitializeCommand command(GetCurrentThreadId());

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = manager.getNvAPIDispatchTable().NvAPI_Initialize();
    }
    command.result_.value = result;
    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = manager.getNvAPIDispatchTable().NvAPI_Initialize();
  }

  return result;
}

NvAPI_Status NvAPI_UnloadWrapper() {
  NvAPI_Status result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    NvAPI_UnloadCommand command(GetCurrentThreadId());

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = manager.getNvAPIDispatchTable().NvAPI_Unload();
    }
    command.result_.value = result;
    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = manager.getNvAPIDispatchTable().NvAPI_Unload();
  }

  return result;
}

NvAPI_Status NvAPI_D3D12_SetNvShaderExtnSlotSpaceWrapper(IUnknown* pDev,
                                                         NvU32 uavSlot,
                                                         NvU32 uavSpace) {
  NvAPI_Status result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {
    NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand command(GetCurrentThreadId(), pDev, uavSlot,
                                                        uavSpace);

    updateInterface(command.pDev_, pDev);
    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = manager.getNvAPIDispatchTable().NvAPI_D3D12_SetNvShaderExtnSlotSpace(pDev, uavSlot,
                                                                                    uavSpace);
    }
    command.result_.value = result;
    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = manager.getNvAPIDispatchTable().NvAPI_D3D12_SetNvShaderExtnSlotSpace(pDev, uavSlot,
                                                                                  uavSpace);
  }

  return result;
}

NvAPI_Status NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadWrapper(IUnknown* pDev,
                                                                    NvU32 uavSlot,
                                                                    NvU32 uavSpace) {
  NvAPI_Status result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {
    NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand command(GetCurrentThreadId(), pDev,
                                                                   uavSlot, uavSpace);

    updateInterface(command.pDev_, pDev);
    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = manager.getNvAPIDispatchTable().NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread(
          pDev, uavSlot, uavSpace);
    }
    command.result_.value = result;
    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = manager.getNvAPIDispatchTable().NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread(
        pDev, uavSlot, uavSpace);
  }

  return result;
}

NvAPI_Status NvAPI_D3D12_BuildRaytracingAccelerationStructureExWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    const NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS* pParams) {
  NvAPI_Status result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {
    GITS_ASSERT(pParams->version == NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS_VER);

    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand command(GetCurrentThreadId(),
                                                                      pCommandList, pParams);

    updateInterface(command.pCommandList_, pCommandList);
    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = manager.getNvAPIDispatchTable().NvAPI_D3D12_BuildRaytracingAccelerationStructureEx(
          pCommandList, pParams);
    }
    command.result_.value = result;
    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = manager.getNvAPIDispatchTable().NvAPI_D3D12_BuildRaytracingAccelerationStructureEx(
        pCommandList, pParams);
  }

  return result;
}

NvAPI_Status NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS* pParams) {
  NvAPI_Status result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {
    GITS_ASSERT(pParams->version == NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS_VER);

    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand command(GetCurrentThreadId(),
                                                                   pCommandList, pParams);

    updateInterface(command.pCommandList_, pCommandList);
    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result = manager.getNvAPIDispatchTable().NvAPI_D3D12_BuildRaytracingOpacityMicromapArray(
          pCommandList, pParams);
    }
    command.result_.value = result;
    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = manager.getNvAPIDispatchTable().NvAPI_D3D12_BuildRaytracingOpacityMicromapArray(
        pCommandList, pParams);
  }

  return result;
}

NvAPI_Status NvAPI_D3D12_RelocateRaytracingOpacityMicromapArrayWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    const NVAPI_RELOCATE_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS* pParams) {
  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "NvAPI_D3D12_RelocateRaytracingOpacityMicromapArray not handled!";
    logged = true;
  }
  auto& manager = CaptureManager::get();
  return manager.getNvAPIDispatchTable().NvAPI_D3D12_RelocateRaytracingOpacityMicromapArray(
      pCommandList, pParams);
}

NvAPI_Status NvAPI_D3D12_EmitRaytracingOpacityMicromapArrayPostbuildInfoWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    const NVAPI_EMIT_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_PARAMS* pParams) {
  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "NvAPI_D3D12_EmitRaytracingOpacityMicromapArrayPostbuildInfo not handled!";
    logged = true;
  }
  auto& manager = CaptureManager::get();
  return manager.getNvAPIDispatchTable()
      .NvAPI_D3D12_EmitRaytracingOpacityMicromapArrayPostbuildInfo(pCommandList, pParams);
}

NvAPI_Status NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    const NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS* pParams) {
  NvAPI_Status result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {
    GITS_ASSERT(pParams->version ==
                NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS_VER);

    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand command(
        GetCurrentThreadId(), pCommandList, pParams);

    updateInterface(command.pCommandList_, pCommandList);
    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    command.key = manager.createCommandKey();
    if (!command.skip) {
      result =
          manager.getNvAPIDispatchTable()
              .NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation(pCommandList, pParams);
    }
    command.result_.value = result;
    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result =
        manager.getNvAPIDispatchTable().NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation(
            pCommandList, pParams);
  }

  return result;
}

NvAPI_Status NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirectWrapper(
    ID3D12GraphicsCommandList4* pCommandList,
    const NVAPI_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PARAMS* pParams) {
  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirect not handled!";
    logged = true;
  }
  auto& manager = CaptureManager::get();
  return manager.getNvAPIDispatchTable().NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirect(
      pCommandList, pParams);
}

} // namespace DirectX
} // namespace gits
