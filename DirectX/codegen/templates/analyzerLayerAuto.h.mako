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
#include "bindingService.h"
#include "analyzerCommandListRestoreService.h"
#include "rootSignatureService.h"
#include "analyzerRaytracingService.h"
#include "analyzerExecuteIndirectService.h"
#include "capturePlayerGpuAddressService.h"
#include "capturePlayerDescriptorHandleService.h"
#include "capturePlayerShaderIdentifierService.h"
#include "subcaptureRange.h"

namespace gits {
namespace DirectX {

class ObjectUsageNotifier {
public:
  virtual void notifyObject(unsigned objectKey) = 0;
};

class AnalyzerLayer : public Layer, public ObjectUsageNotifier {
public:
  AnalyzerLayer(SubcaptureRange& subcaptureRange);

  void notifyObject(unsigned objectKey) override;

  void post(IDXGISwapChainPresentCommand& c) override;
  void post(IDXGISwapChain1Present1Command& c) override;
  void post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void post(ID3D12CommandQueueWaitCommand& c) override;
  void post(ID3D12CommandQueueSignalCommand& c) override;
  void post(ID3D12FenceSignalCommand& c) override;
  void post(ID3D12DeviceCreateFenceCommand& c) override;
  void post(ID3D12Device3EnqueueMakeResidentCommand& c) override;
  void post(ID3D12FenceGetCompletedValueCommand& c) override;
  void post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) override;
  void post(ID3D12DeviceCopyDescriptorsCommand& c) override;
  void post(ID3D12DeviceCreateRenderTargetViewCommand& c) override;
  void post(ID3D12DeviceCreateDepthStencilViewCommand& c) override;
  void post(ID3D12DeviceCreateShaderResourceViewCommand& c) override;
  void post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) override;
  void post(ID3D12DeviceCreateConstantBufferViewCommand& c) override;
  void post(ID3D12DeviceCreateSamplerCommand& c) override;
  void post(ID3D12DeviceCreateDescriptorHeapCommand& c) override;
  void post(ID3D12DeviceCreateRootSignatureCommand& c) override;
  void post(ID3D12Device5CreateStateObjectCommand& c) override;
  void post(ID3D12Device7AddToStateObjectCommand& c) override;
  void pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;
  void pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) override;
  void pre(ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) override;
  void pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) override;
  void pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) override;
  void pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) override;
  void pre(ID3D12GraphicsCommandListExecuteIndirectCommand& c) override;
  void pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void post(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;
  void post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;
  void post(IUnknownReleaseCommand& command) override;
  void post(ID3D12DeviceCreateCommandSignatureCommand& c) override;
  void post(ID3D12DeviceCreateCommittedResourceCommand& c) override;
  void post(ID3D12Device4CreateCommittedResource1Command& c) override;
  void post(ID3D12Device8CreateCommittedResource2Command& c) override;
  void post(ID3D12Device10CreateCommittedResource3Command& c) override;
  void post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void post(MappedDataMetaCommand& c) override;
  void post(xessD3D12CreateContextCommand& c) override;
  void post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) override;
  void post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) override;
  void post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) override;
  void post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) override;
  %for interface in interfaces:
  %for function in interface.functions:
  %if interface.name.startswith('ID3D12GraphicsCommandList') and not function.name.startswith('SetName'):
  void post(${interface.name}${function.name}Command& c) override;
  %endif
  %endfor
  %endfor

private:
  bool optimize_{};
  bool optimizeRaytracing_{};
  SubcaptureRange& subcaptureRange_;
  BindingService bindingService_;
  AnalyzerCommandListRestoreService commandListRestoreService_;
  DescriptorService descriptorService_;
  RootSignatureService rootSignatureService_;
  CapturePlayerGpuAddressService gpuAddressService_;
  CapturePlayerDescriptorHandleService descriptorHandleService_;
  CapturePlayerShaderIdentifierService shaderIdentifierService_;
  AnalyzerRaytracingService raytracingService_;
  AnalyzerExecuteIndirectService executeIndirectService_;
  AnalyzerService analyzerService_;
};

} // namespace DirectX
} // namespace gits
