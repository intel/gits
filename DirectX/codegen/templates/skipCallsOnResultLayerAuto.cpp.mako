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

void SkipCallsOnResultLayer::Pre(IUnknownQueryInterfaceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(INTC_D3D12_CreateHeapCommand& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

%for function in functions:
void SkipCallsOnResultLayer::Pre(${function.name}Command& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
%if function.ret.type == 'HRESULT' and f'{interface.name}{function.name}' not in ['IDXGISwapChainPresent', 'IDXGISwapChain1Present1']:
void SkipCallsOnResultLayer::Pre(${interface.name}${function.name}Command& command) {
  if (command.m_Result.Value != S_OK) {
    command.Skip = true;
  }
}

%endif
%endfor
%endfor

} // namespace DirectX
} // namespace gits
