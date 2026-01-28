// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerCommandListService.h"
#include "analyzerService.h"
#include "keyUtils.h"
#include "gits.h"

namespace gits {
namespace DirectX {

AnalyzerCommandListService::AnalyzerCommandListService(
    AnalyzerService& analyzerService,
    DescriptorService& descriptorService,
    DescriptorRootSignatureService& rootSignatureService,
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

std::set<unsigned>& AnalyzerCommandListService::getTlases() {
  if (dispatchRays_ && tlasBuildKeys_.empty()) {
    raytracingService_.getTlases(tlasBuildKeys_);
  }
  return tlasBuildKeys_;
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
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESET:
      commandAnalysis(static_cast<ID3D12GraphicsCommandListResetCommand&>(*command));
      break;
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
    case CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX:
      commandAnalysis(
          static_cast<NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand&>(*command));
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
    auto& commands = commandsByCommandList_[c.object_.key];
    commands.clear();
    commands.emplace_back(new ID3D12GraphicsCommandListResetCommand(c));
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

void AnalyzerCommandListService::createCommandSignature(
    ID3D12DeviceCreateCommandSignatureCommand& c) {
  D3D12_COMMAND_SIGNATURE_DESC* desc = c.pDesc_.value;
  for (unsigned i = 0; i < desc->NumArgumentDescs; ++i) {
    if (desc->pArgumentDescs[i].Type == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS) {
      dispatchRaysCommandSignatures_.insert(c.ppvCommandSignature_.key);
      break;
    }
  }
}

void AnalyzerCommandListService::copyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (analyzerService_.inRange()) {
    for (unsigned i = 0; i < c.NumDescriptors_.value; ++i) {
      DescriptorState* state = descriptorService_.getDescriptorState(
          c.SrcDescriptorRangeStart_.interfaceKey, c.SrcDescriptorRangeStart_.index + i);
      if (state) {
        addObjectForRestore(state->resourceKey);
        addObjectForRestore(state->auxiliaryResourceKey);
      }
      descriptors_.insert(
          {c.SrcDescriptorRangeStart_.interfaceKey, c.SrcDescriptorRangeStart_.index + i});
    }
    addObjectForRestore(c.SrcDescriptorRangeStart_.interfaceKey);

    addObjectForRestore(c.DestDescriptorRangeStart_.interfaceKey);
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
          addObjectForRestore(state->resourceKey);
          addObjectForRestore(state->auxiliaryResourceKey);
        }
        descriptors_.insert({c.pSrcDescriptorRangeStarts_.interfaceKeys[i],
                             c.pSrcDescriptorRangeStarts_.indexes[i] + j});
      }
      addObjectForRestore(c.pSrcDescriptorRangeStarts_.interfaceKeys[i]);
    }

    for (unsigned key : c.pDestDescriptorRangeStarts_.interfaceKeys) {
      addObjectForRestore(key);
    }
  }
}

void AnalyzerCommandListService::present() {
  firstFrame_ = false;
}

void AnalyzerCommandListService::setBindlessDescriptors(unsigned rootSignatureKey,
                                                        unsigned descriptorHeapKey,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                                        unsigned heapNumDescriptors) {
  if (!rootSignatureKey || !descriptorHeapKey) {
    return;
  }
  std::vector<unsigned> indexes = rootSignatureService_.getBindlessDescriptorIndexes(
      rootSignatureKey, descriptorHeapKey, heapType, heapNumDescriptors);
  for (unsigned index : indexes) {
    DescriptorState* state = descriptorService_.getDescriptorState(descriptorHeapKey, index);
    if (state) {
      addObjectForRestore(state->resourceKey);
      addObjectForRestore(state->auxiliaryResourceKey);
    }
    descriptors_.insert({descriptorHeapKey, index});
  }
  addObjectForRestore(descriptorHeapKey);
}

