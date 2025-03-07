// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "skipCallsLayerAuto.h"

namespace gits {
namespace DirectX {

void SkipCallsLayer::pre(CreateWindowMetaCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " CreateWindowMeta";
  }
}

void SkipCallsLayer::pre(MappedDataMetaCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " MappedDataMeta";
  }
}

void SkipCallsLayer::pre(CreateHeapAllocationMetaCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " CreateHeapAllocationMeta";
  }
}

void SkipCallsLayer::pre(WaitForFenceSignaledCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " WaitForFenceSignaled";
  }
}

void SkipCallsLayer::pre(IUnknownQueryInterfaceCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " IUnknown::QueryInterface";
  }
}

void SkipCallsLayer::pre(IUnknownAddRefCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " IUnknown::AddRef";
  }
}

void SkipCallsLayer::pre(IUnknownReleaseCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " IUnknown::Release";
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " INTC_D3D12_CreateDeviceExtensionContext";
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " INTC_D3D12_CreateDeviceExtensionContext1";
  }
}

void SkipCallsLayer::pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " INTC_DestroyDeviceExtensionContext";
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " INTC_D3D12_CheckFeatureSupport";
  }
}

void SkipCallsLayer::pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " INTC_D3D12_SetFeatureSupport";
  }
}

void SkipCallsLayer::pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " INTC_D3D12_GetResourceAllocationInfo";
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " INTC_D3D12_CreateComputePipelineState";
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " INTC_D3D12_CreatePlacedResource";
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " INTC_D3D12_CreateCommittedResource";
  }
}

%for function in functions:
void SkipCallsLayer::pre(${function.name}Command& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " ${function.name}";
  }
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
void SkipCallsLayer::pre(${interface.name}${function.name}Command& command) {
  if (configKeySet_.contains(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << ConfigKeySet::keyToString(command.key) << " ${interface.name}::${function.name}";
  }
}

%endfor
%endfor

} // namespace DirectX
} // namespace gits
