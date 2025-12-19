// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
                             bool commandListSubcapture);

  std::unordered_set<unsigned>& getObjectsForRestore() {
    return objectsForRestore_;
  }
  std::set<std::pair<unsigned, unsigned>>& getDescriptors() {
    return descriptors_;
  }
  unsigned getComputeRootSignatureKey(unsigned commandListKey) {
    return commandListInfos_[commandListKey].computeRootSignature;
  }

  std::set<unsigned>& getTlases();

  void addObjectForRestore(unsigned key) {
    if (key && optimize_) {
      objectsForRestore_.insert(key);
    }
  }

  void commandListsRestore(const std::set<unsigned>& commandLists);
  void commandListReset(ID3D12GraphicsCommandListResetCommand& c);
  void createDescriptorHeap(ID3D12DeviceCreateDescriptorHeapCommand& c);
  void createCommandSignature(ID3D12DeviceCreateCommandSignatureCommand& c);
  void copyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c);
  void copyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c);
  void present();

  template <typename CommandListCommand>
  void command(CommandListCommand& c);
  void command(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void command(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c);
  void command(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c);
  void command(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c);

private:
  void commandListRestore(unsigned commandListKey);
  void setBindlessDescriptors(unsigned rootSignatureKey,
                              unsigned descriptorHeapKey,
                              D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                              unsigned heapNumDescriptors);
  bool inRange();

  void commandAnalysis(ID3D12GraphicsCommandListResetCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListClearStateCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListDrawInstancedCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListDispatchCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListCopyBufferRegionCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListCopyTextureRegionCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListCopyResourceCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListCopyTilesCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListResolveSubresourceCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetPipelineStateCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListResourceBarrierCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListExecuteBundleCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListIASetIndexBufferCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSOSetTargetsCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListDiscardResourceCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListBeginQueryCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListEndQueryCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListResolveQueryDataCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListSetPredicationCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandListExecuteIndirectCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& c);
  void commandAnalysis(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList4BeginRenderPassCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void commandAnalysis(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c);
  void commandAnalysis(
      ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList4SetPipelineState1Command& c);
  void commandAnalysis(ID3D12GraphicsCommandList4DispatchRaysCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList6DispatchMeshCommand& c);
  void commandAnalysis(ID3D12GraphicsCommandList7BarrierCommand& c);

private:
  AnalyzerService& analyzerService_;
  DescriptorService& descriptorService_;
  DescriptorRootSignatureService& rootSignatureService_;
  AnalyzerRaytracingService& raytracingService_;
  AnalyzerExecuteIndirectService& executeIndirectService_;
  bool commandListSubcapture_{};
  bool optimize_{};
  bool dispatchRays_{};
  bool firstFrame_{true};

  struct CommandListInfo {
    unsigned computeRootSignature{};
    unsigned graphicsRootSignature{};
    unsigned viewDescriptorHeap{};
    unsigned samplerDescriptorHeap{};
  };
  std::unordered_map<unsigned, CommandListInfo> commandListInfos_;

  struct DescriptorHeapInfo {
    D3D12_DESCRIPTOR_HEAP_TYPE type{};
    unsigned numDescriptors{};
  };
  std::unordered_map<unsigned, DescriptorHeapInfo> descriptorHeapInfos_;
  std::unordered_set<unsigned> dispatchRaysCommandSignatures_;

  std::unordered_map<unsigned, std::vector<std::unique_ptr<Command>>> commandsByCommandList_;
  std::unordered_map<unsigned, bool> resetCommandLists_;

  std::set<unsigned> checkedStateObjectSubobjects_;

  std::unordered_set<unsigned> objectsForRestore_;
  std::set<std::pair<unsigned, unsigned>> descriptors_;

  bool restoreTlases_{};
  std::set<unsigned> tlasBuildKeys_;
};

template <typename CommandListCommand>
void AnalyzerCommandListService::command(CommandListCommand& c) {
  if (inRange()) {
    if (!resetCommandLists_[c.object_.key]) {
      commandListRestore(c.object_.key);
    }
    commandAnalysis(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(new CommandListCommand(c));
  }
}

} // namespace DirectX
} // namespace gits
