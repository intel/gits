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
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(MappedDataMetaCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(CreateHeapAllocationMetaCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(WaitForFenceSignaledCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(IUnknownQueryInterfaceCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(IUnknownAddRefCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(IUnknownReleaseCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

void SkipCallsLayer::pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

%for function in functions:
void SkipCallsLayer::pre(${function.name}Command& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
void SkipCallsLayer::pre(${interface.name}${function.name}Command& command) {
  if (isToSkip(command.key)) {
    command.skip = true;
  }
}

%endfor
%endfor

bool SkipCallsLayer::isToSkip(unsigned key) {
  return callsToSkip_.count(key);
}

void SkipCallsLayer::extractKeys(const std::string& keyString,
                                 std::unordered_set<unsigned>& keySet) {
  const char* p = keyString.data();
  do {
    const char* begin = p;
    while (*p != ',' && *p) {
      ++p;
    }
    std::string key(begin, p);
    if (key[0] == 'S') {
      key = key.substr(1);
      unsigned k = std::stoi(key);
      k |= Command::stateRestoreKeyMask;
      keySet.insert(k);
    } else {
      keySet.insert(std::stoi(key));
    }
  } while (*p++);
}

} // namespace DirectX
} // namespace gits
