// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "logDxErrorLayerAuto.h"
#include "toStr.h"
#include "log.h"

namespace gits {
namespace DirectX {

void LogDxErrorLayer::post(ID3D12FenceGetCompletedValueCommand& command) {
  if (command.result_.value == UINT64_MAX) {
    Log(ERR) << callKeyToStr(command.key) << " ID3D12Fence::GetCompletedValue failed - device removed";
  }
}

void LogDxErrorLayer::pre(IUnknownQueryInterfaceCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(IUnknownQueryInterfaceCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " IUnknown::QueryInterface failed " << printResult(command.result_.value);
  }
}

%for function in functions:
%if function.ret.type == 'HRESULT':
void LogDxErrorLayer::pre(${function.name}Command& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(${function.name}Command& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " ${function.name} failed " << printResult(command.result_.value);
  }
}

%elif function.ret.type == 'xess_result_t':
void LogDxErrorLayer::pre(${function.name}Command& command) {
  preResultXess_ = command.result_.value;
}

void LogDxErrorLayer::post(${function.name}Command& command) {
  if (isFailureXess(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " ${function.name} failed " << toStr(command.result_.value);
  }
}

%endif
%endfor
%for interface in interfaces:
%for function in interface.functions:
%if function.ret.type == 'HRESULT':
void LogDxErrorLayer::pre(${interface.name}${function.name}Command& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(${interface.name}${function.name}Command& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " ${interface.name}::${function.name} failed " << printResult(command.result_.value);
  }
}

%endif
%endfor
%endfor
void LogDxErrorLayer::pre(INTC_D3D12_GetSupportedVersionsCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_D3D12_GetSupportedVersionsCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " INTC_D3D12_GetSupportedVersions failed " << printResult(command.result_.value);
  }
}

void LogDxErrorLayer::pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " INTC_D3D12_CreateDeviceExtensionContext failed " << printResult(command.result_.value);
  }
}

void LogDxErrorLayer::pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " INTC_D3D12_CreateDeviceExtensionContext1 failed " << printResult(command.result_.value);
  }
}

void LogDxErrorLayer::pre(INTC_D3D12_SetApplicationInfoCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " INTC_D3D12_SetApplicationInfo failed " << printResult(command.result_.value);
  }
}

void LogDxErrorLayer::pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " INTC_DestroyDeviceExtensionContextCommand failed " << printResult(command.result_.value);
  }
}

void LogDxErrorLayer::pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " INTC_D3D12_CheckFeatureSupport failed " << printResult(command.result_.value);
  }
}

void LogDxErrorLayer::pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " INTC_D3D12_SetFeatureSupport failed " << printResult(command.result_.value);
  }
}

void LogDxErrorLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " INTC_D3D12_CreateComputePipelineState failed " << printResult(command.result_.value);
  }
}

void LogDxErrorLayer::pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key)<< " INTC_D3D12_CreatePlacedResource failed " << printResult(command.result_.value);
  }
}

void LogDxErrorLayer::pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " INTC_D3D12_CreateCommittedResource failed " << printResult(command.result_.value);
  }
}

void LogDxErrorLayer::pre(INTC_D3D12_CreateCommandQueueCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " INTC_D3D12_CreateCommandQueueCommand failed " << printResult(command.result_.value);
  }
}

void LogDxErrorLayer::pre(INTC_D3D12_CreateReservedResourceCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " INTC_D3D12_CreateReservedResourceCommand failed " << printResult(command.result_.value);
  }
}

void LogDxErrorLayer::pre(INTC_D3D12_CreateHeapCommand& command) {
  preResult_ = command.result_.value;
}

void LogDxErrorLayer::post(INTC_D3D12_CreateHeapCommand& command) {
  if (isFailure(command.result_.value)) {
    Log(ERR) << callKeyToStr(command.key) << " INTC_D3D12_CreateHeapCommand failed " << printResult(command.result_.value);
  }
}

std::string LogDxErrorLayer::printResult(HRESULT result) {
  switch (result) {
  case E_INVALIDARG:
    return "E_INVALIDARG";
  case E_OUTOFMEMORY:
    return "E_OUTOFMEMORY";
  case E_NOTIMPL:
    return "E_NOTIMPL";
  case DXGI_ERROR_DEVICE_REMOVED:
    return "DXGI_ERROR_DEVICE_REMOVED";
  case DXGI_ERROR_DEVICE_HUNG:
    return "DXGI_ERROR_DEVICE_HUNG";
  case DXGI_ERROR_INVALID_CALL:
    return "DXGI_ERROR_INVALID_CALL";
  case DXGI_ERROR_UNSUPPORTED:
    return "DXGI_ERROR_UNSUPPORTED";
  case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
    return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT";
  case DXGI_ERROR_WAS_STILL_DRAWING:
    return "DXGI_ERROR_WAS_STILL_DRAWING";
  case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
    return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
  default:
    std::stringstream s;
    s << "0x" << std::hex << result;
    return s.str();
  }
}

} // namespace DirectX
} // namespace gits
