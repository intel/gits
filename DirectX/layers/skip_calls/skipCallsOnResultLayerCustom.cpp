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

void SkipCallsOnResultLayer::pre(IDXGISwapChainPresentCommand& command) {
  if (FAILED(command.result_.value)) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(IDXGISwapChain1Present1Command& command) {
  if (FAILED(command.result_.value)) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(NvAPI_InitializeCommand& command) {
  if (command.result_.value != NVAPI_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(NvAPI_UnloadCommand& command) {
  if (command.result_.value != NVAPI_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  if (command.result_.value != NVAPI_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  if (command.result_.value != NVAPI_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  if (command.result_.value != NVAPI_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  if (command.result_.value != NVAPI_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  if (command.result_.value != NVAPI_OK) {
    command.skip = true;
  }
}

void SkipCallsOnResultLayer::pre(
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  if (command.result_.value != NVAPI_OK) {
    command.skip = true;
  }
}

} // namespace DirectX
} // namespace gits
