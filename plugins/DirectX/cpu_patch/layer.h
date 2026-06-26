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

  void Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& command) override;
  void Pre(ID3D12GraphicsCommandListExecuteIndirectCommand& command) override;
  void Post(ID3D12GraphicsCommandListExecuteIndirectCommand& command) override;
  void Pre(ID3D12CommandQueueExecuteCommandListsCommand& command) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& command) override;
  void Post(ID3D12CommandQueueWaitCommand& command) override;
  void Post(ID3D12CommandQueueSignalCommand& command) override;
  void Post(ID3D12FenceSignalCommand& command) override;
  void Post(ID3D12DeviceCreateFenceCommand& command) override;
  void Post(ID3D12Device3EnqueueMakeResidentCommand& command) override;

  void Post(IDXGISwapChainPresentCommand& command) override;
  void Post(IDXGISwapChain1Present1Command& command) override;

  void Pre(IUnknownReleaseCommand& command) override;

  void Pre(ID3D12ResourceGetGPUVirtualAddressCommand& command) override;
  void Post(ID3D12ResourceGetGPUVirtualAddressCommand& command) override;

  void Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& command) override;
  void Post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& command) override;

  void Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) override;
  void Post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) override;

  void Post(ID3D12DeviceCreateCommittedResourceCommand& command) override;
  void Post(ID3D12Device4CreateCommittedResource1Command& command) override;
  void Post(ID3D12Device8CreateCommittedResource2Command& command) override;
  void Post(ID3D12Device10CreateCommittedResource3Command& command) override;
  void Post(INTC_D3D12_CreateCommittedResourceCommand& command) override;

  void Post(ID3D12DeviceCreatePlacedResourceCommand& command) override;
  void Post(ID3D12Device8CreatePlacedResource1Command& command) override;
  void Post(ID3D12Device10CreatePlacedResource2Command& command) override;
  void Post(INTC_D3D12_CreatePlacedResourceCommand& command) override;

  void Post(ID3D12DeviceCreateReservedResourceCommand& command) override;
  void Post(ID3D12Device4CreateReservedResource1Command& command) override;
  void Post(ID3D12Device10CreateReservedResource2Command& command) override;
  void Post(INTC_D3D12_CreateReservedResourceCommand& command) override;

  void Post(ID3D12DeviceCreateCommandSignatureCommand& command) override;

  void Post(ID3D12GraphicsCommandListResourceBarrierCommand& command) override;
  void Post(ID3D12GraphicsCommandList7BarrierCommand& command) override;

private:
  enum class Mode {
    Store,
    Use
  };

  Mode m_Mode;
  size_t m_FrameNumber{1};
  std::unique_ptr<InstancesDump> m_InstancesDump;
  std::unique_ptr<BindingTableDump> m_BindingTableDump;
  std::unique_ptr<ExecuteIndirectDump> m_ExecuteIndirectDump;
  std::unique_ptr<PatchService> m_PatchService;
  ResourceStateTracker m_ResourceStateTracker;
  CapturePlayerGpuAddressService m_AddressService;
  std::unordered_map<unsigned, ID3D12Resource*> m_ResourceByKey;
  unsigned m_CommandListExecutionCount{};
};

} // namespace DirectX
} // namespace gits
