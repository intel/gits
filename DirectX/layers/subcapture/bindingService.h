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
#include "rootSignatureService.h"
#include "analyzerRaytracingService.h"
#include "analyzerExecuteIndirectService.h"

#include <unordered_set>
#include <unordered_map>
#include <set>
#include <memory>

namespace gits {
namespace DirectX {

class AnalyzerService;

class BindingService {
public:
  BindingService(AnalyzerService& analyzerService,
                 DescriptorService& descriptorService,
                 RootSignatureService& rootSignatureService,
                 AnalyzerRaytracingService& raytracingService,
                 AnalyzerExecuteIndirectService& executeIndirectService,
                 bool commandListSubcapture);

  std::unordered_set<unsigned>& getObjectsForRestore() {
    return objectsForRestore_;
  }
  std::set<std::pair<unsigned, unsigned>>& getDescriptors() {
    return descriptors_;
  }
  std::unordered_set<unsigned>& getBindingTablesResources() {
    return raytracingService_.getBindingTablesResources();
  }
  std::set<std::pair<unsigned, unsigned>>& getBindingTablesDescriptors() {
    return raytracingService_.getBindingTablesDescriptors();
  }

  void addObjectForRestore(unsigned key) {
    objectsForRestore_.insert(key);
  }
  void commandListsRestore(const std::set<unsigned>& commandLists);
  void commandListReset(ID3D12GraphicsCommandListResetCommand& c);
  void setDescriptorHeaps(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c);
  void setRootSignature(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c);
  void setRootSignature(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c);
  void setRootDescriptorTable(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c);
  void setRootDescriptorTable(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c);
  void setRootConstantBufferView(
      ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c);
  void setRootConstantBufferView(
      ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c);
  void setRootShaderResourceView(
      ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c);
  void setRootShaderResourceView(
      ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c);
  void setRootUnorderedAccessView(
      ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c);
  void setRootUnorderedAccessView(
      ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c);
  void setIndexBuffer(ID3D12GraphicsCommandListIASetIndexBufferCommand& c);
  void setVertexBuffers(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c);
  void setSOTargets(ID3D12GraphicsCommandListSOSetTargetsCommand& c);
  void setRenderTargets(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c);
  void clearView(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c);
  void clearView(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c);
  void clearView(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c);
  void clearView(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c);
  void setPipelineState(ID3D12GraphicsCommandList4SetPipelineState1Command& c);
  void buildRaytracingAccelerationStructure(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void copyRaytracingAccelerationStructure(
      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c);
  void dispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& c);
  void executeIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& c);
  void writeBufferImmediate(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c);
  void copyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c);
  void copyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c);

private:
  void setDescriptorHeapsImpl(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c);
  void setRootSignatureImpl(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c);
  void setRootSignatureImpl(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c);
  void setRootDescriptorTableImpl(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c);
  void setRootDescriptorTableImpl(
      ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c);
  void setRootConstantBufferViewImpl(
      ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c);
  void setRootConstantBufferViewImpl(
      ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c);
  void setRootShaderResourceViewImpl(
      ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c);
  void setRootShaderResourceViewImpl(
      ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c);
  void setRootUnorderedAccessViewImpl(
      ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c);
  void setRootUnorderedAccessViewImpl(
      ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c);
  void setIndexBufferImpl(ID3D12GraphicsCommandListIASetIndexBufferCommand& c);
  void setVertexBuffersImpl(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c);
  void setSOTargetsImpl(ID3D12GraphicsCommandListSOSetTargetsCommand& c);
  void setRenderTargetsImpl(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c);
  void clearViewImpl(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c);
  void clearViewImpl(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c);
  void clearViewImpl(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c);
  void clearViewImpl(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c);
  void setPipelineStateImpl(ID3D12GraphicsCommandList4SetPipelineState1Command& c);
  void buildRaytracingAccelerationStructureImpl(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void copyRaytracingAccelerationStructureImpl(
      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c);
  void dispatchRaysImpl(ID3D12GraphicsCommandList4DispatchRaysCommand& c);
  void executeIndirectImpl(ID3D12GraphicsCommandListExecuteIndirectCommand& c);
  void writeBufferImmediateImpl(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c);
  void commandListRestore(unsigned commandListKey);
  unsigned getNumDescriptors(unsigned commandListKey, unsigned descriptorHeapKey);

private:
  AnalyzerService& analyzerService_;
  DescriptorService& descriptorService_;
  RootSignatureService& rootSignatureService_;
  AnalyzerRaytracingService& raytracingService_;
  AnalyzerExecuteIndirectService& executeIndirectService_;
  bool commandListSubcapture_{};
  bool optimize_{};

  std::unordered_map<unsigned, unsigned> computeRootSignatureByCommandList_;
  std::unordered_map<unsigned, unsigned> graphicsRootSignatureByCommandList_;

  struct DescriptorHeap {
    unsigned key;
    unsigned numDescriptors;
  };
  std::unordered_map<unsigned, std::vector<DescriptorHeap>> descriptorHeapsByCommandList_;

  std::unordered_map<unsigned, std::vector<std::unique_ptr<Command>>> commandsByCommandList_;

  std::unordered_set<unsigned> objectsForRestore_;
  std::set<std::pair<unsigned, unsigned>> descriptors_;
  bool restoreTlases_{};
};

} // namespace DirectX
} // namespace gits
