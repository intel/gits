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

std::unique_ptr<stream::CommandSerializer> createCommandSerializer(const Command* command) {
  switch (command->GetId()) {
  case CommandId::ID_INIT_START:
    return std::make_unique<StateRestoreBeginSerializer>(*static_cast<const StateRestoreBeginCommand*>(command));
  case CommandId::ID_INIT_END:
    return std::make_unique<StateRestoreEndSerializer>(*static_cast<const StateRestoreEndCommand*>(command));
  case CommandId::ID_FRAME_END:
    return std::make_unique<FrameEndSerializer>(*static_cast<const FrameEndCommand*>(command));
  case CommandId::ID_MARKER_UINT64:
    return std::make_unique<MarkerUInt64Serializer>(*static_cast<const MarkerUInt64Command*>(command));
  case CommandId::ID_IUNKNOWN_QUERYINTERFACE:
    return std::make_unique<IUnknownQueryInterfaceSerializer>(*static_cast<const IUnknownQueryInterfaceCommand*>(command));
  case CommandId::ID_IUNKNOWN_ADDREF:
    return std::make_unique<IUnknownAddRefSerializer>(*static_cast<const IUnknownAddRefCommand*>(command));
  case CommandId::ID_IUNKNOWN_RELEASE:
    return std::make_unique<IUnknownReleaseSerializer>(*static_cast<const IUnknownReleaseCommand*>(command));
  case CommandId::ID_META_CREATE_WINDOW:
    return std::make_unique<CreateWindowMetaSerializer>(*static_cast<const CreateWindowMetaCommand*>(command));
  case CommandId::ID_MAPPED_DATA:
    return std::make_unique<MappedDataMetaSerializer>(*static_cast<const MappedDataMetaCommand*>(command));
  case CommandId::ID_CREATE_HEAP_ALLOCATION:
    return std::make_unique<CreateHeapAllocationMetaSerializer>(*static_cast<const CreateHeapAllocationMetaCommand*>(command));
  case CommandId::ID_WAIT_FOR_FENCE_SIGNALED:
    return std::make_unique<WaitForFenceSignaledSerializer>(*static_cast<const WaitForFenceSignaledCommand*>(command));
  case CommandId::ID_META_DLL_CONTAINER:
    return std::make_unique<DllContainerMetaSerializer>(*static_cast<const DllContainerMetaCommand*>(command));
  case CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS:
    return std::make_unique<INTC_D3D12_GetSupportedVersionsSerializer>(*static_cast<const INTC_D3D12_GetSupportedVersionsCommand*>(command));
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT:
    return std::make_unique<INTC_D3D12_CreateDeviceExtensionContextSerializer>(*static_cast<const INTC_D3D12_CreateDeviceExtensionContextCommand*>(command));
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1:
    return std::make_unique<INTC_D3D12_CreateDeviceExtensionContext1Serializer>(*static_cast<const INTC_D3D12_CreateDeviceExtensionContext1Command*>(command));
  case CommandId::INTC_D3D12_SETAPPLICATIONINFO:
    return std::make_unique<INTC_D3D12_SetApplicationInfoSerializer>(*static_cast<const INTC_D3D12_SetApplicationInfoCommand*>(command));
  case CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT:
    return std::make_unique<INTC_DestroyDeviceExtensionContextSerializer>(*static_cast<const INTC_DestroyDeviceExtensionContextCommand*>(command));
  case CommandId::INTC_D3D12_CHECKFEATURESUPPORT:
    return std::make_unique<INTC_D3D12_CheckFeatureSupportSerializer>(*static_cast<const INTC_D3D12_CheckFeatureSupportCommand*>(command));
  case CommandId::INTC_D3D12_CREATECOMMANDQUEUE:
    return std::make_unique<INTC_D3D12_CreateCommandQueueSerializer>(*static_cast<const INTC_D3D12_CreateCommandQueueCommand*>(command));
  case CommandId::INTC_D3D12_CREATERESERVEDRESOURCE:
    return std::make_unique<INTC_D3D12_CreateReservedResourceSerializer>(*static_cast<const INTC_D3D12_CreateReservedResourceCommand*>(command));
  case CommandId::INTC_D3D12_SETFEATURESUPPORT:
    return std::make_unique<INTC_D3D12_SetFeatureSupportSerializer>(*static_cast<const INTC_D3D12_SetFeatureSupportCommand*>(command));
  case CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO:
    return std::make_unique<INTC_D3D12_GetResourceAllocationInfoSerializer>(*static_cast<const INTC_D3D12_GetResourceAllocationInfoCommand*>(command));
  case CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE:
    return std::make_unique<INTC_D3D12_CreateComputePipelineStateSerializer>(*static_cast<const INTC_D3D12_CreateComputePipelineStateCommand*>(command));
  case CommandId::INTC_D3D12_CREATEPLACEDRESOURCE:
    return std::make_unique<INTC_D3D12_CreatePlacedResourceSerializer>(*static_cast<const INTC_D3D12_CreatePlacedResourceCommand*>(command));
  case CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE:
    return std::make_unique<INTC_D3D12_CreateCommittedResourceSerializer>(*static_cast<const INTC_D3D12_CreateCommittedResourceCommand*>(command));
  case CommandId::INTC_D3D12_CREATEHEAP:
    return std::make_unique<INTC_D3D12_CreateHeapSerializer>(*static_cast<const INTC_D3D12_CreateHeapCommand*>(command));
  case CommandId::ID_NVAPI_INITIALIZE:
    return std::make_unique<NvAPI_InitializeSerializer>(*static_cast<const NvAPI_InitializeCommand*>(command));
  case CommandId::ID_NVAPI_UNLOAD:
    return std::make_unique<NvAPI_UnloadSerializer>(*static_cast<const NvAPI_UnloadCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_SETCREATEPIPELINESTATEOPTIONS:
    return std::make_unique<NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer>(*static_cast<const NvAPI_D3D12_SetCreatePipelineStateOptionsCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACE:
    return std::make_unique<NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer>(*static_cast<const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD:
    return std::make_unique<NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer>(*static_cast<const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX:
    return std::make_unique<NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer>(*static_cast<const NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY:
    return std::make_unique<NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer>(*static_cast<const NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand*>(command));
  case CommandId::ID_NVAPI_D3D12_RAYTRACINGEXECUTEMULTIINDIRECTCLUSTEROPERATION:
    return std::make_unique<NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer>(*static_cast<const NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand*>(command));
  %for function in functions:
  case CommandId::ID_${function.name.upper()}:
    return std::make_unique<${function.name}Serializer>(*static_cast<const ${function.name}Command*>(command));
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  case CommandId::ID_${interface.name.upper()}_${function.name.upper()}:
    return std::make_unique<${interface.name}${function.name}Serializer>(*static_cast<const ${interface.name}${function.name}Command*>(command));
  %endfor
  %endfor
  }
  return nullptr;
}

} // namespace DirectX
} // namespace gits
