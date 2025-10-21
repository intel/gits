// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerCommandListService.h"
#include "analyzerService.h"
#include "gits.h"

namespace gits {
namespace DirectX {

AnalyzerCommandListService::AnalyzerCommandListService(
    AnalyzerService& analyzerService,
    DescriptorService& descriptorService,
    RootSignatureService& rootSignatureService,
    AnalyzerRaytracingService& raytracingService,
    AnalyzerExecuteIndirectService& executeIndirectService,
    bool commandListSubcapture)
    : analyzerService_(analyzerService),
      descriptorService_(descriptorService),
      rootSignatureService_(rootSignatureService),
      raytracingService_(raytracingService),
      executeIndirectService_(executeIndirectService),
      commandListSubcapture_(commandListSubcapture) {
  restoreTlases_ = Configurator::Get().directx.features.subcapture.restoreTLASes;
  optimize_ = Configurator::Get().directx.features.subcapture.optimize;
}

void AnalyzerCommandListService::commandListsRestore(const std::set<unsigned>& commandLists) {
  for (unsigned commandListKey : commandLists) {
    commandListRestore(commandListKey);
  }
}

void AnalyzerCommandListService::commandListRestore(unsigned commandListKey) {
  auto itCommandList = commandsByCommandList_.find(commandListKey);
  if (itCommandList == commandsByCommandList_.end()) {
    return;
  }
  for (auto& command : itCommandList->second) {
    switch (command->getId()) {
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARSTATE:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListClearStateCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DRAWINSTANCED:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListDrawInstancedCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DRAWINDEXEDINSTANCED:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListDrawIndexedInstancedCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DISPATCH:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListDispatchCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_COPYBUFFERREGION:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListCopyBufferRegionCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_COPYTEXTUREREGION:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListCopyTextureRegionCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_COPYRESOURCE:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListCopyResourceCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_COPYTILES:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListCopyTilesCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESOLVESUBRESOURCE:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListResolveSubresourceCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETPIPELINESTATE:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListSetPipelineStateCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESOURCEBARRIER:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListResourceBarrierCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_EXECUTEBUNDLE:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListExecuteBundleCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETDESCRIPTORHEAPS:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListSetDescriptorHeapsCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSIGNATURE:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetComputeRootSignatureCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTSIGNATURE:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTDESCRIPTORTABLE:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTDESCRIPTORTABLE:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTCONSTANTBUFFERVIEW:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTCONSTANTBUFFERVIEW:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSHADERRESOURCEVIEW:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTSHADERRESOURCEVIEW:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTUNORDEREDACCESSVIEW:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTUNORDEREDACCESSVIEW:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_IASETINDEXBUFFER:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListIASetIndexBufferCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_IASETVERTEXBUFFERS:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListIASetVertexBuffersCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SOSETTARGETS:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListSOSetTargetsCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListOMSetRenderTargetsCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARDEPTHSTENCILVIEW:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListClearDepthStencilViewCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARRENDERTARGETVIEW:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListClearRenderTargetViewCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWUINT:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWFLOAT:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DISCARDRESOURCE:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListDiscardResourceCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_BEGINQUERY:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListBeginQueryCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_ENDQUERY:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListEndQueryCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESOLVEQUERYDATA:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListResolveQueryDataCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETPREDICATION:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListSetPredicationCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_EXECUTEINDIRECT:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListExecuteIndirectCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_ATOMICCOPYBUFFERUINT:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_ATOMICCOPYBUFFERUINT64:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_RESOLVESUBRESOURCEREGION:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST2_WRITEBUFFERIMMEDIATE:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandList2WriteBufferImmediateCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST3_SETPROTECTEDRESOURCESESSION:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_BEGINRENDERPASS:
      commandAnalysis(static_cast<ID3D12GraphicsCommandList4BeginRenderPassCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_INITIALIZEMETACOMMAND:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandList4InitializeMetaCommandCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_EXECUTEMETACOMMAND:
      commandAnalysis(static_cast<ID3D12GraphicsCommandList4ExecuteMetaCommandCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_BUILDRAYTRACINGACCELERATIONSTRUCTURE:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO:
      commandAnalysis(
          static_cast<
              ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_COPYRAYTRACINGACCELERATIONSTRUCTURE:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand&>(
              *command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_SETPIPELINESTATE1:
      commandAnalysis(static_cast<ID3D12GraphicsCommandList4SetPipelineState1Command&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_DISPATCHRAYS:
      commandAnalysis(static_cast<ID3D12GraphicsCommandList4DispatchRaysCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST5_RSSETSHADINGRATEIMAGE:
      commandAnalysis(
          static_cast<ID3D12GraphicsCommandList5RSSetShadingRateImageCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST6_DISPATCHMESH:
      commandAnalysis(static_cast<ID3D12GraphicsCommandList6DispatchMeshCommand&>(*command));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST7_BARRIER:
      commandAnalysis(static_cast<ID3D12GraphicsCommandList7BarrierCommand&>(*command));
      break;
    }
  }
  commandsByCommandList_.erase(itCommandList);
}

void AnalyzerCommandListService::commandListReset(ID3D12GraphicsCommandListResetCommand& c) {
  if (!analyzerService_.inRange() && !commandListSubcapture_) {
    commandsByCommandList_.erase(c.object_.key);
  } else {
    resetCommandLists_[c.object_.key] = true;
  }
  commandListInfos_.erase(c.object_.key);
}

void AnalyzerCommandListService::createDescriptorHeap(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  descriptorHeapInfos_[c.ppvHeap_.key].type = c.pDescriptorHeapDesc_.value->Type;
  descriptorHeapInfos_[c.ppvHeap_.key].numDescriptors =
      c.pDescriptorHeapDesc_.value->NumDescriptors;
}

void AnalyzerCommandListService::copyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (analyzerService_.inRange()) {
    for (unsigned i = 0; i < c.NumDescriptors_.value; ++i) {
      DescriptorState* state = descriptorService_.getDescriptorState(
          c.SrcDescriptorRangeStart_.interfaceKey, c.SrcDescriptorRangeStart_.index + i);
      if (state) {
        objectsForRestore_.insert(state->resourceKey);
      }
      descriptors_.insert(
          {c.SrcDescriptorRangeStart_.interfaceKey, c.SrcDescriptorRangeStart_.index + i});
    }
    objectsForRestore_.insert(c.SrcDescriptorRangeStart_.interfaceKey);

    objectsForRestore_.insert(c.DestDescriptorRangeStart_.interfaceKey);
  }
}

void AnalyzerCommandListService::copyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (analyzerService_.inRange()) {
    for (unsigned i = 0; i < c.NumSrcDescriptorRanges_.value; ++i) {
      unsigned srcRangeSize =
          c.pSrcDescriptorRangeSizes_.value ? c.pSrcDescriptorRangeSizes_.value[i] : 1;
      for (unsigned j = 0; j < srcRangeSize; ++j) {
        DescriptorState* state =
            descriptorService_.getDescriptorState(c.pSrcDescriptorRangeStarts_.interfaceKeys[i],
                                                  c.pSrcDescriptorRangeStarts_.indexes[i] + j);
        if (state) {
          objectsForRestore_.insert(state->resourceKey);
        }
        descriptors_.insert({c.pSrcDescriptorRangeStarts_.interfaceKeys[i],
                             c.pSrcDescriptorRangeStarts_.indexes[i] + j});
      }
      objectsForRestore_.insert(c.pSrcDescriptorRangeStarts_.interfaceKeys[i]);
    }

    for (unsigned key : c.pDestDescriptorRangeStarts_.interfaceKeys) {
      objectsForRestore_.insert(key);
    }
  }
}

void AnalyzerCommandListService::setBindlessDescriptors(unsigned rootSignatureKey,
                                                        unsigned descriptorHeapKey,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                                        unsigned heapNumDescriptors) {
  std::vector<unsigned> indexes = rootSignatureService_.getBindlessDescriptorIndexes(
      rootSignatureKey, descriptorHeapKey, heapType, heapNumDescriptors);
  for (unsigned index : indexes) {
    DescriptorState* state = descriptorService_.getDescriptorState(descriptorHeapKey, index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
    }
    descriptors_.insert({descriptorHeapKey, index});
  }
  objectsForRestore_.insert(descriptorHeapKey);
}

bool AnalyzerCommandListService::inRange() {
  return analyzerService_.inRange();
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListClearStateCommand& c) {
  analyzerService_.notifyObject(c.pPipelineState_.key);
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListDrawInstancedCommand& c) {
  CommandListInfo& info = commandListInfos_[c.object_.key];
  if (info.viewDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = descriptorHeapInfos_[info.viewDescriptorHeap];
    setBindlessDescriptors(info.graphicsRootSignature, info.viewDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
  if (info.samplerDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = descriptorHeapInfos_[info.samplerDescriptorHeap];
    setBindlessDescriptors(info.graphicsRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) {
  CommandListInfo& info = commandListInfos_[c.object_.key];
  if (info.viewDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = descriptorHeapInfos_[info.viewDescriptorHeap];
    setBindlessDescriptors(info.graphicsRootSignature, info.viewDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
  if (info.samplerDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = descriptorHeapInfos_[info.samplerDescriptorHeap];
    setBindlessDescriptors(info.graphicsRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListDispatchCommand& c) {
  CommandListInfo& info = commandListInfos_[c.object_.key];
  if (info.viewDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = descriptorHeapInfos_[info.viewDescriptorHeap];
    setBindlessDescriptors(info.computeRootSignature, info.viewDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
  if (info.samplerDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = descriptorHeapInfos_[info.samplerDescriptorHeap];
    setBindlessDescriptors(info.computeRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListCopyBufferRegionCommand& c) {
  objectsForRestore_.insert(c.pDstBuffer_.key);
  objectsForRestore_.insert(c.pSrcBuffer_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  objectsForRestore_.insert(c.pDst_.resourceKey);
  objectsForRestore_.insert(c.pSrc_.resourceKey);
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListCopyResourceCommand& c) {
  objectsForRestore_.insert(c.pDstResource_.key);
  objectsForRestore_.insert(c.pSrcResource_.key);
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListCopyTilesCommand& c) {
  objectsForRestore_.insert(c.pTiledResource_.key);
  objectsForRestore_.insert(c.pBuffer_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListResolveSubresourceCommand& c) {
  objectsForRestore_.insert(c.pDstResource_.key);
  objectsForRestore_.insert(c.pSrcResource_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  objectsForRestore_.insert(c.pPipelineState_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  for (unsigned key : c.pBarriers_.resourceKeys) {
    objectsForRestore_.insert(key);
  }
  for (unsigned key : c.pBarriers_.resourceAfterKeys) {
    objectsForRestore_.insert(key);
  }
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListExecuteBundleCommand& c) {
  analyzerService_.notifyObject(c.pCommandList_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
  raytracingService_.setDescriptorHeaps(c);
  CommandListInfo& commandListInfo = commandListInfos_[c.object_.key];
  commandListInfo.viewDescriptorHeap = 0;
  commandListInfo.samplerDescriptorHeap = 0;
  for (unsigned key : c.ppDescriptorHeaps_.keys) {
    objectsForRestore_.insert(key);
    DescriptorHeapInfo& info = descriptorHeapInfos_[key];
    if (info.type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
      commandListInfo.viewDescriptorHeap = key;
    } else if (info.type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
      commandListInfo.samplerDescriptorHeap = key;
    }
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  commandListInfos_[c.object_.key].computeRootSignature = c.pRootSignature_.key;
  objectsForRestore_.insert(c.pRootSignature_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  commandListInfos_[c.object_.key].graphicsRootSignature = c.pRootSignature_.key;
  objectsForRestore_.insert(c.pRootSignature_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  unsigned rootSignatureKey = commandListInfos_[c.object_.key].computeRootSignature;
  GITS_ASSERT(rootSignatureKey);
  unsigned numDescriptors = descriptorHeapInfos_[c.BaseDescriptor_.interfaceKey].numDescriptors;
  GITS_ASSERT(numDescriptors);
  std::vector<unsigned> indexes = rootSignatureService_.getDescriptorTableIndexes(
      rootSignatureKey, c.BaseDescriptor_.interfaceKey, c.RootParameterIndex_.value,
      c.BaseDescriptor_.index, numDescriptors);
  for (unsigned index : indexes) {
    DescriptorState* state =
        descriptorService_.getDescriptorState(c.BaseDescriptor_.interfaceKey, index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
    }
    descriptors_.insert({c.BaseDescriptor_.interfaceKey, index});
  }
  objectsForRestore_.insert(c.BaseDescriptor_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  unsigned rootSignatureKey = commandListInfos_[c.object_.key].graphicsRootSignature;
  GITS_ASSERT(rootSignatureKey);
  unsigned numDescriptors = descriptorHeapInfos_[c.BaseDescriptor_.interfaceKey].numDescriptors;
  GITS_ASSERT(numDescriptors);
  std::vector<unsigned> indexes = rootSignatureService_.getDescriptorTableIndexes(
      rootSignatureKey, c.BaseDescriptor_.interfaceKey, c.RootParameterIndex_.value,
      c.BaseDescriptor_.index, numDescriptors);
  for (unsigned index : indexes) {
    DescriptorState* state =
        descriptorService_.getDescriptorState(c.BaseDescriptor_.interfaceKey, index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
    }
    descriptors_.insert({c.BaseDescriptor_.interfaceKey, index});
  }
  objectsForRestore_.insert(c.BaseDescriptor_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  objectsForRestore_.insert(c.BufferLocation_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  objectsForRestore_.insert(c.pView_.bufferLocationKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  for (unsigned i = 0; i < c.pViews_.size; ++i) {
    objectsForRestore_.insert(c.pViews_.bufferLocationKeys[i]);
  }
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  for (unsigned i = 0; i < c.pViews_.size; ++i) {
    objectsForRestore_.insert(c.pViews_.bufferLocationKeys[i]);
    objectsForRestore_.insert(c.pViews_.bufferFilledSizeLocationKeys[i]);
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (!c.RTsSingleHandleToDescriptorRange_.value) {
    for (unsigned i = 0; i < c.NumRenderTargetDescriptors_.value; ++i) {
      unsigned key = c.pRenderTargetDescriptors_.interfaceKeys[i];
      unsigned index = c.pRenderTargetDescriptors_.indexes[i];
      if (key) {
        objectsForRestore_.insert(key);
        DescriptorState* state = descriptorService_.getDescriptorState(key, index);
        if (state) {
          objectsForRestore_.insert(state->resourceKey);
        }
        descriptors_.insert({key, index});
      }
    }
  } else if (c.NumRenderTargetDescriptors_.value) {
    unsigned key = c.pRenderTargetDescriptors_.interfaceKeys[0];
    unsigned index = c.pRenderTargetDescriptors_.indexes[0];
    if (key) {
      objectsForRestore_.insert(key);
      for (unsigned i = 0; i < c.NumRenderTargetDescriptors_.value; ++i) {
        DescriptorState* state = descriptorService_.getDescriptorState(key, index);
        if (state) {
          objectsForRestore_.insert(state->resourceKey);
        }
        descriptors_.insert({key, index});
        ++index;
      }
    }
  }
  if (c.pDepthStencilDescriptor_.value) {
    unsigned key = c.pDepthStencilDescriptor_.interfaceKeys[0];
    unsigned index = c.pDepthStencilDescriptor_.indexes[0];
    if (key) {
      objectsForRestore_.insert(key);
      DescriptorState* state = descriptorService_.getDescriptorState(key, index);
      if (state) {
        objectsForRestore_.insert(state->resourceKey);
      }
      descriptors_.insert({key, index});
    }
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (c.DepthStencilView_.interfaceKey) {
    objectsForRestore_.insert(c.DepthStencilView_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(c.DepthStencilView_.interfaceKey,
                                                                   c.DepthStencilView_.index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
    }
    descriptors_.insert({c.DepthStencilView_.interfaceKey, c.DepthStencilView_.index});
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (c.RenderTargetView_.interfaceKey) {
    objectsForRestore_.insert(c.RenderTargetView_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(c.RenderTargetView_.interfaceKey,
                                                                   c.RenderTargetView_.index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
    }
    descriptors_.insert({c.RenderTargetView_.interfaceKey, c.RenderTargetView_.index});
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (c.ViewGPUHandleInCurrentHeap_.interfaceKey) {
    objectsForRestore_.insert(c.ViewGPUHandleInCurrentHeap_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(
        c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
    }
    descriptors_.insert(
        {c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index});
  }
  if (c.ViewCPUHandle_.interfaceKey) {
    objectsForRestore_.insert(c.ViewCPUHandle_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(c.ViewCPUHandle_.interfaceKey,
                                                                   c.ViewCPUHandle_.index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
    }
    descriptors_.insert({c.ViewCPUHandle_.interfaceKey, c.ViewCPUHandle_.index});
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (c.ViewGPUHandleInCurrentHeap_.interfaceKey) {
    objectsForRestore_.insert(c.ViewGPUHandleInCurrentHeap_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(
        c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
    }
    descriptors_.insert(
        {c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index});
  }
  if (c.ViewCPUHandle_.interfaceKey) {
    objectsForRestore_.insert(c.ViewCPUHandle_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(c.ViewCPUHandle_.interfaceKey,
                                                                   c.ViewCPUHandle_.index);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
    }
    descriptors_.insert({c.ViewCPUHandle_.interfaceKey, c.ViewCPUHandle_.index});
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListDiscardResourceCommand& c) {
  objectsForRestore_.insert(c.pResource_.key);
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListBeginQueryCommand& c) {
  analyzerService_.notifyObject(c.pQueryHeap_.key);
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListEndQueryCommand& c) {
  analyzerService_.notifyObject(c.pQueryHeap_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListResolveQueryDataCommand& c) {
  analyzerService_.notifyObject(c.pQueryHeap_.key);
  analyzerService_.notifyObject(c.pDestinationBuffer_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetPredicationCommand& c) {
  analyzerService_.notifyObject(c.pBuffer_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  executeIndirectService_.executeIndirect(c);
  CommandListInfo& info = commandListInfos_[c.object_.key];
  if (info.viewDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = descriptorHeapInfos_[info.viewDescriptorHeap];
    if (info.computeRootSignature) {
      setBindlessDescriptors(info.computeRootSignature, info.viewDescriptorHeap, heapInfo.type,
                             heapInfo.numDescriptors);
    }
    if (info.graphicsRootSignature) {
      setBindlessDescriptors(info.graphicsRootSignature, info.viewDescriptorHeap, heapInfo.type,
                             heapInfo.numDescriptors);
    }
  }
  if (info.samplerDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = descriptorHeapInfos_[info.samplerDescriptorHeap];
    if (info.computeRootSignature) {
      setBindlessDescriptors(info.computeRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                             heapInfo.numDescriptors);
    }
    if (info.graphicsRootSignature) {
      setBindlessDescriptors(info.graphicsRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                             heapInfo.numDescriptors);
    }
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand& c) {
  analyzerService_.notifyObject(c.pDstBuffer_.key);
  analyzerService_.notifyObject(c.pSrcBuffer_.key);
  analyzerService_.notifyObjects(c.ppDependentResources_.keys);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& c) {
  analyzerService_.notifyObject(c.pDstBuffer_.key);
  analyzerService_.notifyObject(c.pSrcBuffer_.key);
  analyzerService_.notifyObjects(c.ppDependentResources_.keys);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c) {
  objectsForRestore_.insert(c.pDstResource_.key);
  objectsForRestore_.insert(c.pSrcResource_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  for (unsigned key : c.pParams_.destKeys) {
    objectsForRestore_.insert(key);
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  objectsForRestore_.insert(c.pProtectedResourceSession_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList4BeginRenderPassCommand& c) {
  for (unsigned i = 0; i < c.pRenderTargets_.size; ++i) {
    unsigned descriptorKey = c.pRenderTargets_.descriptorKeys[i];
    if (descriptorKey) {
      objectsForRestore_.insert(descriptorKey);
      DescriptorState* state = descriptorService_.getDescriptorState(
          descriptorKey, c.pRenderTargets_.descriptorIndexes[i]);
      if (state) {
        objectsForRestore_.insert(state->resourceKey);
      }
      descriptors_.insert({descriptorKey, c.pRenderTargets_.descriptorIndexes[i]});
    }
  }
  for (unsigned key : c.pRenderTargets_.resolveSrcResourceKeys) {
    objectsForRestore_.insert(key);
  }
  for (unsigned key : c.pRenderTargets_.resolveDstResourceKeys) {
    objectsForRestore_.insert(key);
  }
  if (c.pDepthStencil_.descriptorKey) {
    objectsForRestore_.insert(c.pDepthStencil_.descriptorKey);
    DescriptorState* state = descriptorService_.getDescriptorState(
        c.pDepthStencil_.descriptorKey, c.pDepthStencil_.descriptorIndex);
    if (state) {
      objectsForRestore_.insert(state->resourceKey);
    }
    descriptors_.insert({c.pDepthStencil_.descriptorKey, c.pDepthStencil_.descriptorIndex});
  }
  objectsForRestore_.insert(c.pDepthStencil_.resolveSrcDepthKey);
  objectsForRestore_.insert(c.pDepthStencil_.resolveDstDepthKey);
  objectsForRestore_.insert(c.pDepthStencil_.resolveSrcStencilKey);
  objectsForRestore_.insert(c.pDepthStencil_.resolveDstStencilKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c) {
  objectsForRestore_.insert(c.pMetaCommand_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& c) {
  objectsForRestore_.insert(c.pMetaCommand_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    raytracingService_.buildTlas(c);
  } else if (c.pDesc_.value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (c.pDesc_.sourceAccelerationStructureKey) {
      raytracingService_.addAccelerationStructureSource(c.pDesc_.sourceAccelerationStructureKey,
                                                        c.pDesc_.sourceAccelerationStructureOffset);
    }
  }
}

void AnalyzerCommandListService::command(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (analyzerService_.inRange()) {
    if (!resetCommandLists_[c.object_.key]) {
      commandListRestore(c.object_.key);
    }
    commandAnalysis(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand(c));
  }
  if (!analyzerService_.inRange() && restoreTlases_) {
    if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
      raytracingService_.buildTlas(c);
    }
  }

  if (optimize_) {
    objectsForRestore_.insert(c.pDesc_.destAccelerationStructureKey);
    if (c.pDesc_.sourceAccelerationStructureKey) {
      objectsForRestore_.insert(c.pDesc_.sourceAccelerationStructureKey);
    }
    if (!(c.pDesc_.scratchAccelerationStructureKey & Command::stateRestoreKeyMask)) {
      objectsForRestore_.insert(c.pDesc_.scratchAccelerationStructureKey);
    }
    for (unsigned key : c.pDesc_.inputKeys) {
      objectsForRestore_.insert(key);
    }
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  for (unsigned i = 0; i < c.NumSourceAccelerationStructures_.value; ++i) {
    raytracingService_.addAccelerationStructureSource(
        c.pSourceAccelerationStructureData_.interfaceKeys[i],
        c.pSourceAccelerationStructureData_.offsets[i]);
    if (optimize_) {
      objectsForRestore_.insert(c.pSourceAccelerationStructureData_.interfaceKeys[i]);
    }
  }
  if (optimize_) {
    objectsForRestore_.insert(c.pDesc_.destBufferKey);
  }
}

void AnalyzerCommandListService::command(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (analyzerService_.inRange()) {
    if (!resetCommandLists_[c.object_.key]) {
      commandListRestore(c.object_.key);
    }
    commandAnalysis(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.object_.key].emplace_back(
        new ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand(c));
  }

  if (optimize_) {
    objectsForRestore_.insert(c.DestAccelerationStructureData_.interfaceKey);
    objectsForRestore_.insert(c.SourceAccelerationStructureData_.interfaceKey);
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  raytracingService_.addAccelerationStructureSource(c.SourceAccelerationStructureData_.interfaceKey,
                                                    c.SourceAccelerationStructureData_.offset);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  raytracingService_.setPipelineState(c);

  objectsForRestore_.insert(c.object_.key);
  objectsForRestore_.insert(c.pStateObject_.key);
  if (checkedStateObjectSubobjects_.find(c.pStateObject_.key) ==
      checkedStateObjectSubobjects_.end()) {
    const std::set<unsigned> subobjects =
        raytracingService_.getStateObjectAllSubobjects(c.pStateObject_.key);
    for (unsigned key : subobjects) {
      objectsForRestore_.insert(key);
    }
    checkedStateObjectSubobjects_.insert(c.pStateObject_.key);
  }
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  raytracingService_.dispatchRays(c);
  objectsForRestore_.insert(c.pDesc_.rayGenerationShaderRecordKey);
  objectsForRestore_.insert(c.pDesc_.missShaderTableKey);
  objectsForRestore_.insert(c.pDesc_.hitGroupTableKey);
  objectsForRestore_.insert(c.pDesc_.callableShaderTableKey);

  CommandListInfo& info = commandListInfos_[c.object_.key];
  if (info.viewDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = descriptorHeapInfos_[info.viewDescriptorHeap];
    setBindlessDescriptors(info.computeRootSignature, info.viewDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
  if (info.samplerDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = descriptorHeapInfos_[info.samplerDescriptorHeap];
    setBindlessDescriptors(info.computeRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& c) {
  analyzerService_.notifyObject(c.shadingRateImage_.key);
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandList6DispatchMeshCommand& c) {
  CommandListInfo& info = commandListInfos_[c.object_.key];
  if (info.viewDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = descriptorHeapInfos_[info.viewDescriptorHeap];
    setBindlessDescriptors(info.computeRootSignature, info.viewDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
  if (info.samplerDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = descriptorHeapInfos_[info.samplerDescriptorHeap];
    setBindlessDescriptors(info.computeRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandList7BarrierCommand& c) {
  for (unsigned key : c.pBarrierGroups_.resourceKeys) {
    objectsForRestore_.insert(key);
  }
}

void AnalyzerCommandListService::command(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (analyzerService_.inRange()) {
    if (!resetCommandLists_[c.pCommandList_.key]) {
      commandListRestore(c.pCommandList_.key);
    }
    commandAnalysis(c);
  } else if (!commandListSubcapture_) {
    commandsByCommandList_[c.pCommandList_.key].emplace_back(
        new NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand(c));
  }
  if (!analyzerService_.inRange() && restoreTlases_) {
    if (c.pParams.value->pDesc->inputs.type ==
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
      LOG_ERROR << "NvAPI top level build not handled";
    }
  }

  objectsForRestore_.insert(c.pParams.destAccelerationStructureKey);
  if (c.pParams.sourceAccelerationStructureKey) {
    objectsForRestore_.insert(c.pParams.sourceAccelerationStructureKey);
  }
  if (!(c.pParams.scratchAccelerationStructureKey & Command::stateRestoreKeyMask)) {
    objectsForRestore_.insert(c.pParams.scratchAccelerationStructureKey);
  }
  for (unsigned key : c.pParams.inputKeys) {
    objectsForRestore_.insert(key);
  }
}

void AnalyzerCommandListService::commandAnalysis(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (c.pParams.value->pDesc->inputs.type ==
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    LOG_ERROR << "NvAPI top level build not handled";
  } else if (c.pParams.value->pDesc->inputs.type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (c.pParams.sourceAccelerationStructureKey) {
      raytracingService_.addAccelerationStructureSource(
          c.pParams.sourceAccelerationStructureKey, c.pParams.sourceAccelerationStructureOffset);
    }
  }
}

void AnalyzerCommandListService::command(
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  objectsForRestore_.insert(c.pParams.destOpacityMicromapArrayDataKey);
  if (c.pParams.inputBufferKey) {
    objectsForRestore_.insert(c.pParams.inputBufferKey);
  }
  if (c.pParams.perOMMDescsKey) {
    objectsForRestore_.insert(c.pParams.perOMMDescsKey);
  }
  if (!(c.pParams.scratchOpacityMicromapArrayDataKey & Command::stateRestoreKeyMask)) {
    objectsForRestore_.insert(c.pParams.scratchOpacityMicromapArrayDataKey);
  }
}

} // namespace DirectX
} // namespace gits
