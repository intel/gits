// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "IPlugin.h"
#include "layerAuto.h"
#include "bindingTableDump.h"
#include "instancesDump.h"
#include "executeIndirectDump.h"
#include "patchService.h"
#include "resourceStateTracker.h"
#include "capturePlayerGpuAddressService.h"

#include <memory>
#include <unordered_map>

namespace gits {
namespace DirectX {

class CpuPatchLayer : public Layer {
public:
  CpuPatchLayer(IPluginContext context);

  void Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) override;
  void Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) override;
  void Pre(ID3D12GraphicsCommandListExecuteIndirectCommand& c) override;
  void Post(ID3D12GraphicsCommandListExecuteIndirectCommand& c) override;
  void Pre(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void Post(ID3D12CommandQueueWaitCommand& c) override;
  void Post(ID3D12CommandQueueSignalCommand& c) override;
  void Post(ID3D12FenceSignalCommand& c) override;
  void Post(ID3D12DeviceCreateFenceCommand& c) override;
  void Post(ID3D12Device3EnqueueMakeResidentCommand& c) override;

  void Post(IDXGISwapChainPresentCommand& c) override;
  void Post(IDXGISwapChain1Present1Command& c) override;

  void Pre(IUnknownReleaseCommand& c) override;

  void Pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;
  void Post(ID3D12ResourceGetGPUVirtualAddressCommand& c) override;

  void Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;
  void Post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) override;

  void Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;
  void Post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) override;

  void Post(ID3D12DeviceCreateCommittedResourceCommand& c) override;
  void Post(ID3D12Device4CreateCommittedResource1Command& c) override;
  void Post(ID3D12Device8CreateCommittedResource2Command& c) override;
  void Post(ID3D12Device10CreateCommittedResource3Command& c) override;
  void Post(INTC_D3D12_CreateCommittedResourceCommand& c) override;

  void Post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void Post(INTC_D3D12_CreatePlacedResourceCommand& c) override;

  void Post(ID3D12DeviceCreateReservedResourceCommand& c) override;
  void Post(ID3D12Device4CreateReservedResource1Command& c) override;
  void Post(ID3D12Device10CreateReservedResource2Command& c) override;
  void Post(INTC_D3D12_CreateReservedResourceCommand& c) override;

  void Post(ID3D12DeviceCreateCommandSignatureCommand& c) override;

  void Post(ID3D12GraphicsCommandListResourceBarrierCommand& c) override;
  void Post(ID3D12GraphicsCommandList7BarrierCommand& c) override;

private:
  enum class Mode {
    Store,
    Use
  };

  Mode mode_;
  size_t frameNumber_{1};
  std::unique_ptr<InstancesDump> instancesDump_;
  std::unique_ptr<BindingTableDump> bindingTableDump_;
  std::unique_ptr<ExecuteIndirectDump> executeIndirectDump_;
  std::unique_ptr<PatchService> patchService_;
  ResourceStateTracker resourceStateTracker_;
  CapturePlayerGpuAddressService addressService_;
  std::unordered_map<unsigned, ID3D12Resource*> resourceByKey_;
  unsigned commandListExecutionCount_{};
};

} // namespace DirectX
} // namespace gits
