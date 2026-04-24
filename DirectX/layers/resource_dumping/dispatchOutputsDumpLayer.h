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
  void Post(ID3D12DeviceCreateShaderResourceViewCommand& c) override;
  void Post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) override;
  void Post(ID3D12DeviceCreateConstantBufferViewCommand& c) override;
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
  void Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) override;
  void Post(ID3D12GraphicsCommandListDispatchCommand& c) override;

private:
  std::wstring m_DumpPath;
  DispatchOutputsDump m_ResourceDump;
  BitRange m_FrameRange;
  BitRange m_DispatchRange;
  std::filesystem::path m_AnalysisFilePath;
  bool m_InAnalysis{};
  bool m_DryRun{};
  unsigned m_DispatchCount{};
  unsigned m_ExecuteCount{};
  unsigned m_CurrentFrame{1};

  DescriptorRootSignatureService m_RootSignatureService;
  DescriptorHeapTracker m_DescriptorService;

  std::unordered_map<unsigned, unsigned> m_DispatchCountByCommandList;
  std::unordered_map<unsigned, ID3D12Resource*> m_ResourceByKey;

  struct CommandListInfo {
    unsigned computeRootSignature{};
  };
  std::unordered_map<unsigned, CommandListInfo> m_CommandListInfos;

  struct DescriptorHeapInfo {
    D3D12_DESCRIPTOR_HEAP_TYPE type{};
    unsigned numDescriptors{};
  };
  std::unordered_map<unsigned, DescriptorHeapInfo> m_DescriptorHeapInfos;

  struct DispatchOutput {
    unsigned ResourceKey{};
    ID3D12Resource* resource{};
    unsigned slot{};
  };

  struct IndicesInfo {
    std::vector<unsigned> indices;
    unsigned DescriptorHeapKey{};
  };
  std::unordered_map<unsigned, std::unordered_map<unsigned, unsigned>>
      m_ResourceKeyFromSetViewBySlotByCommandList;
  std::unordered_map<unsigned, std::unordered_map<unsigned, IndicesInfo>>
      m_IndicesBySlotByCommandList;
  std::unordered_map<unsigned,
                     std::unordered_map<unsigned, std::unordered_map<unsigned, IndicesInfo>>>
      m_IndicesBySlotByDispatchByCommandList;
  std::map<unsigned, std::unordered_map<unsigned, std::set<unsigned>>>
      m_ResourceKeysBySlotByDispatch;

  struct DryRunInfo {
    std::map<unsigned, std::set<unsigned>> dispatchesWithTextureByFrame;
  } m_DryRunInfo;

private:
  void CreateDescriptor(unsigned heapKey,
                        unsigned DescriptorIndex,
                        unsigned ResourceKey,
                        DescriptorHeapTracker::DescriptorInfo::DescriptorKind descriptorKind);
  void DumpComputeOutput(ID3D12GraphicsCommandList* commandList,
                         const DispatchOutput& dispatchOutput,
                         unsigned frame,
                         unsigned commandListDispatch);
};

} // namespace DirectX
} // namespace gits
