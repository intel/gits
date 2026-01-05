// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "skipCallsOnResultLayerAuto.h"

namespace gits {
namespace DirectX {

void SkipCallsOnResultLayer::pre(IUnknownQueryInterfaceCommand& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(INTC_D3D12_CreateHeapCommand& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

%for function in functions:
void SkipCallsOnResultLayer::pre(${function.name}Command& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
%if function.ret.type == 'HRESULT' and f'{interface.name}{function.name}' not in ['IDXGISwapChainPresent', 'IDXGISwapChain1Present1']:
void SkipCallsOnResultLayer::pre(${interface.name}${function.name}Command& command) {
  if (command.result_.value != S_OK) {
    command.skip = true;
  }
}

%endif
%endfor
%endfor

} // namespace DirectX
} // namespace gits
