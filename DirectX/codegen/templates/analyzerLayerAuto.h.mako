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
#include "analyzerService.h"
#include "descriptorService.h"
#include "analyzerCommandListService.h"
#include "descriptorRootSignatureService.h"
#include "analyzerRaytracingService.h"
#include "analyzerExecuteIndirectService.h"
#include "capturePlayerGpuAddressService.h"
#include "capturePlayerDescriptorHandleService.h"
#include "capturePlayerShaderIdentifierService.h"
#include "subcaptureRange.h"
#include "resourceStateTracker.h"

namespace gits {
namespace DirectX {

class AnalyzerLayer : public Layer {
public:
  AnalyzerLayer(SubcaptureRange& subcaptureRange);

  %for function in functions:
  void Post(${function.name}Command& c) override;
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  void Post(${interface.name}${function.name}Command& c) override;
  %endfor
  %endfor
  void Post(IUnknownQueryInterfaceCommand& c) override;
  void Post(IUnknownAddRefCommand& c) override;
  void Post(IUnknownReleaseCommand& c) override;
  void Post(MappedDataMetaCommand& c) override;
  void Post(INTC_D3D12_GetSupportedVersionsCommand& c) override;
  void Post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) override;
  void Post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) override;
  void Post(INTC_DestroyDeviceExtensionContextCommand& c) override;
  void Post(INTC_D3D12_CheckFeatureSupportCommand& c) override;
  void Post(INTC_D3D12_CreateCommandQueueCommand& c) override;
  void Post(INTC_D3D12_CreateReservedResourceCommand& c) override;
  void Post(INTC_D3D12_SetFeatureSupportCommand& c) override;
  void Post(INTC_D3D12_GetResourceAllocationInfoCommand& c) override;
  void Post(INTC_D3D12_CreateComputePipelineStateCommand& c) override;
  void Post(INTC_D3D12_CreatePlacedResourceCommand& c) override;
  void Post(INTC_D3D12_CreateCommittedResourceCommand& c) override;
  void Post(INTC_D3D12_CreateHeapCommand& c) override;
  void Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) override;
  void Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) override;
  void Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) override;
  void Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) override;
  void Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) override;
  void Post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) override;
  void Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) override;
  void Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) override;
  void Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) override;
  void Pre(ID3D12GraphicsCommandListExecuteIndirectCommand& c) override;
  void Pre(ID3D12GraphicsCommandListDispatchCommand& c) override;
  void Pre(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) override;
  void Pre(ID3D12GraphicsCommandListDrawInstancedCommand& c) override;
  void Pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;

private:
  bool m_Optimize{};
  bool m_OptimizeRaytracing{};
  SubcaptureRange& m_SubcaptureRange;
  AnalyzerCommandListService m_CommandListService;
  DescriptorService m_DescriptorService;
  DescriptorRootSignatureService m_RootSignatureService;
  CapturePlayerGpuAddressService m_GpuAddressService;
  CapturePlayerDescriptorHandleService m_DescriptorHandleService;
  CapturePlayerShaderIdentifierService m_ShaderIdentifierService;
  AnalyzerRaytracingService m_RaytracingService;
  AnalyzerExecuteIndirectService m_ExecuteIndirectService;
  AnalyzerService m_AnalyzerService;
  ResourceStateTracker m_ResourceStateTracker;
};

} // namespace DirectX
} // namespace gits
