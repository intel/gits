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
#include "analyzerService.h"
#include "descriptorService.h"
#include "analyzerCommandListService.h"
#include "rootSignatureService.h"
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
  void post(${function.name}Command& c) override;
  %endfor
  %for interface in interfaces:
  %for function in interface.functions:
  void post(${interface.name}${function.name}Command& c) override;
  %endfor
  %endfor
  void post(IUnknownQueryInterfaceCommand& c) override;
  void post(IUnknownAddRefCommand& c) override;
  void post(IUnknownReleaseCommand& c) override;
  void post(MappedDataMetaCommand& c) override;
  void post(INTC_D3D12_GetSupportedVersionsCommand& c) override;
  void post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) override;
  void post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) override;
  void post(INTC_DestroyDeviceExtensionContextCommand& c) override;
  void post(INTC_D3D12_CheckFeatureSupportCommand& c) override;
  void post(INTC_D3D12_CreateCommandQueueCommand& c) override;
  void post(INTC_D3D12_CreateReservedResourceCommand& c) override;
  void post(INTC_D3D12_SetFeatureSupportCommand& c) override;
  void post(INTC_D3D12_GetResourceAllocationInfoCommand& c) override;
  void post(INTC_D3D12_CreateComputePipelineStateCommand& c) override;
  void post(INTC_D3D12_CreatePlacedResourceCommand& c) override;
  void post(INTC_D3D12_CreateCommittedResourceCommand& c) override;
  void post(INTC_D3D12_CreateHeapCommand& c) override;
  void post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) override;
  void post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) override;
  void post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) override;
  void post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) override;
  void post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) override;
  void post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) override;
  void pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;
  void pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) override;
  void pre(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) override;
  void pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) override;
  void pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) override;
  void pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) override;
  void pre(ID3D12GraphicsCommandListExecuteIndirectCommand& c) override;
  void pre(ID3D12GraphicsCommandListDispatchCommand& c) override;
  void pre(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) override;
  void pre(ID3D12GraphicsCommandListDrawInstancedCommand& c) override;
  void pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;

private:
  bool optimize_{};
  bool optimizeRaytracing_{};
  SubcaptureRange& subcaptureRange_;
  AnalyzerCommandListService commandListService_;
  DescriptorService descriptorService_;
  RootSignatureService rootSignatureService_;
  CapturePlayerGpuAddressService gpuAddressService_;
  CapturePlayerDescriptorHandleService descriptorHandleService_;
  CapturePlayerShaderIdentifierService shaderIdentifierService_;
  AnalyzerRaytracingService raytracingService_;
  AnalyzerExecuteIndirectService executeIndirectService_;
  AnalyzerService analyzerService_;
  ResourceStateTracker resourceStateTracker_;
};

} // namespace DirectX
} // namespace gits
