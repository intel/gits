// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerCommandListRestoreService.h"
#include "analyzerService.h"
#include "gits.h"

namespace gits {
namespace DirectX {

AnalyzerCommandListRestoreService::AnalyzerCommandListRestoreService(
    AnalyzerService& analyzerService, bool commandListSubcapture)
    : analyzerService_(analyzerService), commandListSubcapture_(commandListSubcapture) {
  optimize_ = Configurator::Get().directx.features.subcapture.optimize;
}

void AnalyzerCommandListRestoreService::commandListsRestore(
    const std::set<unsigned>& commandLists) {
  for (unsigned commandListKey : commandLists) {
    auto itCommandList = commandsByCommandList_.find(commandListKey);
    if (itCommandList == commandsByCommandList_.end()) {
      continue;
    }
    for (auto& command : itCommandList->second) {
      switch (command->getId()) {
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_COPYBUFFERREGION:
        copyBufferRegionImpl(
            static_cast<ID3D12GraphicsCommandListCopyBufferRegionCommand&>(*command));
        break;
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_COPYRESOURCE:
        copyResourceImpl(static_cast<ID3D12GraphicsCommandListCopyResourceCommand&>(*command));
        break;
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_COPYTEXTUREREGION:
        copyTextureRegionImpl(
            static_cast<ID3D12GraphicsCommandListCopyTextureRegionCommand&>(*command));
        break;
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_COPYTILES:
        copyTilesImpl(static_cast<ID3D12GraphicsCommandListCopyTilesCommand&>(*command));
        break;
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DISCARDRESOURCE:
        discardResourceImpl(
            static_cast<ID3D12GraphicsCommandListDiscardResourceCommand&>(*command));
        break;
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESOLVESUBRESOURCE:
        resolveSubresourceImpl(
            static_cast<ID3D12GraphicsCommandListResolveSubresourceCommand&>(*command));
        break;
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESOURCEBARRIER:
        resourceBarrierImpl(
            static_cast<ID3D12GraphicsCommandListResourceBarrierCommand&>(*command));
        break;
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETPIPELINESTATE:
        setPipelineStateImpl(
            static_cast<ID3D12GraphicsCommandListSetPipelineStateCommand&>(*command));
        break;
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_RESOLVESUBRESOURCEREGION:
        resolveSubresourceRegionImpl(
            static_cast<ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand&>(*command));
        break;
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST3_SETPROTECTEDRESOURCESESSION:
        setProtectedResourceSessionImpl(
            static_cast<ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand&>(*command));
        break;
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_INITIALIZEMETACOMMAND:
        initializeMetaCommandImpl(
            static_cast<ID3D12GraphicsCommandList4InitializeMetaCommandCommand&>(*command));
        break;
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST7_BARRIER:
        barrierImpl(static_cast<ID3D12GraphicsCommandList7BarrierCommand&>(*command));
        break;
      }
    }
    commandsByCommandList_.erase(itCommandList);
  }
}

void AnalyzerCommandListRestoreService::commandListReset(ID3D12GraphicsCommandListResetCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_.erase(c.object_.key);
  }
}

void AnalyzerCommandListRestoreService::copyBufferRegion(
    ID3D12GraphicsCommandListCopyBufferRegionCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListCopyBufferRegionCommand(c));
  }
}

void AnalyzerCommandListRestoreService::copyBufferRegionImpl(
    ID3D12GraphicsCommandListCopyBufferRegionCommand& c) {
  objectsForRestore_.insert(c.pDstBuffer_.key);
  objectsForRestore_.insert(c.pSrcBuffer_.key);
}

void AnalyzerCommandListRestoreService::copyResource(
    ID3D12GraphicsCommandListCopyResourceCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListCopyResourceCommand(c));
  }
}

