// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

void SkipCallsOnConfigLayer::Pre(CreateWindowMetaCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " CreateWindowMeta";
  }
}

void SkipCallsOnConfigLayer::Pre(MappedDataMetaCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " MappedDataMeta";
  }
}

void SkipCallsOnConfigLayer::Pre(CreateHeapAllocationMetaCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " CreateHeapAllocationMeta";
  }
}

void SkipCallsOnConfigLayer::Pre(WaitForFenceSignaledDeprecatedCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " WaitForFenceSignaled";
  }
}

void SkipCallsOnConfigLayer::Pre(WaitForFenceSignaledCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " WaitForFenceSignaled";
  }
}

void SkipCallsOnConfigLayer::Pre(DllContainerMetaCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " DllContainerMeta";
  }
}

void SkipCallsOnConfigLayer::Pre(IUnknownQueryInterfaceCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " IUnknown::QueryInterface";
  }
}

void SkipCallsOnConfigLayer::Pre(IUnknownAddRefCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " IUnknown::AddRef";
  }
}

void SkipCallsOnConfigLayer::Pre(IUnknownReleaseCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " IUnknown::Release";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_D3D12_CreateDeviceExtensionContext";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_D3D12_CreateDeviceExtensionContext1";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_D3D12_SetApplicationInfo";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_DestroyDeviceExtensionContext";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_D3D12_CheckFeatureSupport";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_D3D12_SetFeatureSupport";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_D3D12_GetResourceAllocationInfo";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_D3D12_CreateComputePipelineState";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_D3D12_CreatePlacedResource";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_D3D12_CreateCommittedResource";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_D3D12_CreateReservedResourceCommand";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_D3D12_CreateCommandQueueCommand";
  }
}

void SkipCallsOnConfigLayer::Pre(INTC_D3D12_CreateHeapCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " INTC_D3D12_CreateHeapCommand";
  }
}

void SkipCallsOnConfigLayer::Pre(NvAPI_InitializeCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " NvAPI_InitializeCommand";
  }
}

void SkipCallsOnConfigLayer::Pre(NvAPI_UnloadCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " NvAPI_UnloadCommand";
  }
}

void SkipCallsOnConfigLayer::Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " NvAPI_D3D12_SetCreatePipelineStateOptionsCommand";
  }
}

void SkipCallsOnConfigLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand";
  }
}

void SkipCallsOnConfigLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand";
  }
}

void SkipCallsOnConfigLayer::Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand";
  }
}

void SkipCallsOnConfigLayer::Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand";
  }
}

void SkipCallsOnConfigLayer::Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand";
  }
}

%for function in functions:
void SkipCallsOnConfigLayer::Pre(${function.name}Command& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " ${function.name}";
  }
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
void SkipCallsOnConfigLayer::Pre(${interface.name}${function.name}Command& command) {
  if (m_KeyRange[command.Key]) {
    command.Skip = true;
    LOG_INFO << "[SKIPPED] call " << keyToStr(command.Key) << " ${interface.name}::${function.name}";
  }
}

%endfor
%endfor

} // namespace DirectX
} // namespace gits
