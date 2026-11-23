// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "logDxErrorLayerAuto.h"
#include "to_string/toStr.h"
#include "log.h"
#include "nvapi.h"

namespace gits {
namespace DirectX {

void LogDxErrorLayer::Post(ID3D12FenceGetCompletedValueCommand& command) {
  if (command.m_Result.Value == UINT64_MAX) {
    LOG_ERROR << keyToStr(command.Key) << " ID3D12Fence::GetCompletedValue failed - device removed";
  }
}

void LogDxErrorLayer::Pre(IUnknownQueryInterfaceCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(IUnknownQueryInterfaceCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " IUnknown::QueryInterface failed " << PrintResult(command.m_Result.Value);
  }
}

%for function in functions:
%if function.ret.type == 'HRESULT':
void LogDxErrorLayer::Pre(${function.name}Command& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(${function.name}Command& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " ${function.name} failed " << PrintResult(command.m_Result.Value);
  }
}

%elif function.ret.type == 'xess_result_t':
void LogDxErrorLayer::Pre(${function.name}Command& command) {
  m_PreResultXess = command.m_Result.Value;
}

void LogDxErrorLayer::Post(${function.name}Command& command) {
  if (IsFailureXess(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " ${function.name} failed " << toStr(command.m_Result.Value);
  }
}

%elif function.ret.type == 'xell_result_t':
void LogDxErrorLayer::Pre(${function.name}Command& command) {
  m_PreResultXell = command.m_Result.Value;
}

void LogDxErrorLayer::Post(${function.name}Command& command) {
  if (IsFailureXell(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " ${function.name} failed " << toStr(command.m_Result.Value);
  }
}

%elif function.ret.type == 'xefg_swapchain_result_t':
void LogDxErrorLayer::Pre(${function.name}Command& command) {
  m_PreResultXefg = command.m_Result.Value;
}

void LogDxErrorLayer::Post(${function.name}Command& command) {
  if (IsFailureXefg(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " ${function.name} failed " << toStr(command.m_Result.Value);
  }
}

%endif
%endfor
%for interface in interfaces:
%for function in interface.functions:
%if function.ret.type == 'HRESULT':
void LogDxErrorLayer::Pre(${interface.name}${function.name}Command& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(${interface.name}${function.name}Command& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " ${interface.name}::${function.name} failed " << PrintResult(command.m_Result.Value);
  }
}

%endif
%endfor
%endfor
void LogDxErrorLayer::Pre(INTC_D3D12_GetSupportedVersionsCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_GetSupportedVersionsCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_D3D12_GetSupportedVersions failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_D3D12_CreateDeviceExtensionContext failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_D3D12_CreateDeviceExtensionContext1 failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext2Command& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_CreateDeviceExtensionContext2Command& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_D3D12_CreateDeviceExtensionContext2 failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_D3D12_SetApplicationInfoCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_D3D12_SetApplicationInfo failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_DestroyDeviceExtensionContextCommand failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_D3D12_CheckFeatureSupport failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_D3D12_SetFeatureSupport failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_D3D12_CreateComputePipelineState failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key)<< " INTC_D3D12_CreatePlacedResource failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_D3D12_CreateCommittedResource failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_D3D12_CreateCommandQueueCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_D3D12_CreateCommandQueueCommand failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_D3D12_CreateReservedResourceCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_D3D12_CreateReservedResourceCommand failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(INTC_D3D12_CreateHeapCommand& command) {
  m_PreResult = command.m_Result.Value;
}

void LogDxErrorLayer::Post(INTC_D3D12_CreateHeapCommand& command) {
  if (IsFailure(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " INTC_D3D12_CreateHeapCommand failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(NvAPI_InitializeCommand& command) {
  m_PreResultNvAPI = command.m_Result.Value;
}

void LogDxErrorLayer::Post(NvAPI_InitializeCommand& command) {
  if (IsFailureNvAPI(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " NvAPI_InitializeCommand failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(NvAPI_UnloadCommand& command) {
  m_PreResultNvAPI = command.m_Result.Value;
}

void LogDxErrorLayer::Post(NvAPI_UnloadCommand& command) {
  if (IsFailureNvAPI(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " NvAPI_UnloadCommand failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  m_PreResultNvAPI = command.m_Result.Value;
}

void LogDxErrorLayer::Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  if (IsFailureNvAPI(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " NvAPI_D3D12_SetCreatePipelineStateOptionsCommand failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  m_PreResultNvAPI = command.m_Result.Value;
}

void LogDxErrorLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  if (IsFailureNvAPI(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  m_PreResultNvAPI = command.m_Result.Value;
}

void LogDxErrorLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  if (IsFailureNvAPI(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  m_PreResultNvAPI = command.m_Result.Value;
}

void LogDxErrorLayer::Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  if (IsFailureNvAPI(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  m_PreResultNvAPI = command.m_Result.Value;
}

void LogDxErrorLayer::Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  if (IsFailureNvAPI(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand failed " << PrintResult(command.m_Result.Value);
  }
}

void LogDxErrorLayer::Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  m_PreResultNvAPI = command.m_Result.Value;
}

void LogDxErrorLayer::Post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  if (IsFailureNvAPI(command.m_Result.Value)) {
    LOG_ERROR << keyToStr(command.Key) << " NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand failed " << PrintResult(command.m_Result.Value);
  }
}

std::string LogDxErrorLayer::PrintResult(HRESULT result) {
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

bool LogDxErrorLayer::IsFailureNvAPI(NvAPI_Status result) {
    return result != NVAPI_OK && (!m_IsPlayer || result != m_PreResultNvAPI);
  }

std::string LogDxErrorLayer::PrintResult(NvAPI_Status result) {
  switch (result) {
  case NVAPI_OK:
    return "NVAPI_OK";
  case NVAPI_ERROR:
    return "NVAPI_ERROR";
  case NVAPI_LIBRARY_NOT_FOUND:
    return "NVAPI_LIBRARY_NOT_FOUND";
  case NVAPI_NVIDIA_DEVICE_NOT_FOUND:
    return "NVAPI_NVIDIA_DEVICE_NOT_FOUND";
  default:
    std::stringstream s;
    s << "0x" << std::hex << result;
    return s.str();
  }
}

} // namespace DirectX
} // namespace gits
