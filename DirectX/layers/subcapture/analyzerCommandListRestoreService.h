// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>

namespace gits {
namespace DirectX {

class AnalyzerService;

class AnalyzerCommandListRestoreService {
public:
  AnalyzerCommandListRestoreService(AnalyzerService& analyzerService, bool commandListSubcapture);

  std::unordered_set<unsigned>& getObjectsForRestore() {
    return objectsForRestore_;
  }

  void commandListsRestore(const std::set<unsigned>& commandLists);
  void commandListReset(ID3D12GraphicsCommandListResetCommand& c);
  void copyBufferRegion(ID3D12GraphicsCommandListCopyBufferRegionCommand& c);
  void copyResource(ID3D12GraphicsCommandListCopyResourceCommand& c);
  void copyTextureRegion(ID3D12GraphicsCommandListCopyTextureRegionCommand& c);
  void copyTiles(ID3D12GraphicsCommandListCopyTilesCommand& c);
  void discardResource(ID3D12GraphicsCommandListDiscardResourceCommand& c);
  void resolveSubresource(ID3D12GraphicsCommandListResolveSubresourceCommand& c);
  void resourceBarrier(ID3D12GraphicsCommandListResourceBarrierCommand& c);
  void setPipelineState(ID3D12GraphicsCommandListSetPipelineStateCommand& c);
  void resolveSubresourceRegion(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c);
  void setProtectedResourceSession(ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c);
  void initializeMetaCommand(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c);
  void barrier(ID3D12GraphicsCommandList7BarrierCommand& c);

private:
  void copyBufferRegionImpl(ID3D12GraphicsCommandListCopyBufferRegionCommand& c);
  void copyResourceImpl(ID3D12GraphicsCommandListCopyResourceCommand& c);
  void copyTextureRegionImpl(ID3D12GraphicsCommandListCopyTextureRegionCommand& c);
  void copyTilesImpl(ID3D12GraphicsCommandListCopyTilesCommand& c);
  void discardResourceImpl(ID3D12GraphicsCommandListDiscardResourceCommand& c);
  void resolveSubresourceImpl(ID3D12GraphicsCommandListResolveSubresourceCommand& c);
  void resourceBarrierImpl(ID3D12GraphicsCommandListResourceBarrierCommand& c);
  void setPipelineStateImpl(ID3D12GraphicsCommandListSetPipelineStateCommand& c);
  void resolveSubresourceRegionImpl(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c);
  void setProtectedResourceSessionImpl(
      ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c);
  void initializeMetaCommandImpl(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c);
  void barrierImpl(ID3D12GraphicsCommandList7BarrierCommand& c);

private:
  AnalyzerService& analyzerService_;
  bool commandListSubcapture_{};
  bool optimize_{};

  std::unordered_map<unsigned, std::vector<std::unique_ptr<Command>>> commandsByCommandList_;
  std::unordered_set<unsigned> objectsForRestore_;
};

} // namespace DirectX
} // namespace gits
