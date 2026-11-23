// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "skipCallsOnResultLayerAuto.h"
#include "nvapi.h"

namespace gits {
namespace DirectX {

void SkipCallsOnResultLayer::Pre(IDXGISwapChainPresentCommand& command) {
  if (FAILED(command.m_Result.Value)) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(IDXGISwapChain1Present1Command& command) {
  if (FAILED(command.m_Result.Value)) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(NvAPI_InitializeCommand& command) {
  if (command.m_Result.Value != NVAPI_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(NvAPI_UnloadCommand& command) {
  if (command.m_Result.Value != NVAPI_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  if (command.m_Result.Value != NVAPI_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  if (command.m_Result.Value != NVAPI_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  if (command.m_Result.Value != NVAPI_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  if (command.m_Result.Value != NVAPI_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  if (command.m_Result.Value != NVAPI_OK) {
    command.Skip = true;
  }
}

void SkipCallsOnResultLayer::Pre(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  if (command.m_Result.Value != NVAPI_OK) {
    command.Skip = true;
  }
}

} // namespace DirectX
} // namespace gits
