// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "directXCommandFactory.h"
#include "commandRunnersAuto.h"
#include "commandRunnersCustom.h"
#include "commandIdsAuto.h"
#include "playerManager.h"

namespace gits {
namespace DirectX {

stream::CommandRunner* DirectXCommandFactory::CreateCommand(unsigned id) {
  switch (static_cast<CommandId>(id)) {
  case CommandId::ID_INIT_START:
    return new StateRestoreBeginRunner();
  case CommandId::ID_INIT_END:
    return new StateRestoreEndRunner();
  case CommandId::ID_FRAME_END:
    return new FrameEndRunner();
  case CommandId::ID_MARKER_UINT64:
    return new MarkerUInt64Runner();
  case CommandId::ID_META_CREATE_WINDOW:
    return new CreateWindowMetaRunner();
  case CommandId::ID_MAPPED_DATA:
    return new MappedDataMetaRunner();
  case CommandId::ID_CREATE_HEAP_ALLOCATION:
    return new CreateHeapAllocationMetaRunner();
  case CommandId::ID_WAIT_FOR_FENCE_SIGNALED:
    return new WaitForFenceSignaledRunner();
  case CommandId::ID_META_DLL_CONTAINER:
    return new DllContainerMetaRunner();
  case CommandId::ID_IUNKNOWN_QUERYINTERFACE:
    return new IUnknownQueryInterfaceRunner();
  case CommandId::ID_IUNKNOWN_ADDREF:
    return new IUnknownAddRefRunner();
  case CommandId::ID_IUNKNOWN_RELEASE:
    return new IUnknownReleaseRunner();
  case CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS:
    return new INTC_D3D12_GetSupportedVersionsRunner();
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT:
    return new INTC_D3D12_CreateDeviceExtensionContextRunner();
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1:
    return new INTC_D3D12_CreateDeviceExtensionContext1Runner();
  case CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT:
    return new INTC_DestroyDeviceExtensionContextRunner();
  case CommandId::INTC_D3D12_CHECKFEATURESUPPORT:
    return new INTC_D3D12_CheckFeatureSupportRunner();
  case CommandId::INTC_D3D12_SETFEATURESUPPORT:
    return new INTC_D3D12_SetFeatureSupportRunner();
  case CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO:
    return new INTC_D3D12_GetResourceAllocationInfoRunner();
  case CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE:
    return new INTC_D3D12_CreateComputePipelineStateRunner();
  case CommandId::INTC_D3D12_CREATEPLACEDRESOURCE:
    return new INTC_D3D12_CreatePlacedResourceRunner();
  case CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE:
    return new INTC_D3D12_CreateCommittedResourceRunner();
  case CommandId::INTC_D3D12_CREATEHEAP:
    return new INTC_D3D12_CreateHeapRunner();
  case CommandId::INTC_D3D12_SETAPPLICATIONINFO:
    return new INTC_D3D12_SetApplicationInfoRunner();
  case CommandId::ID_NVAPI_INITIALIZE:
    return new NvAPI_InitializeRunner();
  case CommandId::ID_NVAPI_UNLOAD:
    return new NvAPI_UnloadRunner();
  case CommandId::ID_NVAPI_D3D12_SETCREATEPIPELINESTATEOPTIONS:
    return new NvAPI_D3D12_SetCreatePipelineStateOptionsRunner();
  case CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACE:
    return new NvAPI_D3D12_SetNvShaderExtnSlotSpaceRunner();
  case CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD:
    return new NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadRunner();
  case CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX:
    return new NvAPI_D3D12_BuildRaytracingAccelerationStructureExRunner();
  case CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY:
    return new NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayRunner();
  case CommandId::ID_NVAPI_D3D12_RAYTRACINGEXECUTEMULTIINDIRECTCLUSTEROPERATION:
    return new NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationRunner();
  %for function in functions:
  case CommandId::ID_${function.name.upper()}:
    return new ${function.name}Runner();
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  case CommandId::ID_${interface.name.upper()}_${function.name.upper()}:
    return new ${interface.name}${function.name}Runner();
  %endfor
  %endfor
  }

  return nullptr;
}

} // namespace DirectX
} // namespace gits
