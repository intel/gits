// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "SkipCallsLayerAuto.h"

namespace gits {
namespace DirectX {

void SkipCallsLayer::pre(CreateWindowMetaCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " CreateWindowMeta" <<  std::endl;
  }
}

void SkipCallsLayer::pre(MappedDataMetaCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " MappedDataMeta" <<  std::endl;
  }
}

void SkipCallsLayer::pre(CreateHeapAllocationMetaCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " CreateHeapAllocationMeta" <<  std::endl;
  }
}

void SkipCallsLayer::pre(WaitForFenceSignaledCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " WaitForFenceSignaled" <<  std::endl;
  }
}

void SkipCallsLayer::pre(IUnknownQueryInterfaceCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " IUnknownQueryInterface" <<  std::endl;
  }
}

void SkipCallsLayer::pre(IUnknownAddRefCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " IUnknownAddRef" <<  std::endl;
  }
}

void SkipCallsLayer::pre(IUnknownReleaseCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " IUnknownRelease" <<  std::endl;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " INTC_D3D12_CreateDeviceExtensionContext" <<  std::endl;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " INTC_D3D12_CreateDeviceExtensionContext1" <<  std::endl;
  }
}

void SkipCallsLayer::pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " INTC_DestroyDeviceExtensionContext" <<  std::endl;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " INTC_D3D12_CheckFeatureSupport" <<  std::endl;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " INTC_D3D12_SetFeatureSupport" <<  std::endl;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " INTC_D3D12_GetResourceAllocationInfo" <<  std::endl;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " INTC_D3D12_CreateComputePipelineState" <<  std::endl;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " INTC_D3D12_CreatePlacedResource" <<  std::endl;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " INTC_D3D12_CreateCommittedResource" <<  std::endl;
  }
}

%for function in functions:
void SkipCallsLayer::pre(${function.name}Command& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " ${function.name}" <<  std::endl;
  }
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
void SkipCallsLayer::pre(${interface.name}${function.name}Command& command) {
  if (const auto skipKey = commandKeysService_.atCommand(command.key)) {
    command.skip = true;
    Log(INFO) << "[SKIPPED] call " << commandKeysService_.keyToString(skipKey) <<   " ${interface.name}::${function.name}" <<  std::endl;
  }
}

%endfor
%endfor

} // namespace DirectX
} // namespace gits
