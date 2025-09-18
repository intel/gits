// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandWritersFactory.h"
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"

namespace gits {
namespace DirectX {

CommandWriter* createCommandWriter(Command* command) {
  switch (command->getId()) {
  case CommandId::ID_IUNKNOWN_QUERYINTERFACE:
    return new IUnknownQueryInterfaceWriter(*static_cast<IUnknownQueryInterfaceCommand*>(command));
  case CommandId::ID_IUNKNOWN_ADDREF:
    return new IUnknownAddRefWriter(*static_cast<IUnknownAddRefCommand*>(command));
  case CommandId::ID_IUNKNOWN_RELEASE:
    return new IUnknownReleaseWriter(*static_cast<IUnknownReleaseCommand*>(command));
  case CommandId::ID_META_CREATE_WINDOW:
    return new CreateWindowMetaWriter(*static_cast<CreateWindowMetaCommand*>(command));
  case CommandId::ID_MAPPED_DATA:
    return new MappedDataMetaWriter(*static_cast<MappedDataMetaCommand*>(command));
  case CommandId::ID_CREATE_HEAP_ALLOCATION:
    return new CreateHeapAllocationMetaWriter(*static_cast<CreateHeapAllocationMetaCommand*>(command));
  case CommandId::ID_WAIT_FOR_FENCE_SIGNALED:
    return new WaitForFenceSignaledWriter(*static_cast<WaitForFenceSignaledCommand*>(command));
  case CommandId::ID_META_DLL_CONTAINER:
    return new DllContainerMetaWriter(*static_cast<DllContainerMetaCommand*>(command));
  case CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS:
    return new INTC_D3D12_GetSupportedVersionsWriter(*static_cast<INTC_D3D12_GetSupportedVersionsCommand*>(command));
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT:
    return new INTC_D3D12_CreateDeviceExtensionContextWriter(*static_cast<INTC_D3D12_CreateDeviceExtensionContextCommand*>(command));
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1:
    return new INTC_D3D12_CreateDeviceExtensionContext1Writer(*static_cast<INTC_D3D12_CreateDeviceExtensionContext1Command*>(command));
  case CommandId::INTC_D3D12_SETAPPLICATIONINFO:
    return new INTC_D3D12_SetApplicationInfoWriter(*static_cast<INTC_D3D12_SetApplicationInfoCommand*>(command));
  case CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT:
    return new INTC_DestroyDeviceExtensionContextWriter(*static_cast<INTC_DestroyDeviceExtensionContextCommand*>(command));
  case CommandId::INTC_D3D12_CHECKFEATURESUPPORT:
    return new INTC_D3D12_CheckFeatureSupportWriter(*static_cast<INTC_D3D12_CheckFeatureSupportCommand*>(command));
  case CommandId::INTC_D3D12_CREATECOMMANDQUEUE:
    return new INTC_D3D12_CreateCommandQueueWriter(*static_cast<INTC_D3D12_CreateCommandQueueCommand*>(command));
  case CommandId::INTC_D3D12_CREATERESERVEDRESOURCE:
    return new INTC_D3D12_CreateReservedResourceWriter(*static_cast<INTC_D3D12_CreateReservedResourceCommand*>(command));
  case CommandId::INTC_D3D12_SETFEATURESUPPORT:
    return new INTC_D3D12_SetFeatureSupportWriter(*static_cast<INTC_D3D12_SetFeatureSupportCommand*>(command));
  case CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO:
    return new INTC_D3D12_GetResourceAllocationInfoWriter(*static_cast<INTC_D3D12_GetResourceAllocationInfoCommand*>(command));
  case CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE:
    return new INTC_D3D12_CreateComputePipelineStateWriter(*static_cast<INTC_D3D12_CreateComputePipelineStateCommand*>(command));
  case CommandId::INTC_D3D12_CREATEPLACEDRESOURCE:
    return new INTC_D3D12_CreatePlacedResourceWriter(*static_cast<INTC_D3D12_CreatePlacedResourceCommand*>(command));
  case CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE:
    return new INTC_D3D12_CreateCommittedResourceWriter(*static_cast<INTC_D3D12_CreateCommittedResourceCommand*>(command));
  case CommandId::INTC_D3D12_CREATEHEAP:
    return new INTC_D3D12_CreateHeapWriter(*static_cast<INTC_D3D12_CreateHeapCommand*>(command));
  case CommandId::ID_NVAPI_INITIALIZE:
    return new NvAPI_InitializeWriter(*static_cast<NvAPI_InitializeCommand*>(command));
  case CommandId::ID_NVAPI_UNLOAD:
    return new NvAPI_UnloadWriter(*static_cast<NvAPI_UnloadCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_SETCREATEPIPELINESTATEOPTIONS:
    return new NvAPI_D3D12_SetCreatePipelineStateOptionsWriter(*static_cast<NvAPI_D3D12_SetCreatePipelineStateOptionsCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACE:
    return new NvAPI_D3D12_SetNvShaderExtnSlotSpaceWriter(*static_cast<NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD:
    return new NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadWriter(*static_cast<NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX:
    return new NvAPI_D3D12_BuildRaytracingAccelerationStructureExWriter(*static_cast<NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY:
    return new NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayWriter(*static_cast<NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_RAYTRACINGEXECUTEMULTIINDIRECTCLUSTEROPERATION:
    return new NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationWriter(*static_cast<NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand*>(command));
  %for function in functions:
  case CommandId::ID_${function.name.upper()}:
    return new ${function.name}Writer(*static_cast<${function.name}Command*>(command));
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  case CommandId::ID_${interface.name.upper()}_${function.name.upper()}:
    return new ${interface.name}${function.name}Writer(*static_cast<${interface.name}${function.name}Command*>(command));
  %endfor
  %endfor
  }
  return nullptr;
}

} // namespace DirectX
} // namespace gits
