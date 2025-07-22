// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "directXLibrary.h"
#include "commandPlayersAuto.h"
#include "commandPlayersCustom.h"
#include "commandIdsAuto.h"

#include "gits.h"

namespace gits {
namespace DirectX {

DirectXLibrary& DirectXLibrary::Get() {
  return static_cast<DirectXLibrary&>(CGits::Instance().Library(ID_DirectX));
}

gits::CFunction* DirectXLibrary::FunctionCreate(unsigned type) const {

  switch (static_cast<CommandId>(type)) {
  case CommandId::ID_META_CREATE_WINDOW:
    return new CreateWindowMetaPlayer();
  case CommandId::ID_MAPPED_DATA:
    return new MappedDataMetaPlayer();
  case CommandId::ID_CREATE_HEAP_ALLOCATION:
    return new CreateHeapAllocationMetaPlayer();
  case CommandId::ID_WAIT_FOR_FENCE_SIGNALED:
    return new WaitForFenceSignaledPlayer();
  case CommandId::ID_IUNKNOWN_QUERYINTERFACE:
    return new IUnknownQueryInterfacePlayer();
  case CommandId::ID_IUNKNOWN_ADDREF:
    return new IUnknownAddRefPlayer();
  case CommandId::ID_IUNKNOWN_RELEASE:
    return new IUnknownReleasePlayer();
  case CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS:
    return new INTC_D3D12_GetSupportedVersionsPlayer();
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT:
    return new INTC_D3D12_CreateDeviceExtensionContextPlayer();
  case CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1:
    return new INTC_D3D12_CreateDeviceExtensionContext1Player();
  case CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT:
    return new INTC_DestroyDeviceExtensionContextPlayer();
  case CommandId::INTC_D3D12_CHECKFEATURESUPPORT:
    return new INTC_D3D12_CheckFeatureSupportPlayer();
  case CommandId::INTC_D3D12_SETFEATURESUPPORT:
    return new INTC_D3D12_SetFeatureSupportPlayer();
  case CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO:
    return new INTC_D3D12_GetResourceAllocationInfoPlayer();
  case CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE:
    return new INTC_D3D12_CreateComputePipelineStatePlayer();
  case CommandId::INTC_D3D12_CREATEPLACEDRESOURCE:
    return new INTC_D3D12_CreatePlacedResourcePlayer();
  case CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE:
    return new INTC_D3D12_CreateCommittedResourcePlayer();
  case CommandId::INTC_D3D12_CREATEHEAP:
    return new INTC_D3D12_CreateHeapPlayer();
  case CommandId::INTC_D3D12_SETAPPLICATIONINFO:
    return new INTC_D3D12_SetApplicationInfoPlayer();
  case CommandId::ID_NVAPI_INITIALIZE:
    return new NvAPI_InitializePlayer();
  case CommandId::ID_NVAPI_UNLOAD:
    return new NvAPI_UnloadPlayer();
  case CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD:
    return new NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadPlayer();
  case CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX:
    return new NvAPI_D3D12_BuildRaytracingAccelerationStructureExPlayer();
  case CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY:
    return new NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayPlayer();
  %for function in functions:
  case CommandId::ID_${function.name.upper()}:
    return new ${function.name}Player();
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  case CommandId::ID_${interface.name.upper()}_${function.name.upper()}:
    return new ${interface.name}${function.name}Player();
  %endfor
  %endfor
  }

  return nullptr;
}

} // namespace DirectX
} // namespace gits
