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

#include <unordered_set>
#include <unordered_map>
#include <set>

namespace gits {
namespace DirectX {

class AnalyzerService;

class BindingService {
public:
  BindingService(AnalyzerService& analyzerService,
                 DescriptorService& descriptorService,
                 RootSignatureService& rootSignatureService,
                 AnalyzerRaytracingService& raytracingService);

  std::unordered_set<unsigned>& getObjectsForRestore() {
    return objectsForRestore_;
  }
  std::set<std::pair<unsigned, unsigned>>& getDescriptors() {
    return descriptors_;
  }

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
  void copyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c);
  void copyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c);
  void setPipelineState(ID3D12GraphicsCommandList4SetPipelineState1Command& c);
  void buildRaytracingAccelerationStructure(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);

private:
  unsigned getNumDescriptors(unsigned commandListKey, unsigned descriptorHeapKey);

private:
  AnalyzerService& analyzerService_;
  DescriptorService& descriptorService_;
  RootSignatureService& rootSignatureService_;
  AnalyzerRaytracingService& raytracingService_;

  std::unordered_map<unsigned, unsigned> computeRootSignatureByCommandList_;
  std::unordered_map<unsigned, unsigned> graphicsRootSignatureByCommandList_;

  struct DescriptorHeap {
    unsigned key;
    unsigned numDescriptors;
  };
  std::unordered_map<unsigned, std::vector<DescriptorHeap>> descriptorHeapsByCommandList_;

  std::unordered_set<unsigned> objectsForRestore_;
  std::set<std::pair<unsigned, unsigned>> descriptors_;
  bool restoreTlases_{};
};

} // namespace DirectX
} // namespace gits