bool AnalyzerCommandListService::inRange() {
  return analyzerService_.inRange();
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListResetCommand& c) {
  addObjectForRestore(c.pAllocator_.key);
  addObjectForRestore(c.pInitialState_.key);
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListClearStateCommand& c) {
  addObjectForRestore(c.pPipelineState_.key);
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
  addObjectForRestore(c.pDstBuffer_.key);
  addObjectForRestore(c.pSrcBuffer_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  addObjectForRestore(c.pDst_.resourceKey);
  addObjectForRestore(c.pSrc_.resourceKey);
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListCopyResourceCommand& c) {
  addObjectForRestore(c.pDstResource_.key);
  addObjectForRestore(c.pSrcResource_.key);
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListCopyTilesCommand& c) {
  addObjectForRestore(c.pTiledResource_.key);
  addObjectForRestore(c.pBuffer_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListResolveSubresourceCommand& c) {
  addObjectForRestore(c.pDstResource_.key);
  addObjectForRestore(c.pSrcResource_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  addObjectForRestore(c.pPipelineState_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  for (unsigned key : c.pBarriers_.resourceKeys) {
    addObjectForRestore(key);
  }
  for (unsigned key : c.pBarriers_.resourceAfterKeys) {
    addObjectForRestore(key);
  }
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListExecuteBundleCommand& c) {
  addObjectForRestore(c.pCommandList_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {

  std::vector<AnalyzerRaytracingService::DescriptorHeapInfo> infos;
  for (unsigned i = 0; i < c.NumDescriptorHeaps_.value; ++i) {
    AnalyzerRaytracingService::DescriptorHeapInfo info;
    info.key = c.ppDescriptorHeaps_.keys[i];
    auto it = descriptorHeapInfos_.find(info.key);
    GITS_ASSERT(it != descriptorHeapInfos_.end());
    info.type = it->second.type;
    info.numDescriptors = it->second.numDescriptors;
    infos.push_back(info);
  }
  raytracingService_.setDescriptorHeaps(c.object_.key, infos);

  CommandListInfo& commandListInfo = commandListInfos_[c.object_.key];
  commandListInfo.viewDescriptorHeap = 0;
  commandListInfo.samplerDescriptorHeap = 0;
  for (unsigned key : c.ppDescriptorHeaps_.keys) {
    addObjectForRestore(key);
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
  addObjectForRestore(c.pRootSignature_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  commandListInfos_[c.object_.key].graphicsRootSignature = c.pRootSignature_.key;
  addObjectForRestore(c.pRootSignature_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (!c.BaseDescriptor_.value.ptr) {
    return;
  }
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
      addObjectForRestore(state->resourceKey);
      addObjectForRestore(state->auxiliaryResourceKey);
    }
    descriptors_.insert({c.BaseDescriptor_.interfaceKey, index});
  }
  addObjectForRestore(c.BaseDescriptor_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  if (!c.BaseDescriptor_.value.ptr) {
    return;
  }
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
      addObjectForRestore(state->resourceKey);
      addObjectForRestore(state->auxiliaryResourceKey);
    }
    descriptors_.insert({c.BaseDescriptor_.interfaceKey, index});
  }
  addObjectForRestore(c.BaseDescriptor_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  addObjectForRestore(c.BufferLocation_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  addObjectForRestore(c.BufferLocation_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  addObjectForRestore(c.BufferLocation_.interfaceKey);

  unsigned tlasBuildKey = raytracingService_.findTlas(AnalyzerRaytracingService::KeyOffset(
      c.BufferLocation_.interfaceKey, c.BufferLocation_.offset));
  if (tlasBuildKey) {
    tlasBuildKeys_.insert(tlasBuildKey);
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  addObjectForRestore(c.BufferLocation_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  addObjectForRestore(c.BufferLocation_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  addObjectForRestore(c.BufferLocation_.interfaceKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  addObjectForRestore(c.pView_.bufferLocationKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  for (unsigned i = 0; i < c.pViews_.size; ++i) {
    addObjectForRestore(c.pViews_.bufferLocationKeys[i]);
  }
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  for (unsigned i = 0; i < c.pViews_.size; ++i) {
    addObjectForRestore(c.pViews_.bufferLocationKeys[i]);
    addObjectForRestore(c.pViews_.bufferFilledSizeLocationKeys[i]);
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (!c.RTsSingleHandleToDescriptorRange_.value) {
    for (unsigned i = 0; i < c.NumRenderTargetDescriptors_.value; ++i) {
      unsigned key = c.pRenderTargetDescriptors_.interfaceKeys[i];
      unsigned index = c.pRenderTargetDescriptors_.indexes[i];
      if (key) {
        addObjectForRestore(key);
        DescriptorState* state = descriptorService_.getDescriptorState(key, index);
        if (state) {
          addObjectForRestore(state->resourceKey);
          addObjectForRestore(state->auxiliaryResourceKey);
        }
        descriptors_.insert({key, index});
      }
    }
  } else if (c.NumRenderTargetDescriptors_.value) {
    unsigned key = c.pRenderTargetDescriptors_.interfaceKeys[0];
    unsigned index = c.pRenderTargetDescriptors_.indexes[0];
    if (key) {
      addObjectForRestore(key);
      for (unsigned i = 0; i < c.NumRenderTargetDescriptors_.value; ++i) {
        DescriptorState* state = descriptorService_.getDescriptorState(key, index);
        if (state) {
          addObjectForRestore(state->resourceKey);
          addObjectForRestore(state->auxiliaryResourceKey);
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
      addObjectForRestore(key);
      DescriptorState* state = descriptorService_.getDescriptorState(key, index);
      if (state) {
        addObjectForRestore(state->resourceKey);
        addObjectForRestore(state->auxiliaryResourceKey);
      }
      descriptors_.insert({key, index});
    }
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (c.DepthStencilView_.interfaceKey) {
    addObjectForRestore(c.DepthStencilView_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(c.DepthStencilView_.interfaceKey,
                                                                   c.DepthStencilView_.index);
    if (state) {
      addObjectForRestore(state->resourceKey);
      addObjectForRestore(state->auxiliaryResourceKey);
    }
    descriptors_.insert({c.DepthStencilView_.interfaceKey, c.DepthStencilView_.index});
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (c.RenderTargetView_.interfaceKey) {
    addObjectForRestore(c.RenderTargetView_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(c.RenderTargetView_.interfaceKey,
                                                                   c.RenderTargetView_.index);
    if (state) {
      addObjectForRestore(state->resourceKey);
      addObjectForRestore(state->auxiliaryResourceKey);
    }
    descriptors_.insert({c.RenderTargetView_.interfaceKey, c.RenderTargetView_.index});
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (c.ViewGPUHandleInCurrentHeap_.interfaceKey) {
    addObjectForRestore(c.ViewGPUHandleInCurrentHeap_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(
        c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index);
    if (state) {
      addObjectForRestore(state->resourceKey);
      addObjectForRestore(state->auxiliaryResourceKey);
    }
    descriptors_.insert(
        {c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index});
  }
  if (c.ViewCPUHandle_.interfaceKey) {
    addObjectForRestore(c.ViewCPUHandle_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(c.ViewCPUHandle_.interfaceKey,
                                                                   c.ViewCPUHandle_.index);
    if (state) {
      addObjectForRestore(state->resourceKey);
      addObjectForRestore(state->auxiliaryResourceKey);
    }
    descriptors_.insert({c.ViewCPUHandle_.interfaceKey, c.ViewCPUHandle_.index});
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (c.ViewGPUHandleInCurrentHeap_.interfaceKey) {
    addObjectForRestore(c.ViewGPUHandleInCurrentHeap_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(
        c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index);
    if (state) {
      addObjectForRestore(state->resourceKey);
      addObjectForRestore(state->auxiliaryResourceKey);
    }
    descriptors_.insert(
        {c.ViewGPUHandleInCurrentHeap_.interfaceKey, c.ViewGPUHandleInCurrentHeap_.index});
  }
  if (c.ViewCPUHandle_.interfaceKey) {
    addObjectForRestore(c.ViewCPUHandle_.interfaceKey);
    DescriptorState* state = descriptorService_.getDescriptorState(c.ViewCPUHandle_.interfaceKey,
                                                                   c.ViewCPUHandle_.index);
    if (state) {
      addObjectForRestore(state->resourceKey);
      addObjectForRestore(state->auxiliaryResourceKey);
    }
    descriptors_.insert({c.ViewCPUHandle_.interfaceKey, c.ViewCPUHandle_.index});
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListDiscardResourceCommand& c) {
  addObjectForRestore(c.pResource_.key);
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListBeginQueryCommand& c) {
  addObjectForRestore(c.pQueryHeap_.key);
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandListEndQueryCommand& c) {
  addObjectForRestore(c.pQueryHeap_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListResolveQueryDataCommand& c) {
  addObjectForRestore(c.pQueryHeap_.key);
  addObjectForRestore(c.pDestinationBuffer_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListSetPredicationCommand& c) {
  addObjectForRestore(c.pBuffer_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandListExecuteIndirectCommand& c) {

  if (dispatchRaysCommandSignatures_.find(c.pCommandSignature_.key) !=
      dispatchRaysCommandSignatures_.end()) {
    dispatchRays_ = true;
  }

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
  addObjectForRestore(c.pDstBuffer_.key);
  addObjectForRestore(c.pSrcBuffer_.key);
  for (unsigned key : c.ppDependentResources_.keys) {
    addObjectForRestore(key);
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& c) {
  addObjectForRestore(c.pDstBuffer_.key);
  addObjectForRestore(c.pSrcBuffer_.key);
  for (unsigned key : c.ppDependentResources_.keys) {
    addObjectForRestore(key);
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c) {
  addObjectForRestore(c.pDstResource_.key);
  addObjectForRestore(c.pSrcResource_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  for (unsigned key : c.pParams_.destKeys) {
    addObjectForRestore(key);
  }
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  addObjectForRestore(c.pProtectedResourceSession_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList4BeginRenderPassCommand& c) {
  for (unsigned i = 0; i < c.pRenderTargets_.size; ++i) {
    unsigned descriptorKey = c.pRenderTargets_.descriptorKeys[i];
    if (descriptorKey) {
      addObjectForRestore(descriptorKey);
      DescriptorState* state = descriptorService_.getDescriptorState(
          descriptorKey, c.pRenderTargets_.descriptorIndexes[i]);
      if (state) {
        addObjectForRestore(state->resourceKey);
        addObjectForRestore(state->auxiliaryResourceKey);
      }
      descriptors_.insert({descriptorKey, c.pRenderTargets_.descriptorIndexes[i]});
    }
  }
  for (unsigned key : c.pRenderTargets_.resolveSrcResourceKeys) {
    addObjectForRestore(key);
  }
  for (unsigned key : c.pRenderTargets_.resolveDstResourceKeys) {
    addObjectForRestore(key);
  }
  if (c.pDepthStencil_.descriptorKey) {
    addObjectForRestore(c.pDepthStencil_.descriptorKey);
    DescriptorState* state = descriptorService_.getDescriptorState(
        c.pDepthStencil_.descriptorKey, c.pDepthStencil_.descriptorIndex);
    if (state) {
      addObjectForRestore(state->resourceKey);
      addObjectForRestore(state->auxiliaryResourceKey);
    }
    descriptors_.insert({c.pDepthStencil_.descriptorKey, c.pDepthStencil_.descriptorIndex});
  }
  addObjectForRestore(c.pDepthStencil_.resolveSrcDepthKey);
  addObjectForRestore(c.pDepthStencil_.resolveDstDepthKey);
  addObjectForRestore(c.pDepthStencil_.resolveSrcStencilKey);
  addObjectForRestore(c.pDepthStencil_.resolveDstStencilKey);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c) {
  addObjectForRestore(c.pMetaCommand_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& c) {
  addObjectForRestore(c.pMetaCommand_.key);
}

void AnalyzerCommandListService::commandAnalysis(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    if (tlasBuildKeys_.find(c.key) == tlasBuildKeys_.end()) {
      tlasBuildKeys_.insert(c.key);
      raytracingService_.buildTlas(c);
    }
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
  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL &&
      analyzerService_.beforeRange() && (firstFrame_ || restoreTlases_ || commandListSubcapture_)) {
    tlasBuildKeys_.insert(c.key);
    raytracingService_.buildTlas(c);
  }

  if (optimize_) {
    addObjectForRestore(c.pDesc_.destAccelerationStructureKey);
    if (c.pDesc_.sourceAccelerationStructureKey) {
      addObjectForRestore(c.pDesc_.sourceAccelerationStructureKey);
    }
    if (!isStateRestoreKey(c.pDesc_.scratchAccelerationStructureKey)) {
      addObjectForRestore(c.pDesc_.scratchAccelerationStructureKey);
    }
    for (unsigned key : c.pDesc_.inputKeys) {
      addObjectForRestore(key);
    }
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
  if (c.pParams.value->pDesc->inputs.type ==
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    LOG_ERROR << "NvAPI top level build not handled";
  }

  if (optimize_) {
    addObjectForRestore(c.pParams.destAccelerationStructureKey);
    if (c.pParams.sourceAccelerationStructureKey) {
      addObjectForRestore(c.pParams.sourceAccelerationStructureKey);
    }
    if (!isStateRestoreKey(c.pParams.scratchAccelerationStructureKey)) {
      addObjectForRestore(c.pParams.scratchAccelerationStructureKey);
    }
    for (unsigned key : c.pParams.inputKeys) {
      addObjectForRestore(key);
    }
    for (unsigned key : c.pParams.destPostBuildBufferKeys) {
      addObjectForRestore(key);
    }
  }
}

void AnalyzerCommandListService::command(
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (optimize_) {
    addObjectForRestore(c.pParams.destOpacityMicromapArrayDataKey);
    if (c.pParams.inputBufferKey) {
      addObjectForRestore(c.pParams.inputBufferKey);
    }
    if (c.pParams.perOMMDescsKey) {
      addObjectForRestore(c.pParams.perOMMDescsKey);
    }
    if (!isStateRestoreKey(c.pParams.scratchOpacityMicromapArrayDataKey)) {
      addObjectForRestore(c.pParams.scratchOpacityMicromapArrayDataKey);
    }
    for (unsigned key : c.pParams.destPostBuildBufferKeys) {
      addObjectForRestore(key);
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
      addObjectForRestore(c.pSourceAccelerationStructureData_.interfaceKeys[i]);
    }
  }
  if (optimize_) {
    addObjectForRestore(c.pDesc_.destBufferKey);
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
    addObjectForRestore(c.DestAccelerationStructureData_.interfaceKey);
    addObjectForRestore(c.SourceAccelerationStructureData_.interfaceKey);
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

  addObjectForRestore(c.object_.key);
  addObjectForRestore(c.pStateObject_.key);
  if (checkedStateObjectSubobjects_.find(c.pStateObject_.key) ==
      checkedStateObjectSubobjects_.end()) {
    const std::set<unsigned> subobjects =
        raytracingService_.getStateObjectAllSubobjects(c.pStateObject_.key);
    for (unsigned key : subobjects) {
      addObjectForRestore(key);
    }
    checkedStateObjectSubobjects_.insert(c.pStateObject_.key);
  }
}

void AnalyzerCommandListService::commandAnalysis(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  dispatchRays_ = true;
  raytracingService_.dispatchRays(c);
  addObjectForRestore(c.pDesc_.rayGenerationShaderRecordKey);
  addObjectForRestore(c.pDesc_.missShaderTableKey);
  addObjectForRestore(c.pDesc_.hitGroupTableKey);
  addObjectForRestore(c.pDesc_.callableShaderTableKey);

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
  addObjectForRestore(c.shadingRateImage_.key);
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
    addObjectForRestore(key);
  }
}

} // namespace DirectX
} // namespace gits
