// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandSerializersFactory.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"

namespace gits {
namespace DirectX {

stream::CommandSerializer* createCommandSerializer(Command* command) {
  switch (command->getId()) {
  case CommandId::ID_INIT_START:
    return new StateRestoreBeginSerializer(*static_cast<StateRestoreBeginCommand*>(command));
  case CommandId::ID_INIT_END:
    return new StateRestoreEndSerializer(*static_cast<StateRestoreEndCommand*>(command));
  case CommandId::ID_FRAME_END:
    return new FrameEndSerializer(*static_cast<FrameEndCommand*>(command));
  case CommandId::ID_MARKER_UINT64:
    return new MarkerUInt64Serializer(*static_cast<MarkerUInt64Command*>(command));
  case CommandId::ID_IUNKNOWN_QUERYINTERFACE:
    return new IUnknownQueryInterfaceSerializer(*static_cast<IUnknownQueryInterfaceCommand*>(command));
  case CommandId::ID_IUNKNOWN_ADDREF:
    return new IUnknownAddRefSerializer(*static_cast<IUnknownAddRefCommand*>(command));
  case CommandId::ID_IUNKNOWN_RELEASE:
    return new IUnknownReleaseSerializer(*static_cast<IUnknownReleaseCommand*>(command));
  case CommandId::ID_META_CREATE_WINDOW:
    return new CreateWindowMetaSerializer(*static_cast<CreateWindowMetaCommand*>(command));
  case CommandId::ID_MAPPED_DATA:
    return new MappedDataMetaSerializer(*static_cast<MappedDataMetaCommand*>(command));
  case CommandId::ID_CREATE_HEAP_ALLOCATION:
    return new CreateHeapAllocationMetaSerializer(*static_cast<CreateHeapAllocationMetaCommand*>(command));
  case CommandId::ID_WAIT_FOR_FENCE_SIGNALED:
    return new WaitForFenceSignaledSerializer(*static_cast<WaitForFenceSignaledCommand*>(command));
  case CommandId::ID_META_DLL_CONTAINER:
    return new DllContainerMetaSerializer(*static_cast<DllContainerMetaCommand*>(command));
  case CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS:
    return new INTC_D3D12_GetSupportedVersionsSerializer(*static_cast<INTC_D3D12_GetSupportedVersionsCommand*>(command));
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT:
    return new INTC_D3D12_CreateDeviceExtensionContextSerializer(*static_cast<INTC_D3D12_CreateDeviceExtensionContextCommand*>(command));
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1:
    return new INTC_D3D12_CreateDeviceExtensionContext1Serializer(*static_cast<INTC_D3D12_CreateDeviceExtensionContext1Command*>(command));
  case CommandId::INTC_D3D12_SETAPPLICATIONINFO:
    return new INTC_D3D12_SetApplicationInfoSerializer(*static_cast<INTC_D3D12_SetApplicationInfoCommand*>(command));
  case CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT:
    return new INTC_DestroyDeviceExtensionContextSerializer(*static_cast<INTC_DestroyDeviceExtensionContextCommand*>(command));
  case CommandId::INTC_D3D12_CHECKFEATURESUPPORT:
    return new INTC_D3D12_CheckFeatureSupportSerializer(*static_cast<INTC_D3D12_CheckFeatureSupportCommand*>(command));
  case CommandId::INTC_D3D12_CREATECOMMANDQUEUE:
    return new INTC_D3D12_CreateCommandQueueSerializer(*static_cast<INTC_D3D12_CreateCommandQueueCommand*>(command));
  case CommandId::INTC_D3D12_CREATERESERVEDRESOURCE:
    return new INTC_D3D12_CreateReservedResourceSerializer(*static_cast<INTC_D3D12_CreateReservedResourceCommand*>(command));
  case CommandId::INTC_D3D12_SETFEATURESUPPORT:
    return new INTC_D3D12_SetFeatureSupportSerializer(*static_cast<INTC_D3D12_SetFeatureSupportCommand*>(command));
  case CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO:
    return new INTC_D3D12_GetResourceAllocationInfoSerializer(*static_cast<INTC_D3D12_GetResourceAllocationInfoCommand*>(command));
  case CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE:
    return new INTC_D3D12_CreateComputePipelineStateSerializer(*static_cast<INTC_D3D12_CreateComputePipelineStateCommand*>(command));
  case CommandId::INTC_D3D12_CREATEPLACEDRESOURCE:
    return new INTC_D3D12_CreatePlacedResourceSerializer(*static_cast<INTC_D3D12_CreatePlacedResourceCommand*>(command));
  case CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE:
    return new INTC_D3D12_CreateCommittedResourceSerializer(*static_cast<INTC_D3D12_CreateCommittedResourceCommand*>(command));
  case CommandId::INTC_D3D12_CREATEHEAP:
    return new INTC_D3D12_CreateHeapSerializer(*static_cast<INTC_D3D12_CreateHeapCommand*>(command));
  case CommandId::ID_NVAPI_INITIALIZE:
    return new NvAPI_InitializeSerializer(*static_cast<NvAPI_InitializeCommand*>(command));
  case CommandId::ID_NVAPI_UNLOAD:
    return new NvAPI_UnloadSerializer(*static_cast<NvAPI_UnloadCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_SETCREATEPIPELINESTATEOPTIONS:
    return new NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer(*static_cast<NvAPI_D3D12_SetCreatePipelineStateOptionsCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACE:
    return new NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer(*static_cast<NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD:
    return new NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer(*static_cast<NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX:
    return new NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer(*static_cast<NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY:
    return new NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer(*static_cast<NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_RAYTRACINGEXECUTEMULTIINDIRECTCLUSTEROPERATION:
    return new NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer(*static_cast<NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand*>(command));
  %for function in functions:
  case CommandId::ID_${function.name.upper()}:
    return new ${function.name}Serializer(*static_cast<${function.name}Command*>(command));
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  case CommandId::ID_${interface.name.upper()}_${function.name.upper()}:
    return new ${interface.name}${function.name}Serializer(*static_cast<${interface.name}${function.name}Command*>(command));
  %endfor
  %endfor
  }
  return nullptr;
}

} // namespace DirectX
} // namespace gits
