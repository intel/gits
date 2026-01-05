// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "dispatchOutputsDump.h"
#include "bit_range.h"
#include "resourceStateTracker.h"
#include "descriptorRootSignatureService.h"
#include "descriptorHeapTracker.h"

#include <unordered_map>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class DispatchOutputsDumpLayer : public Layer {
public:
  DispatchOutputsDumpLayer();
  void post(ID3D12DeviceCreateCommittedResourceCommand& c) override;
  void post(ID3D12Device4CreateCommittedResource1Command& c) override;
  void post(ID3D12Device8CreateCommittedResource2Command& c) override;
  void post(ID3D12Device10CreateCommittedResource3Command& c) override;
  void post(INTC_D3D12_CreateCommittedResourceCommand& c) override;
  void post(ID3D12DeviceCreatePlacedResourceCommand& c) override;
  void post(ID3D12Device8CreatePlacedResource1Command& c) override;
  void post(ID3D12Device10CreatePlacedResource2Command& c) override;
  void post(INTC_D3D12_CreatePlacedResourceCommand& c) override;
  void post(ID3D12DeviceCreateReservedResourceCommand& c) override;
  void post(ID3D12Device4CreateReservedResource1Command& c) override;
  void post(ID3D12Device10CreateReservedResource2Command& c) override;
  void post(INTC_D3D12_CreateReservedResourceCommand& c) override;

  void post(IUnknownReleaseCommand& c) override;
  void post(ID3D12DeviceCreateDescriptorHeapCommand& c) override;
  void post(ID3D12DeviceCreateRenderTargetViewCommand& c) override;
  void post(ID3D12DeviceCreateDepthStencilViewCommand& c) override;
  void post(ID3D12DeviceCreateShaderResourceViewCommand& c) override;
  void post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) override;
  void post(ID3D12DeviceCreateConstantBufferViewCommand& c) override;
  void post(ID3D12DeviceCreateSamplerCommand& c) override;
  void post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) override;
  void post(ID3D12DeviceCopyDescriptorsCommand& c) override;
  void post(ID3D12DeviceCreateRootSignatureCommand& c) override;

  void post(StateRestoreBeginCommand& c) override;
  void post(StateRestoreEndCommand& c) override;
  void post(IDXGISwapChainPresentCommand& c) override;
  void post(IDXGISwapChain1Present1Command& c) override;

  void post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void post(ID3D12CommandQueueWaitCommand& c) override;
  void post(ID3D12CommandQueueSignalCommand& c) override;
  void post(ID3D12FenceSignalCommand& c) override;
  void post(ID3D12DeviceCreateFenceCommand& c) override;
  void post(ID3D12Device3EnqueueMakeResidentCommand& c) override;

  void post(ID3D12GraphicsCommandListResetCommand& c) override;
  void post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) override;
  void post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) override;
  void post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) override;
  void post(ID3D12GraphicsCommandListDispatchCommand& c) override;

private:
  std::wstring dumpPath_;
  DispatchOutputsDump resourceDump_;
  BitRange frameRange_;
  BitRange dispatchRange_;
  unsigned dispatchCount_{};
  unsigned executeCount_{};
  bool stateRestorePhase_{};

  DescriptorRootSignatureService rootSignatureService_;
  DescriptorHeapTracker descriptorService_;

  std::unordered_map<unsigned, unsigned> dispatchCountByCommandList_;
  std::unordered_map<unsigned, ID3D12Resource*> resourceByKey_;

  struct CommandListInfo {
    unsigned computeRootSignature{};
  };
  std::unordered_map<unsigned, CommandListInfo> commandListInfos_;

  struct DescriptorHeapInfo {
    D3D12_DESCRIPTOR_HEAP_TYPE type{};
    unsigned numDescriptors{};
  };
  std::unordered_map<unsigned, DescriptorHeapInfo> descriptorHeapInfos_;

  struct DispatchOutput {
    unsigned resourceKey{};
    ID3D12Resource* resource{};
    unsigned slot{};
  };
  std::unordered_map<unsigned,
                     std::unordered_map<unsigned, std::unordered_map<unsigned, DispatchOutput>>>
      dispatchOutputsByResourceBySlotByCommandList_;

private:
  void createDescriptor(unsigned heapKey,
                        unsigned descriptorIndex,
                        unsigned resourceKey,
                        DescriptorHeapTracker::DescriptorInfo::DescriptorType descriptorType);
  void dumpComputeOutput(ID3D12GraphicsCommandList* commandList,
                         const DispatchOutput& dispatchOutput,
                         unsigned frame,
                         unsigned commandListDispatch);
};

} // namespace DirectX
} // namespace gits