void AnalyzerCommandListRestoreService::copyResourceImpl(
    ID3D12GraphicsCommandListCopyResourceCommand& c) {
  objectsForRestore_.insert(c.pDstResource_.key);
  objectsForRestore_.insert(c.pSrcResource_.key);
}

void AnalyzerCommandListRestoreService::copyTextureRegion(
    ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListCopyTextureRegionCommand(c));
  }
}

void AnalyzerCommandListRestoreService::copyTextureRegionImpl(
    ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  objectsForRestore_.insert(c.pDst_.resourceKey);
  objectsForRestore_.insert(c.pSrc_.resourceKey);
}

void AnalyzerCommandListRestoreService::copyTiles(ID3D12GraphicsCommandListCopyTilesCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListCopyTilesCommand(c));
  }
}

void AnalyzerCommandListRestoreService::copyTilesImpl(
    ID3D12GraphicsCommandListCopyTilesCommand& c) {
  objectsForRestore_.insert(c.pTiledResource_.key);
  objectsForRestore_.insert(c.pBuffer_.key);
}

void AnalyzerCommandListRestoreService::discardResource(
    ID3D12GraphicsCommandListDiscardResourceCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListDiscardResourceCommand(c));
  }
}

void AnalyzerCommandListRestoreService::discardResourceImpl(
    ID3D12GraphicsCommandListDiscardResourceCommand& c) {
  objectsForRestore_.insert(c.pResource_.key);
}

void AnalyzerCommandListRestoreService::resolveSubresource(
    ID3D12GraphicsCommandListResolveSubresourceCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListResolveSubresourceCommand(c));
  }
}

void AnalyzerCommandListRestoreService::resolveSubresourceImpl(
    ID3D12GraphicsCommandListResolveSubresourceCommand& c) {
  objectsForRestore_.insert(c.pDstResource_.key);
  objectsForRestore_.insert(c.pSrcResource_.key);
}

void AnalyzerCommandListRestoreService::resourceBarrier(
    ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListResourceBarrierCommand(c));
  }
}

void AnalyzerCommandListRestoreService::resourceBarrierImpl(
    ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  for (unsigned key : c.pBarriers_.resourceKeys) {
    objectsForRestore_.insert(key);
  }
  for (unsigned key : c.pBarriers_.resourceAfterKeys) {
    objectsForRestore_.insert(key);
  }
}

void AnalyzerCommandListRestoreService::setPipelineState(
    ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandListSetPipelineStateCommand(c));
  }
}

void AnalyzerCommandListRestoreService::setPipelineStateImpl(
    ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  objectsForRestore_.insert(c.pPipelineState_.key);
}

void AnalyzerCommandListRestoreService::resolveSubresourceRegion(
    ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand(c));
  }
}

void AnalyzerCommandListRestoreService::resolveSubresourceRegionImpl(
    ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c) {
  objectsForRestore_.insert(c.pDstResource_.key);
  objectsForRestore_.insert(c.pSrcResource_.key);
}

void AnalyzerCommandListRestoreService::setProtectedResourceSession(
    ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand(c));
  }
}

void AnalyzerCommandListRestoreService::setProtectedResourceSessionImpl(
    ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  objectsForRestore_.insert(c.pProtectedResourceSession_.key);
}

void AnalyzerCommandListRestoreService::initializeMetaCommand(
    ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandList4InitializeMetaCommandCommand(c));
  }
}

void AnalyzerCommandListRestoreService::initializeMetaCommandImpl(
    ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c) {
  objectsForRestore_.insert(c.pMetaCommand_.key);
}

void AnalyzerCommandListRestoreService::barrier(ID3D12GraphicsCommandList7BarrierCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandList7BarrierCommand(c));
  }
}

void AnalyzerCommandListRestoreService::barrierImpl(ID3D12GraphicsCommandList7BarrierCommand& c) {
  for (unsigned key : c.pBarrierGroups_.resourceKeys) {
    objectsForRestore_.insert(key);
  }
}

} // namespace DirectX
} // namespace gits
