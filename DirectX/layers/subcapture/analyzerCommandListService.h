// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "commandsCustom.h"
#include "descriptorService.h"
#include "descriptorRootSignatureService.h"
#include "analyzerRaytracingService.h"
#include "analyzerExecuteIndirectService.h"

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>

namespace gits {
namespace DirectX {

class AnalyzerService;

class AnalyzerCommandListService {
public:
  AnalyzerCommandListService(AnalyzerService& analyzerService,
                             DescriptorService& descriptorService,
                             DescriptorRootSignatureService& rootSignatureService,
                             AnalyzerRaytracingService& raytracingService,
                             AnalyzerExecuteIndirectService& executeIndirectService,
                             bool CommandListSubcapture);

  std::unordered_set<unsigned>& GetObjectsForRestore() {
    return m_ObjectsForRestore;
  }
  std::set<std::pair<unsigned, unsigned>>& GetDescriptors() {
    return m_Descriptors;
  }
  unsigned GetComputeRootSignatureKey(unsigned commandListKey) {
    return m_CommandListInfos[commandListKey].computeRootSignature;
  }

  std::set<unsigned>& GetTlases();

  void AddObjectForRestore(unsigned key) {
    if (key && m_Optimize) {
      m_ObjectsForRestore.insert(key);
    }
  }

  void CommandListsRestore(const std::set<unsigned>& commandLists);
  void CommandListReset(ID3D12GraphicsCommandListResetCommand& c);
  void CreateDescriptorHeap(ID3D12DeviceCreateDescriptorHeapCommand& c);
  void CreateCommandSignature(ID3D12DeviceCreateCommandSignatureCommand& c);
  void CopyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c);
  void CopyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c);
  void Present();

  template <typename CommandListCommand>
  void Command(CommandListCommand& c);
  void Command(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void Command(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c);
  void Command(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c);
  void Command(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c);

private:
  void CommandListRestore(unsigned commandListKey);
  void SetBindlessDescriptors(unsigned RootSignatureKey,
                              unsigned DescriptorHeapKey,
                              D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                              unsigned heapNumDescriptors);
  bool InRange();

  void CommandAnalysis(ID3D12GraphicsCommandListResetCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListClearStateCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListDrawInstancedCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListDispatchCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListCopyBufferRegionCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListCopyTextureRegionCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListCopyResourceCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListCopyTilesCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListResolveSubresourceCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetPipelineStateCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListResourceBarrierCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListExecuteBundleCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListIASetIndexBufferCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSOSetTargetsCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListDiscardResourceCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListBeginQueryCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListEndQueryCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListResolveQueryDataCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListSetPredicationCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandListExecuteIndirectCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& c);
  void CommandAnalysis(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList4BeginRenderPassCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void CommandAnalysis(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c);
  void CommandAnalysis(
      ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList4SetPipelineState1Command& c);
  void CommandAnalysis(ID3D12GraphicsCommandList4DispatchRaysCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList6DispatchMeshCommand& c);
  void CommandAnalysis(ID3D12GraphicsCommandList7BarrierCommand& c);

private:
  AnalyzerService& m_AnalyzerService;
  DescriptorService& m_DescriptorService;
  DescriptorRootSignatureService& m_RootSignatureService;
  AnalyzerRaytracingService& m_RaytracingService;
  AnalyzerExecuteIndirectService& m_ExecuteIndirectService;
  bool m_CommandListSubcapture{};
  bool m_Optimize{};
  bool m_DispatchRays{};
  bool m_FirstFrame{true};

  struct CommandListInfo {
    unsigned computeRootSignature{};
    unsigned graphicsRootSignature{};
    unsigned viewDescriptorHeap{};
    unsigned samplerDescriptorHeap{};
  };
  std::unordered_map<unsigned, CommandListInfo> m_CommandListInfos;

  struct DescriptorHeapInfo {
    D3D12_DESCRIPTOR_HEAP_TYPE type{};
    unsigned numDescriptors{};
  };
  std::unordered_map<unsigned, DescriptorHeapInfo> m_DescriptorHeapInfos;
  std::unordered_set<unsigned> m_DispatchRaysCommandSignatures;

  std::unordered_map<unsigned, std::vector<std::unique_ptr<::gits::DirectX::Command>>>
      m_CommandsByCommandList;
  std::unordered_map<unsigned, bool> m_ResetCommandLists;

  std::set<unsigned> m_CheckedStateObjectSubobjects;

  std::unordered_set<unsigned> m_ObjectsForRestore;
  std::set<std::pair<unsigned, unsigned>> m_Descriptors;

  bool m_RestoreTlases{};
  std::set<unsigned> m_TlasBuildKeys;
};

template <typename CommandListCommand>
void AnalyzerCommandListService::Command(CommandListCommand& c) {
  if (InRange()) {
    if (!m_ResetCommandLists[c.m_Object.Key]) {
      CommandListRestore(c.m_Object.Key);
    }
    CommandAnalysis(c);
  } else if (!m_CommandListSubcapture) {
    m_CommandsByCommandList[c.m_Object.Key].emplace_back(new CommandListCommand(c));
  }
}

} // namespace DirectX
} // namespace gits
