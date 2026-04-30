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
#include "log.h"

namespace gits {
namespace DirectX {

AnalyzerCommandListService::AnalyzerCommandListService(
    AnalyzerService& analyzerService,
    DescriptorService& descriptorService,
    DescriptorRootSignatureService& rootSignatureService,
    AnalyzerRaytracingService& raytracingService,
    AnalyzerExecuteIndirectService& executeIndirectService,
    bool CommandListSubcapture)
    : m_AnalyzerService(analyzerService),
      m_DescriptorService(descriptorService),
      m_RootSignatureService(rootSignatureService),
      m_RaytracingService(raytracingService),
      m_ExecuteIndirectService(executeIndirectService),
      m_CommandListSubcapture(CommandListSubcapture) {
  m_RestoreTlases = Configurator::Get().directx.features.subcapture.restoreTLASes;
  m_Optimize = Configurator::Get().directx.features.subcapture.optimize;
}

std::set<unsigned>& AnalyzerCommandListService::GetTlases() {
  if (m_DispatchRays && m_TlasBuildKeys.empty()) {
    m_RaytracingService.GetTlases(m_TlasBuildKeys);
  }
  return m_TlasBuildKeys;
}

void AnalyzerCommandListService::CommandListsRestore(const std::set<unsigned>& commandLists) {
  for (unsigned commandListKey : commandLists) {
    CommandListRestore(commandListKey);
  }
}

void AnalyzerCommandListService::CommandListRestore(unsigned commandListKey) {
  auto itCommandList = m_CommandsByCommandList.find(commandListKey);
  if (itCommandList == m_CommandsByCommandList.end()) {
    return;
  }
  for (auto& storedCommand : itCommandList->second) {
    switch (storedCommand->GetId()) {
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESET:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListResetCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARSTATE:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListClearStateCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DRAWINSTANCED:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListDrawInstancedCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DRAWINDEXEDINSTANCED:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListDrawIndexedInstancedCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DISPATCH:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListDispatchCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_COPYBUFFERREGION:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListCopyBufferRegionCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_COPYTEXTUREREGION:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListCopyTextureRegionCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_COPYRESOURCE:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListCopyResourceCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_COPYTILES:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListCopyTilesCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESOLVESUBRESOURCE:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListResolveSubresourceCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETPIPELINESTATE:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetPipelineStateCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESOURCEBARRIER:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListResourceBarrierCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_EXECUTEBUNDLE:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListExecuteBundleCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETDESCRIPTORHEAPS:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetDescriptorHeapsCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSIGNATURE:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetComputeRootSignatureCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTSIGNATURE:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTDESCRIPTORTABLE:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand&>(
          *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTDESCRIPTORTABLE:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand&>(
          *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTCONSTANTBUFFERVIEW:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand&>(
              *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTCONSTANTBUFFERVIEW:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand&>(
              *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSHADERRESOURCEVIEW:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand&>(
              *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTSHADERRESOURCEVIEW:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand&>(
              *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTUNORDEREDACCESSVIEW:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand&>(
              *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTUNORDEREDACCESSVIEW:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand&>(
              *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_IASETINDEXBUFFER:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListIASetIndexBufferCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_IASETVERTEXBUFFERS:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListIASetVertexBuffersCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SOSETTARGETS:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListSOSetTargetsCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListOMSetRenderTargetsCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARDEPTHSTENCILVIEW:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListClearDepthStencilViewCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARRENDERTARGETVIEW:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListClearRenderTargetViewCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWUINT:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand&>(
          *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWFLOAT:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand&>(
          *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DISCARDRESOURCE:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListDiscardResourceCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_BEGINQUERY:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListBeginQueryCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_ENDQUERY:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListEndQueryCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESOLVEQUERYDATA:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListResolveQueryDataCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETPREDICATION:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandListSetPredicationCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_EXECUTEINDIRECT:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandListExecuteIndirectCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_ATOMICCOPYBUFFERUINT:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_ATOMICCOPYBUFFERUINT64:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_RESOLVESUBRESOURCEREGION:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST2_WRITEBUFFERIMMEDIATE:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandList2WriteBufferImmediateCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST3_SETPROTECTEDRESOURCESESSION:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand&>(
          *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_BEGINRENDERPASS:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandList4BeginRenderPassCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_INITIALIZEMETACOMMAND:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandList4InitializeMetaCommandCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_EXECUTEMETACOMMAND:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandList4ExecuteMetaCommandCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_BUILDRAYTRACINGACCELERATIONSTRUCTURE:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand&>(
              *storedCommand));
      break;
    case CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX:
      CommandAnalysis(
          static_cast<NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO:
      CommandAnalysis(
          static_cast<
              ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand&>(
              *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_COPYRAYTRACINGACCELERATIONSTRUCTURE:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand&>(
              *storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_SETPIPELINESTATE1:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandList4SetPipelineState1Command&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_DISPATCHRAYS:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandList4DispatchRaysCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST5_RSSETSHADINGRATEIMAGE:
      CommandAnalysis(
          static_cast<ID3D12GraphicsCommandList5RSSetShadingRateImageCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST6_DISPATCHMESH:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandList6DispatchMeshCommand&>(*storedCommand));
      break;
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST7_BARRIER:
      CommandAnalysis(static_cast<ID3D12GraphicsCommandList7BarrierCommand&>(*storedCommand));
      break;
    }
  }
  m_CommandsByCommandList.erase(itCommandList);
}

void AnalyzerCommandListService::CommandListReset(ID3D12GraphicsCommandListResetCommand& c) {
  if (!m_AnalyzerService.InRange() && !m_CommandListSubcapture) {
    auto& commands = m_CommandsByCommandList[c.m_Object.Key];
    commands.clear();
    commands.emplace_back(new ID3D12GraphicsCommandListResetCommand(c));
  } else {
    m_ResetCommandLists[c.m_Object.Key] = true;
  }
  m_CommandListInfos.erase(c.m_Object.Key);
}

void AnalyzerCommandListService::CreateDescriptorHeap(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  m_DescriptorHeapInfos[c.m_ppvHeap.Key].type = c.m_pDescriptorHeapDesc.Value->Type;
  m_DescriptorHeapInfos[c.m_ppvHeap.Key].numDescriptors =
      c.m_pDescriptorHeapDesc.Value->NumDescriptors;
}

void AnalyzerCommandListService::CreateCommandSignature(
    ID3D12DeviceCreateCommandSignatureCommand& c) {
  D3D12_COMMAND_SIGNATURE_DESC* desc = c.m_pDesc.Value;
  for (unsigned i = 0; i < desc->NumArgumentDescs; ++i) {
    if (desc->pArgumentDescs[i].Type == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS) {
      m_DispatchRaysCommandSignatures.insert(c.m_ppvCommandSignature.Key);
      break;
    }
  }
}

void AnalyzerCommandListService::CopyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (m_AnalyzerService.InRange()) {
    for (unsigned i = 0; i < c.m_NumDescriptors.Value; ++i) {
      DescriptorState* state = m_DescriptorService.GetDescriptorState(
          c.m_SrcDescriptorRangeStart.InterfaceKey, c.m_SrcDescriptorRangeStart.Index + i);
      if (state) {
        AddObjectForRestore(state->ResourceKey);
        AddObjectForRestore(state->AuxiliaryResourceKey);
      }
      m_Descriptors.insert(
          {c.m_SrcDescriptorRangeStart.InterfaceKey, c.m_SrcDescriptorRangeStart.Index + i});
    }
    AddObjectForRestore(c.m_SrcDescriptorRangeStart.InterfaceKey);

    AddObjectForRestore(c.m_DestDescriptorRangeStart.InterfaceKey);
  }
}

void AnalyzerCommandListService::CopyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (m_AnalyzerService.InRange()) {
    for (unsigned i = 0; i < c.m_NumSrcDescriptorRanges.Value; ++i) {
      unsigned srcRangeSize =
          c.m_pSrcDescriptorRangeSizes.Value ? c.m_pSrcDescriptorRangeSizes.Value[i] : 1;
      for (unsigned j = 0; j < srcRangeSize; ++j) {
        DescriptorState* state =
            m_DescriptorService.GetDescriptorState(c.m_pSrcDescriptorRangeStarts.InterfaceKeys[i],
                                                   c.m_pSrcDescriptorRangeStarts.Indexes[i] + j);
        if (state) {
          AddObjectForRestore(state->ResourceKey);
          AddObjectForRestore(state->AuxiliaryResourceKey);
        }
        m_Descriptors.insert({c.m_pSrcDescriptorRangeStarts.InterfaceKeys[i],
                              c.m_pSrcDescriptorRangeStarts.Indexes[i] + j});
      }
      AddObjectForRestore(c.m_pSrcDescriptorRangeStarts.InterfaceKeys[i]);
    }

    for (unsigned key : c.m_pDestDescriptorRangeStarts.InterfaceKeys) {
      AddObjectForRestore(key);
    }
  }
}

void AnalyzerCommandListService::Present() {
  m_FirstFrame = false;
}

void AnalyzerCommandListService::SetBindlessDescriptors(unsigned rootSignatureKey,
                                                        unsigned descriptorHeapKey,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                                        unsigned heapNumDescriptors) {
  if (!rootSignatureKey || !descriptorHeapKey) {
    return;
  }
  std::vector<unsigned> Indexes = m_RootSignatureService.GetBindlessDescriptorIndexes(
      rootSignatureKey, descriptorHeapKey, heapType, heapNumDescriptors);
  for (unsigned index : Indexes) {
    DescriptorState* state = m_DescriptorService.GetDescriptorState(descriptorHeapKey, index);
    if (state) {
      AddObjectForRestore(state->ResourceKey);
      AddObjectForRestore(state->AuxiliaryResourceKey);
    }
    m_Descriptors.insert({descriptorHeapKey, index});
  }
  AddObjectForRestore(descriptorHeapKey);
}

bool AnalyzerCommandListService::InRange() {
  return m_AnalyzerService.InRange();
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandListResetCommand& c) {
  AddObjectForRestore(c.m_pAllocator.Key);
  AddObjectForRestore(c.m_pInitialState.Key);
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandListClearStateCommand& c) {
  AddObjectForRestore(c.m_pPipelineState.Key);
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandListDrawInstancedCommand& c) {
  CommandListInfo& info = m_CommandListInfos[c.m_Object.Key];
  if (info.viewDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = m_DescriptorHeapInfos[info.viewDescriptorHeap];
    SetBindlessDescriptors(info.graphicsRootSignature, info.viewDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
  if (info.samplerDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = m_DescriptorHeapInfos[info.samplerDescriptorHeap];
    SetBindlessDescriptors(info.graphicsRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) {
  CommandListInfo& info = m_CommandListInfos[c.m_Object.Key];
  if (info.viewDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = m_DescriptorHeapInfos[info.viewDescriptorHeap];
    SetBindlessDescriptors(info.graphicsRootSignature, info.viewDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
  if (info.samplerDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = m_DescriptorHeapInfos[info.samplerDescriptorHeap];
    SetBindlessDescriptors(info.graphicsRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandListDispatchCommand& c) {
  CommandListInfo& info = m_CommandListInfos[c.m_Object.Key];
  if (info.viewDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = m_DescriptorHeapInfos[info.viewDescriptorHeap];
    SetBindlessDescriptors(info.computeRootSignature, info.viewDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
  if (info.samplerDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = m_DescriptorHeapInfos[info.samplerDescriptorHeap];
    SetBindlessDescriptors(info.computeRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListCopyBufferRegionCommand& c) {
  AddObjectForRestore(c.m_pDstBuffer.Key);
  AddObjectForRestore(c.m_pSrcBuffer.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  AddObjectForRestore(c.m_pDst.ResourceKey);
  AddObjectForRestore(c.m_pSrc.ResourceKey);
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandListCopyResourceCommand& c) {
  AddObjectForRestore(c.m_pDstResource.Key);
  AddObjectForRestore(c.m_pSrcResource.Key);
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandListCopyTilesCommand& c) {
  AddObjectForRestore(c.m_pTiledResource.Key);
  AddObjectForRestore(c.m_pBuffer.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListResolveSubresourceCommand& c) {
  AddObjectForRestore(c.m_pDstResource.Key);
  AddObjectForRestore(c.m_pSrcResource.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  AddObjectForRestore(c.m_pPipelineState.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  for (unsigned key : c.m_pBarriers.ResourceKeys) {
    AddObjectForRestore(key);
  }
  for (unsigned key : c.m_pBarriers.ResourceAfterKeys) {
    AddObjectForRestore(key);
  }
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandListExecuteBundleCommand& c) {
  AddObjectForRestore(c.m_pCommandList.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {

  std::vector<AnalyzerRaytracingService::DescriptorHeapInfo> infos;
  for (unsigned i = 0; i < c.m_NumDescriptorHeaps.Value; ++i) {
    AnalyzerRaytracingService::DescriptorHeapInfo info;
    info.Key = c.m_ppDescriptorHeaps.Keys[i];
    auto it = m_DescriptorHeapInfos.find(info.Key);
    GITS_ASSERT(it != m_DescriptorHeapInfos.end());
    info.Type = it->second.type;
    info.NumDescriptors = it->second.numDescriptors;
    infos.push_back(info);
  }
  m_RaytracingService.SetDescriptorHeaps(c.m_Object.Key, infos);

  CommandListInfo& commandListInfo = m_CommandListInfos[c.m_Object.Key];
  commandListInfo.viewDescriptorHeap = 0;
  commandListInfo.samplerDescriptorHeap = 0;
  for (unsigned key : c.m_ppDescriptorHeaps.Keys) {
    AddObjectForRestore(key);
    DescriptorHeapInfo& info = m_DescriptorHeapInfos[key];
    if (info.type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
      commandListInfo.viewDescriptorHeap = key;
    } else if (info.type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
      commandListInfo.samplerDescriptorHeap = key;
    }
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  m_CommandListInfos[c.m_Object.Key].computeRootSignature = c.m_pRootSignature.Key;
  AddObjectForRestore(c.m_pRootSignature.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  m_CommandListInfos[c.m_Object.Key].graphicsRootSignature = c.m_pRootSignature.Key;
  AddObjectForRestore(c.m_pRootSignature.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (!c.m_BaseDescriptor.Value.ptr) {
    return;
  }
  unsigned rootSignatureKey = m_CommandListInfos[c.m_Object.Key].computeRootSignature;
  GITS_ASSERT(rootSignatureKey);
  unsigned numDescriptors = m_DescriptorHeapInfos[c.m_BaseDescriptor.InterfaceKey].numDescriptors;
  GITS_ASSERT(numDescriptors);
  std::vector<unsigned> Indexes = m_RootSignatureService.GetDescriptorTableIndexes(
      rootSignatureKey, c.m_BaseDescriptor.InterfaceKey, c.m_RootParameterIndex.Value,
      c.m_BaseDescriptor.Index, numDescriptors);
  for (unsigned index : Indexes) {
    DescriptorState* state =
        m_DescriptorService.GetDescriptorState(c.m_BaseDescriptor.InterfaceKey, index);
    if (state) {
      AddObjectForRestore(state->ResourceKey);
      AddObjectForRestore(state->AuxiliaryResourceKey);
    }
    m_Descriptors.insert({c.m_BaseDescriptor.InterfaceKey, index});
  }
  AddObjectForRestore(c.m_BaseDescriptor.InterfaceKey);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  if (!c.m_BaseDescriptor.Value.ptr) {
    return;
  }
  unsigned rootSignatureKey = m_CommandListInfos[c.m_Object.Key].graphicsRootSignature;
  GITS_ASSERT(rootSignatureKey);
  unsigned numDescriptors = m_DescriptorHeapInfos[c.m_BaseDescriptor.InterfaceKey].numDescriptors;
  GITS_ASSERT(numDescriptors);
  std::vector<unsigned> Indexes = m_RootSignatureService.GetDescriptorTableIndexes(
      rootSignatureKey, c.m_BaseDescriptor.InterfaceKey, c.m_RootParameterIndex.Value,
      c.m_BaseDescriptor.Index, numDescriptors);
  for (unsigned index : Indexes) {
    DescriptorState* state =
        m_DescriptorService.GetDescriptorState(c.m_BaseDescriptor.InterfaceKey, index);
    if (state) {
      AddObjectForRestore(state->ResourceKey);
      AddObjectForRestore(state->AuxiliaryResourceKey);
    }
    m_Descriptors.insert({c.m_BaseDescriptor.InterfaceKey, index});
  }
  AddObjectForRestore(c.m_BaseDescriptor.InterfaceKey);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  AddObjectForRestore(c.m_BufferLocation.InterfaceKey);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  AddObjectForRestore(c.m_BufferLocation.InterfaceKey);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  AddObjectForRestore(c.m_BufferLocation.InterfaceKey);

  unsigned tlasBuildKey = m_RaytracingService.FindTlas(AnalyzerRaytracingService::KeyOffset(
      c.m_BufferLocation.InterfaceKey, c.m_BufferLocation.Offset));
  if (tlasBuildKey) {
    m_TlasBuildKeys.insert(tlasBuildKey);
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  AddObjectForRestore(c.m_BufferLocation.InterfaceKey);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  AddObjectForRestore(c.m_BufferLocation.InterfaceKey);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  AddObjectForRestore(c.m_BufferLocation.InterfaceKey);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  AddObjectForRestore(c.m_pView.BufferLocationKey);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  for (unsigned i = 0; i < c.m_pViews.Size; ++i) {
    AddObjectForRestore(c.m_pViews.BufferLocationKeys[i]);
  }
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  for (unsigned i = 0; i < c.m_pViews.Size; ++i) {
    AddObjectForRestore(c.m_pViews.BufferLocationKeys[i]);
    AddObjectForRestore(c.m_pViews.BufferFilledSizeLocationKeys[i]);
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (!c.m_RTsSingleHandleToDescriptorRange.Value) {
    for (unsigned i = 0; i < c.m_NumRenderTargetDescriptors.Value; ++i) {
      unsigned key = c.m_pRenderTargetDescriptors.InterfaceKeys[i];
      unsigned index = c.m_pRenderTargetDescriptors.Indexes[i];
      if (key) {
        AddObjectForRestore(key);
        DescriptorState* state = m_DescriptorService.GetDescriptorState(key, index);
        if (state) {
          AddObjectForRestore(state->ResourceKey);
          AddObjectForRestore(state->AuxiliaryResourceKey);
        }
        m_Descriptors.insert({key, index});
      }
    }
  } else if (c.m_NumRenderTargetDescriptors.Value) {
    unsigned key = c.m_pRenderTargetDescriptors.InterfaceKeys[0];
    unsigned index = c.m_pRenderTargetDescriptors.Indexes[0];
    if (key) {
      AddObjectForRestore(key);
      for (unsigned i = 0; i < c.m_NumRenderTargetDescriptors.Value; ++i) {
        DescriptorState* state = m_DescriptorService.GetDescriptorState(key, index);
        if (state) {
          AddObjectForRestore(state->ResourceKey);
          AddObjectForRestore(state->AuxiliaryResourceKey);
        }
        m_Descriptors.insert({key, index});
        ++index;
      }
    }
  }
  if (c.m_pDepthStencilDescriptor.Value) {
    unsigned key = c.m_pDepthStencilDescriptor.InterfaceKeys[0];
    unsigned index = c.m_pDepthStencilDescriptor.Indexes[0];
    if (key) {
      AddObjectForRestore(key);
      DescriptorState* state = m_DescriptorService.GetDescriptorState(key, index);
      if (state) {
        AddObjectForRestore(state->ResourceKey);
        AddObjectForRestore(state->AuxiliaryResourceKey);
      }
      m_Descriptors.insert({key, index});
    }
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (c.m_DepthStencilView.InterfaceKey) {
    AddObjectForRestore(c.m_DepthStencilView.InterfaceKey);
    DescriptorState* state = m_DescriptorService.GetDescriptorState(
        c.m_DepthStencilView.InterfaceKey, c.m_DepthStencilView.Index);
    if (state) {
      AddObjectForRestore(state->ResourceKey);
      AddObjectForRestore(state->AuxiliaryResourceKey);
    }
    m_Descriptors.insert({c.m_DepthStencilView.InterfaceKey, c.m_DepthStencilView.Index});
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (c.m_RenderTargetView.InterfaceKey) {
    AddObjectForRestore(c.m_RenderTargetView.InterfaceKey);
    DescriptorState* state = m_DescriptorService.GetDescriptorState(
        c.m_RenderTargetView.InterfaceKey, c.m_RenderTargetView.Index);
    if (state) {
      AddObjectForRestore(state->ResourceKey);
      AddObjectForRestore(state->AuxiliaryResourceKey);
    }
    m_Descriptors.insert({c.m_RenderTargetView.InterfaceKey, c.m_RenderTargetView.Index});
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (c.m_ViewGPUHandleInCurrentHeap.InterfaceKey) {
    AddObjectForRestore(c.m_ViewGPUHandleInCurrentHeap.InterfaceKey);
    DescriptorState* state = m_DescriptorService.GetDescriptorState(
        c.m_ViewGPUHandleInCurrentHeap.InterfaceKey, c.m_ViewGPUHandleInCurrentHeap.Index);
    if (state) {
      AddObjectForRestore(state->ResourceKey);
      AddObjectForRestore(state->AuxiliaryResourceKey);
    }
    m_Descriptors.insert(
        {c.m_ViewGPUHandleInCurrentHeap.InterfaceKey, c.m_ViewGPUHandleInCurrentHeap.Index});
  }
  if (c.m_ViewCPUHandle.InterfaceKey) {
    AddObjectForRestore(c.m_ViewCPUHandle.InterfaceKey);
    DescriptorState* state = m_DescriptorService.GetDescriptorState(c.m_ViewCPUHandle.InterfaceKey,
                                                                    c.m_ViewCPUHandle.Index);
    if (state) {
      AddObjectForRestore(state->ResourceKey);
      AddObjectForRestore(state->AuxiliaryResourceKey);
    }
    m_Descriptors.insert({c.m_ViewCPUHandle.InterfaceKey, c.m_ViewCPUHandle.Index});
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (c.m_ViewGPUHandleInCurrentHeap.InterfaceKey) {
    AddObjectForRestore(c.m_ViewGPUHandleInCurrentHeap.InterfaceKey);
    DescriptorState* state = m_DescriptorService.GetDescriptorState(
        c.m_ViewGPUHandleInCurrentHeap.InterfaceKey, c.m_ViewGPUHandleInCurrentHeap.Index);
    if (state) {
      AddObjectForRestore(state->ResourceKey);
      AddObjectForRestore(state->AuxiliaryResourceKey);
    }
    m_Descriptors.insert(
        {c.m_ViewGPUHandleInCurrentHeap.InterfaceKey, c.m_ViewGPUHandleInCurrentHeap.Index});
  }
  if (c.m_ViewCPUHandle.InterfaceKey) {
    AddObjectForRestore(c.m_ViewCPUHandle.InterfaceKey);
    DescriptorState* state = m_DescriptorService.GetDescriptorState(c.m_ViewCPUHandle.InterfaceKey,
                                                                    c.m_ViewCPUHandle.Index);
    if (state) {
      AddObjectForRestore(state->ResourceKey);
      AddObjectForRestore(state->AuxiliaryResourceKey);
    }
    m_Descriptors.insert({c.m_ViewCPUHandle.InterfaceKey, c.m_ViewCPUHandle.Index});
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListDiscardResourceCommand& c) {
  AddObjectForRestore(c.m_pResource.Key);
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandListBeginQueryCommand& c) {
  AddObjectForRestore(c.m_pQueryHeap.Key);
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandListEndQueryCommand& c) {
  AddObjectForRestore(c.m_pQueryHeap.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListResolveQueryDataCommand& c) {
  AddObjectForRestore(c.m_pQueryHeap.Key);
  AddObjectForRestore(c.m_pDestinationBuffer.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListSetPredicationCommand& c) {
  AddObjectForRestore(c.m_pBuffer.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandListExecuteIndirectCommand& c) {

  if (m_DispatchRaysCommandSignatures.find(c.m_pCommandSignature.Key) !=
      m_DispatchRaysCommandSignatures.end()) {
    m_DispatchRays = true;
  }

  m_ExecuteIndirectService.ExecuteIndirect(c);
  CommandListInfo& info = m_CommandListInfos[c.m_Object.Key];
  if (info.viewDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = m_DescriptorHeapInfos[info.viewDescriptorHeap];
    if (info.computeRootSignature) {
      SetBindlessDescriptors(info.computeRootSignature, info.viewDescriptorHeap, heapInfo.type,
                             heapInfo.numDescriptors);
    }
    if (info.graphicsRootSignature) {
      SetBindlessDescriptors(info.graphicsRootSignature, info.viewDescriptorHeap, heapInfo.type,
                             heapInfo.numDescriptors);
    }
  }
  if (info.samplerDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = m_DescriptorHeapInfos[info.samplerDescriptorHeap];
    if (info.computeRootSignature) {
      SetBindlessDescriptors(info.computeRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                             heapInfo.numDescriptors);
    }
    if (info.graphicsRootSignature) {
      SetBindlessDescriptors(info.graphicsRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                             heapInfo.numDescriptors);
    }
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand& c) {
  AddObjectForRestore(c.m_pDstBuffer.Key);
  AddObjectForRestore(c.m_pSrcBuffer.Key);
  for (unsigned key : c.m_ppDependentResources.Keys) {
    AddObjectForRestore(key);
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& c) {
  AddObjectForRestore(c.m_pDstBuffer.Key);
  AddObjectForRestore(c.m_pSrcBuffer.Key);
  for (unsigned key : c.m_ppDependentResources.Keys) {
    AddObjectForRestore(key);
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c) {
  AddObjectForRestore(c.m_pDstResource.Key);
  AddObjectForRestore(c.m_pSrcResource.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  for (unsigned key : c.m_pParams.DestKeys) {
    AddObjectForRestore(key);
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  AddObjectForRestore(c.m_pProtectedResourceSession.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList4BeginRenderPassCommand& c) {
  for (unsigned i = 0; i < c.m_pRenderTargets.Size; ++i) {
    unsigned DescriptorKey = c.m_pRenderTargets.DescriptorKeys[i];
    if (DescriptorKey) {
      AddObjectForRestore(DescriptorKey);
      DescriptorState* state = m_DescriptorService.GetDescriptorState(
          DescriptorKey, c.m_pRenderTargets.DescriptorIndexes[i]);
      if (state) {
        AddObjectForRestore(state->ResourceKey);
        AddObjectForRestore(state->AuxiliaryResourceKey);
      }
      m_Descriptors.insert({DescriptorKey, c.m_pRenderTargets.DescriptorIndexes[i]});
    }
  }
  for (unsigned key : c.m_pRenderTargets.ResolveSrcResourceKeys) {
    AddObjectForRestore(key);
  }
  for (unsigned key : c.m_pRenderTargets.ResolveDstResourceKeys) {
    AddObjectForRestore(key);
  }
  if (c.m_pDepthStencil.DescriptorKey) {
    AddObjectForRestore(c.m_pDepthStencil.DescriptorKey);
    DescriptorState* state = m_DescriptorService.GetDescriptorState(
        c.m_pDepthStencil.DescriptorKey, c.m_pDepthStencil.DescriptorIndex);
    if (state) {
      AddObjectForRestore(state->ResourceKey);
      AddObjectForRestore(state->AuxiliaryResourceKey);
    }
    m_Descriptors.insert({c.m_pDepthStencil.DescriptorKey, c.m_pDepthStencil.DescriptorIndex});
  }
  AddObjectForRestore(c.m_pDepthStencil.ResolveSrcDepthKey);
  AddObjectForRestore(c.m_pDepthStencil.ResolveDstDepthKey);
  AddObjectForRestore(c.m_pDepthStencil.ResolveSrcStencilKey);
  AddObjectForRestore(c.m_pDepthStencil.ResolveDstStencilKey);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c) {
  AddObjectForRestore(c.m_pMetaCommand.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& c) {
  AddObjectForRestore(c.m_pMetaCommand.Key);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    if (m_TlasBuildKeys.find(c.Key) == m_TlasBuildKeys.end()) {
      m_TlasBuildKeys.insert(c.Key);
      m_RaytracingService.BuildTlas(c);
    }
  } else if (c.m_pDesc.Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (c.m_pDesc.SourceAccelerationStructureKey) {
      m_RaytracingService.AddAccelerationStructureSource(
          c.m_pDesc.SourceAccelerationStructureKey, c.m_pDesc.SourceAccelerationStructureOffset);
    }
  }
}

void AnalyzerCommandListService::Command(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (m_AnalyzerService.InRange()) {
    if (!m_ResetCommandLists[c.m_Object.Key]) {
      CommandListRestore(c.m_Object.Key);
    }
    CommandAnalysis(c);
  } else if (!m_CommandListSubcapture) {
    m_CommandsByCommandList[c.m_Object.Key].emplace_back(
        new ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand(c));
  }
  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL &&
      m_AnalyzerService.BeforeRange() &&
      (m_FirstFrame || m_RestoreTlases || m_CommandListSubcapture)) {
    m_TlasBuildKeys.insert(c.Key);
    m_RaytracingService.BuildTlas(c);
  }

  if (m_Optimize) {
    AddObjectForRestore(c.m_pDesc.DestAccelerationStructureKey);
    if (c.m_pDesc.SourceAccelerationStructureKey) {
      AddObjectForRestore(c.m_pDesc.SourceAccelerationStructureKey);
    }
    if (!IsStateRestoreKey(c.m_pDesc.ScratchAccelerationStructureKey)) {
      AddObjectForRestore(c.m_pDesc.ScratchAccelerationStructureKey);
    }
    for (unsigned key : c.m_pDesc.InputKeys) {
      AddObjectForRestore(key);
    }
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (c.m_pParams.Value->pDesc->inputs.type ==
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    LOG_ERROR << "NvAPI top level build not handled";
  } else if (c.m_pParams.Value->pDesc->inputs.type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (c.m_pParams.SourceAccelerationStructureKey) {
      m_RaytracingService.AddAccelerationStructureSource(
          c.m_pParams.SourceAccelerationStructureKey,
          c.m_pParams.SourceAccelerationStructureOffset);
    }
  }
}

void AnalyzerCommandListService::Command(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (m_AnalyzerService.InRange()) {
    if (!m_ResetCommandLists[c.m_pCommandList.Key]) {
      CommandListRestore(c.m_pCommandList.Key);
    }
    CommandAnalysis(c);
  } else if (!m_CommandListSubcapture) {
    m_CommandsByCommandList[c.m_pCommandList.Key].emplace_back(
        new NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand(c));
  }
  if (c.m_pParams.Value->pDesc->inputs.type ==
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    LOG_ERROR << "NvAPI top level build not handled";
  }

  if (m_Optimize) {
    AddObjectForRestore(c.m_pParams.DestAccelerationStructureKey);
    if (c.m_pParams.SourceAccelerationStructureKey) {
      AddObjectForRestore(c.m_pParams.SourceAccelerationStructureKey);
    }
    if (!IsStateRestoreKey(c.m_pParams.ScratchAccelerationStructureKey)) {
      AddObjectForRestore(c.m_pParams.ScratchAccelerationStructureKey);
    }
    for (unsigned key : c.m_pParams.InputKeys) {
      AddObjectForRestore(key);
    }
    for (unsigned key : c.m_pParams.DestPostBuildBufferKeys) {
      AddObjectForRestore(key);
    }
  }
}

void AnalyzerCommandListService::Command(
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (m_Optimize) {
    AddObjectForRestore(c.m_pParams.DestOpacityMicromapArrayDataKey);
    if (c.m_pParams.InputBufferKey) {
      AddObjectForRestore(c.m_pParams.InputBufferKey);
    }
    if (c.m_pParams.PerOMMDescsKey) {
      AddObjectForRestore(c.m_pParams.PerOMMDescsKey);
    }
    if (!IsStateRestoreKey(c.m_pParams.ScratchOpacityMicromapArrayDataKey)) {
      AddObjectForRestore(c.m_pParams.ScratchOpacityMicromapArrayDataKey);
    }
    for (unsigned key : c.m_pParams.DestPostBuildBufferKeys) {
      AddObjectForRestore(key);
    }
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  for (unsigned i = 0; i < c.m_NumSourceAccelerationStructures.Value; ++i) {
    m_RaytracingService.AddAccelerationStructureSource(
        c.m_pSourceAccelerationStructureData.InterfaceKeys[i],
        c.m_pSourceAccelerationStructureData.Offsets[i]);
    if (m_Optimize) {
      AddObjectForRestore(c.m_pSourceAccelerationStructureData.InterfaceKeys[i]);
    }
  }
  if (m_Optimize) {
    AddObjectForRestore(c.m_pDesc.destBufferKey);
  }
}

void AnalyzerCommandListService::Command(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (m_AnalyzerService.InRange()) {
    if (!m_ResetCommandLists[c.m_Object.Key]) {
      CommandListRestore(c.m_Object.Key);
    }
    CommandAnalysis(c);
  } else if (!m_CommandListSubcapture) {
    m_CommandsByCommandList[c.m_Object.Key].emplace_back(
        new ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand(c));
  }

  if (m_Optimize) {
    AddObjectForRestore(c.m_DestAccelerationStructureData.InterfaceKey);
    AddObjectForRestore(c.m_SourceAccelerationStructureData.InterfaceKey);
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  m_RaytracingService.AddAccelerationStructureSource(
      c.m_SourceAccelerationStructureData.InterfaceKey, c.m_SourceAccelerationStructureData.Offset);
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  m_RaytracingService.SetPipelineState(c);

  AddObjectForRestore(c.m_Object.Key);
  AddObjectForRestore(c.m_pStateObject.Key);
  if (m_CheckedStateObjectSubobjects.find(c.m_pStateObject.Key) ==
      m_CheckedStateObjectSubobjects.end()) {
    const std::set<unsigned> subobjects =
        m_RaytracingService.GetStateObjectAllSubobjects(c.m_pStateObject.Key);
    for (unsigned key : subobjects) {
      AddObjectForRestore(key);
    }
    m_CheckedStateObjectSubobjects.insert(c.m_pStateObject.Key);
  }
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  m_DispatchRays = true;
  m_RaytracingService.DispatchRays(c);
  AddObjectForRestore(c.m_pDesc.RayGenerationShaderRecordKey);
  AddObjectForRestore(c.m_pDesc.MissShaderTableKey);
  AddObjectForRestore(c.m_pDesc.HitGroupTableKey);
  AddObjectForRestore(c.m_pDesc.CallableShaderTableKey);

  CommandListInfo& info = m_CommandListInfos[c.m_Object.Key];
  if (info.viewDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = m_DescriptorHeapInfos[info.viewDescriptorHeap];
    SetBindlessDescriptors(info.computeRootSignature, info.viewDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
  if (info.samplerDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = m_DescriptorHeapInfos[info.samplerDescriptorHeap];
    SetBindlessDescriptors(info.computeRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
}

void AnalyzerCommandListService::CommandAnalysis(
    ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& c) {
  AddObjectForRestore(c.m_shadingRateImage.Key);
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandList6DispatchMeshCommand& c) {
  CommandListInfo& info = m_CommandListInfos[c.m_Object.Key];
  if (info.viewDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = m_DescriptorHeapInfos[info.viewDescriptorHeap];
    SetBindlessDescriptors(info.computeRootSignature, info.viewDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
  if (info.samplerDescriptorHeap) {
    DescriptorHeapInfo& heapInfo = m_DescriptorHeapInfos[info.samplerDescriptorHeap];
    SetBindlessDescriptors(info.computeRootSignature, info.samplerDescriptorHeap, heapInfo.type,
                           heapInfo.numDescriptors);
  }
}

void AnalyzerCommandListService::CommandAnalysis(ID3D12GraphicsCommandList7BarrierCommand& c) {
  for (unsigned key : c.m_pBarrierGroups.ResourceKeys) {
    AddObjectForRestore(key);
  }
}

} // namespace DirectX
} // namespace gits
