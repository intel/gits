// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "configurationLib.h"

namespace gits {
namespace DirectX {

class LogDxErrorLayer : public Layer {
public:
  LogDxErrorLayer() : Layer("LogDxError") {
    m_IsPlayer = Configurator::IsPlayer();
  }

  virtual void Post(ID3D12FenceGetCompletedValueCommand& command);
  virtual void Pre(IUnknownQueryInterfaceCommand& command);
  virtual void Post(IUnknownQueryInterfaceCommand& command);
  %for function in functions:
  %if function.ret.type in ['HRESULT', 'xess_result_t', 'xell_result_t', 'xefg_swapchain_result_t']:
  virtual void Pre(${function.name}Command& command);
  virtual void Post(${function.name}Command& command);
  %endif
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  %if function.ret.type == 'HRESULT':
  virtual void Pre(${interface.name}${function.name}Command& command);
  virtual void Post(${interface.name}${function.name}Command& command);
  %endif
  %endfor
  %endfor
  virtual void Pre(INTC_D3D12_GetSupportedVersionsCommand& command);
  virtual void Post(INTC_D3D12_GetSupportedVersionsCommand& command);
  virtual void Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command);
  virtual void Post(INTC_D3D12_CreateDeviceExtensionContextCommand& command);
  virtual void Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command);
  virtual void Post(INTC_D3D12_CreateDeviceExtensionContext1Command& command);
  virtual void Pre(INTC_D3D12_SetApplicationInfoCommand& command);
  virtual void Post(INTC_D3D12_SetApplicationInfoCommand& command);
  virtual void Pre(INTC_DestroyDeviceExtensionContextCommand& command);
  virtual void Post(INTC_DestroyDeviceExtensionContextCommand& command);
  virtual void Pre(INTC_D3D12_CheckFeatureSupportCommand& command);
  virtual void Post(INTC_D3D12_CheckFeatureSupportCommand& command);
  virtual void Pre(INTC_D3D12_SetFeatureSupportCommand& command);
  virtual void Post(INTC_D3D12_SetFeatureSupportCommand& command);
  virtual void Pre(INTC_D3D12_CreateComputePipelineStateCommand& command);
  virtual void Post(INTC_D3D12_CreateComputePipelineStateCommand& command);
  virtual void Pre(INTC_D3D12_CreatePlacedResourceCommand& command);
  virtual void Post(INTC_D3D12_CreatePlacedResourceCommand& command);
  virtual void Pre(INTC_D3D12_CreateCommittedResourceCommand& command);
  virtual void Post(INTC_D3D12_CreateCommittedResourceCommand& command);
  virtual void Pre(INTC_D3D12_CreateCommandQueueCommand& command);
  virtual void Post(INTC_D3D12_CreateCommandQueueCommand& command);
  virtual void Pre(INTC_D3D12_CreateReservedResourceCommand& command);
  virtual void Post(INTC_D3D12_CreateReservedResourceCommand& command);
  virtual void Pre(INTC_D3D12_CreateHeapCommand& command);
  virtual void Post(INTC_D3D12_CreateHeapCommand& command);
  virtual void Pre(NvAPI_InitializeCommand& command) override;
  virtual void Post(NvAPI_InitializeCommand& command) override;
  virtual void Pre(NvAPI_UnloadCommand& command) override;
  virtual void Post(NvAPI_UnloadCommand& command) override;
  virtual void Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) override;
  virtual void Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) override;
  virtual void Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  virtual void Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) override;
  virtual void Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  virtual void Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) override;
  virtual void Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  virtual void Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  virtual void Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  virtual void Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;
  virtual void Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;
  virtual void Post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) override;

private:
  bool isFailure(HRESULT result) {
    return FAILED(result) && result != E_NOINTERFACE && result != DXGI_ERROR_NOT_FOUND &&
           (!m_IsPlayer || result != m_PreResult);
  }
  std::string printResult(HRESULT result);

  bool isFailureXess(xess_result_t result) {
    return result != XESS_RESULT_SUCCESS && (!m_IsPlayer || result != m_PreResultXess);
  }

  bool isFailureNvAPI(NvAPI_Status result);
  std::string printResult(NvAPI_Status result);

  bool isFailureXell(xell_result_t result) {
    return result != XELL_RESULT_SUCCESS && (!m_IsPlayer || result != m_PreResultXell);
  }

  bool isFailureXefg(xefg_swapchain_result_t result) {
    return result != XEFG_SWAPCHAIN_RESULT_SUCCESS && (!m_IsPlayer || result != m_PreResultXefg);
  }

private:
  bool m_IsPlayer{};
  HRESULT m_PreResult{};
  xess_result_t m_PreResultXess{};
  NvAPI_Status m_PreResultNvAPI{};
  xell_result_t m_PreResultXell{};
  xefg_swapchain_result_t m_PreResultXefg{};
};

} // namespace DirectX
} // namespace gits
