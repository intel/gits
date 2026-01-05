// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "encoderLayerAuto.h"
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"
#include "token.h"
#include "streams.h"

namespace gits {
namespace DirectX {

void EncoderLayer::post(IUnknownQueryInterfaceCommand& command) {
  recorder_.record(command.key, new IUnknownQueryInterfaceWriter(command));
}

void EncoderLayer::post(IUnknownAddRefCommand& command) {
  recorder_.record(command.key, new IUnknownAddRefWriter(command));
}

void EncoderLayer::post(IUnknownReleaseCommand& command) {
  recorder_.record(command.key, new IUnknownReleaseWriter(command));
}

void EncoderLayer::post(INTC_D3D12_GetSupportedVersionsCommand& command) {
  recorder_.record(command.key, new INTC_D3D12_GetSupportedVersionsWriter(command));
}

void EncoderLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  recorder_.record(command.key, new INTC_D3D12_CreateDeviceExtensionContextWriter(command));
}

void EncoderLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  recorder_.record(command.key, new INTC_D3D12_CreateDeviceExtensionContext1Writer(command));
}

void EncoderLayer::post(INTC_D3D12_SetApplicationInfoCommand& command) {
  recorder_.record(command.key, new INTC_D3D12_SetApplicationInfoWriter(command));
}

void EncoderLayer::post(INTC_DestroyDeviceExtensionContextCommand& command) {
  recorder_.record(command.key, new INTC_DestroyDeviceExtensionContextWriter(command));
}

void EncoderLayer::post(INTC_D3D12_CheckFeatureSupportCommand& command) {
  recorder_.record(command.key, new INTC_D3D12_CheckFeatureSupportWriter(command));
}

void EncoderLayer::post(INTC_D3D12_CreateCommandQueueCommand& command) {
  recorder_.record(command.key, new INTC_D3D12_CreateCommandQueueWriter(command));
}

void EncoderLayer::post(INTC_D3D12_CreateReservedResourceCommand& command) {
  recorder_.record(command.key, new INTC_D3D12_CreateReservedResourceWriter(command));
}

void EncoderLayer::post(INTC_D3D12_SetFeatureSupportCommand& command) {
  recorder_.record(command.key, new INTC_D3D12_SetFeatureSupportWriter(command));
}

void EncoderLayer::post(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  recorder_.record(command.key, new INTC_D3D12_GetResourceAllocationInfoWriter(command));
}

void EncoderLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  recorder_.record(command.key, new INTC_D3D12_CreateComputePipelineStateWriter(command));
}

void EncoderLayer::post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  recorder_.record(command.key, new INTC_D3D12_CreatePlacedResourceWriter(command));
}

void EncoderLayer::post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  recorder_.record(command.key, new INTC_D3D12_CreateCommittedResourceWriter(command));
}

void EncoderLayer::post(INTC_D3D12_CreateHeapCommand& command) {
  recorder_.record(command.key, new INTC_D3D12_CreateHeapWriter(command));
}

void EncoderLayer::post(NvAPI_InitializeCommand& command) {
  recorder_.record(command.key, new NvAPI_InitializeWriter(command));
}

void EncoderLayer::post(NvAPI_UnloadCommand& command) {
  recorder_.record(command.key, new NvAPI_UnloadWriter(command));
}

void EncoderLayer::post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  recorder_.record(command.key, new NvAPI_D3D12_SetCreatePipelineStateOptionsWriter(command));
}

void EncoderLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  recorder_.record(command.key, new NvAPI_D3D12_SetNvShaderExtnSlotSpaceWriter(command));
}

void EncoderLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  recorder_.record(command.key, new NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadWriter(command));
}

void EncoderLayer::post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  recorder_.record(command.key, new NvAPI_D3D12_BuildRaytracingAccelerationStructureExWriter(command));
}

void EncoderLayer::post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  recorder_.record(command.key, new NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayWriter(command));
}

void EncoderLayer::post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  recorder_.record(command.key, new NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationWriter(command));
}

<%
custom = [
    'IDXGISwapChainPresent',
    'IDXGISwapChain1Present1'
]
%>\
%for function in functions:
%if not function.name in custom:
void EncoderLayer::post(${function.name}Command& command) {
  recorder_.record(command.key, new ${function.name}Writer(command));
}

%endif
%endfor
%for interface in interfaces:
%for function in interface.functions:
%if not interface.name + function.name in custom:
void EncoderLayer::post(${interface.name}${function.name}Command& command) {
  recorder_.record(command.key, new ${interface.name}${function.name}Writer(command));
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
