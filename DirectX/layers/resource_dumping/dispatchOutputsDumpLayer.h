// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "bit_range.h"
#include "dispatchOutputsAnalyzer.h"
#include "dispatchOutputsDumpService.h"

#include <unordered_map>

namespace gits {
namespace DirectX {

class DispatchOutputsDumpLayer : public Layer {
public:
  DispatchOutputsDumpLayer();
  ~DispatchOutputsDumpLayer();
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

  void Post(IUnknownReleaseCommand& c) override;
  void Post(ID3D12DeviceCreateDescriptorHeapCommand& c) override;
  void Post(ID3D12DeviceCreateRenderTargetViewCommand& c) override;
  void Post(ID3D12DeviceCreateDepthStencilViewCommand& c) override;
  void Post(ID3D12Device15TryCreateRenderTargetViewCommand& c) override;
  void Post(ID3D12Device15TryCreateDepthStencilViewCommand& c) override;
  void Post(ID3D12DeviceCreateShaderResourceViewCommand& c) override;
  void Post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) override;
  void Post(ID3D12DeviceCreateConstantBufferViewCommand& c) override;
  void Post(ID3D12Device15TryCreateShaderResourceViewCommand& c) override;
  void Post(ID3D12Device15TryCreateUnorderedAccessViewCommand& c) override;
  void Post(ID3D12Device15TryCreateConstantBufferViewCommand& c) override;
  void Post(ID3D12DeviceCreateSamplerCommand& c) override;
  void Post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) override;
  void Post(ID3D12DeviceCopyDescriptorsCommand& c) override;
  void Post(ID3D12DeviceCreateRootSignatureCommand& c) override;

  void Post(StateRestoreBeginCommand& c) override;
  void Post(StateRestoreEndCommand& c) override;
  void Post(IDXGISwapChainPresentCommand& c) override;
  void Post(IDXGISwapChain1Present1Command& c) override;

  void Post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void Post(ID3D12CommandQueueWaitCommand& c) override;
  void Post(ID3D12CommandQueueSignalCommand& c) override;
  void Post(ID3D12FenceSignalCommand& c) override;
  void Post(ID3D12DeviceCreateFenceCommand& c) override;
  void Post(ID3D12Device3EnqueueMakeResidentCommand& c) override;

  void Post(ID3D12GraphicsCommandListResetCommand& c) override;
  void Post(ID3D12GraphicsCommandListClearStateCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) override;
  void Post(ID3D12GraphicsCommandListDispatchCommand& c) override;

  void Post(ID3D12GraphicsCommandListResourceBarrierCommand& c) override;
  void Post(ID3D12GraphicsCommandList7BarrierCommand& c) override;

private:
  DispatchOutputsAnalyzer m_DispatchOutputsAnalyzer;
  DispatchOutputsDumpService m_DispatchOutputsDumpService;

  BitRange m_FrameRange;
  BitRange m_DispatchRange;
  bool m_InAnalysis{};
  bool m_DryRun{};
  unsigned m_DispatchCount{};
  unsigned m_ExecuteCount{};
  unsigned m_CurrentFrame{1};
  std::unordered_map<unsigned, unsigned> m_DispatchCountByCommandList;
};

} // namespace DirectX
} // namespace gits
