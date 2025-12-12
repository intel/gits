// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "skipCallsOnConfigLayerAuto.h"
#include "to_string/toStr.h"
#include "log.h"

namespace gits {
namespace DirectX {

void SkipCallsOnConfigLayer::pre(CreateWindowMetaCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " CreateWindowMeta";
  }
}

void SkipCallsOnConfigLayer::pre(MappedDataMetaCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " MappedDataMeta";
  }
}

void SkipCallsOnConfigLayer::pre(CreateHeapAllocationMetaCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " CreateHeapAllocationMeta";
  }
}

void SkipCallsOnConfigLayer::pre(WaitForFenceSignaledCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " WaitForFenceSignaled";
  }
}

void SkipCallsOnConfigLayer::pre(DllContainerMetaCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " DllContainerMeta";
  }
}

void SkipCallsOnConfigLayer::pre(IUnknownQueryInterfaceCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " IUnknown::QueryInterface";
  }
}

void SkipCallsOnConfigLayer::pre(IUnknownAddRefCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " IUnknown::AddRef";
  }
}

void SkipCallsOnConfigLayer::pre(IUnknownReleaseCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " IUnknown::Release";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_D3D12_CreateDeviceExtensionContext";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_D3D12_CreateDeviceExtensionContext1";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_D3D12_SetApplicationInfo";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_DestroyDeviceExtensionContext";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_D3D12_CheckFeatureSupport";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_D3D12_SetFeatureSupport";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_D3D12_GetResourceAllocationInfo";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_D3D12_CreateComputePipelineState";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_D3D12_CreatePlacedResource";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_D3D12_CreateCommittedResource";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_D3D12_CreateReservedResourceCommand";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_D3D12_CreateCommandQueueCommand";
  }
}

void SkipCallsOnConfigLayer::pre(INTC_D3D12_CreateHeapCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " INTC_D3D12_CreateHeapCommand";
  }
}

void SkipCallsOnConfigLayer::pre(NvAPI_InitializeCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " NvAPI_InitializeCommand";
  }
}

void SkipCallsOnConfigLayer::pre(NvAPI_UnloadCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " NvAPI_UnloadCommand";
  }
}

void SkipCallsOnConfigLayer::pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " NvAPI_D3D12_SetCreatePipelineStateOptionsCommand";
  }
}

void SkipCallsOnConfigLayer::pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand";
  }
}

void SkipCallsOnConfigLayer::pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand";
  }
}

void SkipCallsOnConfigLayer::pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand";
  }
}

void SkipCallsOnConfigLayer::pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand";
  }
}

void SkipCallsOnConfigLayer::pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand";
  }
}

%for function in functions:
void SkipCallsOnConfigLayer::pre(${function.name}Command& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " ${function.name}";
  }
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
void SkipCallsOnConfigLayer::pre(${interface.name}${function.name}Command& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.key) << " ${interface.name}::${function.name}";
  }
}

%endfor
%endfor

} // namespace DirectX
} // namespace gits
