// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "config.h"

namespace gits {
namespace DirectX {

class LogDxErrorLayer : public Layer {
public:
  LogDxErrorLayer() : Layer("LogDxError") {
    isPlayer_ = Configurator::IsPlayer();
  }

  virtual void post(ID3D12FenceGetCompletedValueCommand& command);
  virtual void pre(IUnknownQueryInterfaceCommand& command);
  virtual void post(IUnknownQueryInterfaceCommand& command);
  %for function in functions:
  %if function.ret.type == 'HRESULT' or function.ret.type == 'xess_result_t':
  virtual void pre(${function.name}Command& command);
  virtual void post(${function.name}Command& command);
  %endif
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  %if function.ret.type == 'HRESULT':
  virtual void pre(${interface.name}${function.name}Command& command);
  virtual void post(${interface.name}${function.name}Command& command);
  %endif
  %endfor
  %endfor
  virtual void pre(INTC_D3D12_GetSupportedVersionsCommand& command);
  virtual void post(INTC_D3D12_GetSupportedVersionsCommand& command);
  virtual void pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command);
  virtual void post(INTC_D3D12_CreateDeviceExtensionContextCommand& command);
  virtual void pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command);
  virtual void post(INTC_D3D12_CreateDeviceExtensionContext1Command& command);
  virtual void pre(INTC_D3D12_SetApplicationInfoCommand& command);
  virtual void post(INTC_D3D12_SetApplicationInfoCommand& command);
  virtual void pre(INTC_DestroyDeviceExtensionContextCommand& command);
  virtual void post(INTC_DestroyDeviceExtensionContextCommand& command);
  virtual void pre(INTC_D3D12_CheckFeatureSupportCommand& command);
  virtual void post(INTC_D3D12_CheckFeatureSupportCommand& command);
  virtual void pre(INTC_D3D12_SetFeatureSupportCommand& command);
  virtual void post(INTC_D3D12_SetFeatureSupportCommand& command);
  virtual void pre(INTC_D3D12_CreateComputePipelineStateCommand& command);
  virtual void post(INTC_D3D12_CreateComputePipelineStateCommand& command);
  virtual void pre(INTC_D3D12_CreatePlacedResourceCommand& command);
  virtual void post(INTC_D3D12_CreatePlacedResourceCommand& command);
  virtual void pre(INTC_D3D12_CreateCommittedResourceCommand& command);
  virtual void post(INTC_D3D12_CreateCommittedResourceCommand& command);
  virtual void pre(INTC_D3D12_CreateCommandQueueCommand& command);
  virtual void post(INTC_D3D12_CreateCommandQueueCommand& command);
  virtual void pre(INTC_D3D12_CreateReservedResourceCommand& command);
  virtual void post(INTC_D3D12_CreateReservedResourceCommand& command);
  virtual void pre(INTC_D3D12_CreateHeapCommand& command);
  virtual void post(INTC_D3D12_CreateHeapCommand& command);
  virtual void pre(NvAPI_InitializeCommand& command) override;
  virtual void post(NvAPI_InitializeCommand& command) override;
  virtual void pre(NvAPI_UnloadCommand& command) override;
  virtual void post(NvAPI_UnloadCommand& command) override;
  virtual void pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  virtual void post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  virtual void pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  virtual void post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  virtual void pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  virtual void post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  virtual void pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  virtual void post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  virtual void pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;
  virtual void post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;

private:
  bool isFailure(HRESULT result) {
    return FAILED(result) && result != E_NOINTERFACE && result != DXGI_ERROR_NOT_FOUND &&
           (!isPlayer_ || result != preResult_);
  }
  std::string printResult(HRESULT result);

  bool isFailureXess(xess_result_t result) {
    return result != XESS_RESULT_SUCCESS && (!isPlayer_ || result != preResultXess_);
  }

  bool isFailureNvAPI(NvAPI_Status result);
  std::string printResult(NvAPI_Status result);

private:
  bool isPlayer_{};
  HRESULT preResult_{};
  xess_result_t preResultXess_{};
  NvAPI_Status preResultNvAPI_{};
};

} // namespace DirectX
} // namespace gits
