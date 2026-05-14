// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandListStateService.h"

#include "commandCopyFactory.h"
#include "log.h"

#include <utility>
#include <algorithm>
#include <memory>
#include <map>

namespace gits {
namespace DirectX {

CommandListStateService::CommandListStateService(unsigned commandListKey)
    : m_CommandListKey(commandListKey) {}

bool CommandListStateService::IsStateCommand(CommandId id) {
  switch (id) {
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_SETPIPELINESTATE1:
  case CommandId::ID_ID3D12DEVICE_CREATECOMMANDLIST:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETPIPELINESTATE:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETDESCRIPTORHEAPS:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSIGNATURE:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTDESCRIPTORTABLE:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOT32BITCONSTANT:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOT32BITCONSTANTS:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSHADERRESOURCEVIEW:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTCONSTANTBUFFERVIEW:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTUNORDEREDACCESSVIEW:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTSIGNATURE:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTDESCRIPTORTABLE:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOT32BITCONSTANT:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOT32BITCONSTANTS:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTSHADERRESOURCEVIEW:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTCONSTANTBUFFERVIEW:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTUNORDEREDACCESSVIEW:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_IASETPRIMITIVETOPOLOGY:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_IASETINDEXBUFFER:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_IASETVERTEXBUFFERS:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RSSETVIEWPORTS:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RSSETSCISSORRECTS:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETBLENDFACTOR:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETSTENCILREF:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_OMSETDEPTHBOUNDS:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_SETSAMPLEPOSITIONS:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_SETVIEWINSTANCEMASK:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST3_SETPROTECTEDRESOURCESESSION:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETPREDICATION:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST5_RSSETSHADINGRATE:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST5_RSSETSHADINGRATEIMAGE:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST8_OMSETFRONTANDBACKSTENCILREF:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST9_RSSETDEPTHBIAS:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST9_IASETINDEXBUFFERSTRIPCUTVALUE:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST10_SETPROGRAM:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLISTPREVIEW_SETWORKGRAPHMAXIMUMGPUINPUTRECORDS:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SOSETTARGETS:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESET:
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARSTATE:
    return true;
  default:
    return false;
  }
}

void CommandListStateService::StoreCommand(const Command& c) {
  switch (c.GetId()) {
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_SETPIPELINESTATE1:
    StoreCommand(static_cast<const ID3D12GraphicsCommandList4SetPipelineState1Command&>(c));
    break;
  case CommandId::ID_ID3D12DEVICE_CREATECOMMANDLIST:
    StoreCommand(static_cast<const ID3D12DeviceCreateCommandListCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETPIPELINESTATE:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListSetPipelineStateCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETDESCRIPTORHEAPS:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListSetDescriptorHeapsCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSIGNATURE:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListSetComputeRootSignatureCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTDESCRIPTORTABLE:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOT32BITCONSTANT:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOT32BITCONSTANTS:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSHADERRESOURCEVIEW:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTCONSTANTBUFFERVIEW:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTUNORDEREDACCESSVIEW:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTSIGNATURE:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTDESCRIPTORTABLE:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOT32BITCONSTANT:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOT32BITCONSTANTS:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTSHADERRESOURCEVIEW:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTCONSTANTBUFFERVIEW:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTUNORDEREDACCESSVIEW:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_IASETPRIMITIVETOPOLOGY:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_IASETINDEXBUFFER:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListIASetIndexBufferCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_IASETVERTEXBUFFERS:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListIASetVertexBuffersCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RSSETVIEWPORTS:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListRSSetViewportsCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RSSETSCISSORRECTS:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListRSSetScissorRectsCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETBLENDFACTOR:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListOMSetBlendFactorCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETSTENCILREF:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListOMSetStencilRefCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_OMSETDEPTHBOUNDS:
    StoreCommand(static_cast<const ID3D12GraphicsCommandList1OMSetDepthBoundsCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_SETSAMPLEPOSITIONS:
    StoreCommand(static_cast<const ID3D12GraphicsCommandList1SetSamplePositionsCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST1_SETVIEWINSTANCEMASK:
    StoreCommand(static_cast<const ID3D12GraphicsCommandList1SetViewInstanceMaskCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST3_SETPROTECTEDRESOURCESESSION:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETPREDICATION:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListSetPredicationCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST5_RSSETSHADINGRATE:
    StoreCommand(static_cast<const ID3D12GraphicsCommandList5RSSetShadingRateCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST5_RSSETSHADINGRATEIMAGE:
    StoreCommand(static_cast<const ID3D12GraphicsCommandList5RSSetShadingRateImageCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST8_OMSETFRONTANDBACKSTENCILREF:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandList8OMSetFrontAndBackStencilRefCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST9_RSSETDEPTHBIAS:
    StoreCommand(static_cast<const ID3D12GraphicsCommandList9RSSetDepthBiasCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST9_IASETINDEXBUFFERSTRIPCUTVALUE:
    StoreCommand(
        static_cast<const ID3D12GraphicsCommandList9IASetIndexBufferStripCutValueCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST10_SETPROGRAM:
    StoreCommand(static_cast<const ID3D12GraphicsCommandList10SetProgramCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLISTPREVIEW_SETWORKGRAPHMAXIMUMGPUINPUTRECORDS:
    StoreCommand(
        static_cast<
            const ID3D12GraphicsCommandListPreviewSetWorkGraphMaximumGPUInputRecordsCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListOMSetRenderTargetsCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SOSETTARGETS:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListSOSetTargetsCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_RESET:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListResetCommand&>(c));
    break;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARSTATE:
    StoreCommand(static_cast<const ID3D12GraphicsCommandListClearStateCommand&>(c));
    break;
  default:
    break;
  }
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->ComputeRootSignature.emplace(c);
  m_State->ComputeRootArguments.clear();
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->GraphicsRootSignature.emplace(c);
  m_State->GraphicsRootArguments.clear();
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->ComputeRootArguments[c.m_RootParameterIndex.Value] = CreateCommandCopy(&c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->GraphicsRootArguments[c.m_RootParameterIndex.Value] = CreateCommandCopy(&c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->ComputeRootArguments[c.m_RootParameterIndex.Value] = CreateCommandCopy(&c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->GraphicsRootArguments[c.m_RootParameterIndex.Value] = CreateCommandCopy(&c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->ComputeRootArguments[c.m_RootParameterIndex.Value] = CreateCommandCopy(&c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->GraphicsRootArguments[c.m_RootParameterIndex.Value] = CreateCommandCopy(&c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->ComputeRootArguments[c.m_RootParameterIndex.Value] = CreateCommandCopy(&c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->GraphicsRootArguments[c.m_RootParameterIndex.Value] = CreateCommandCopy(&c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->ComputeRootArguments[c.m_RootParameterIndex.Value] = CreateCommandCopy(&c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->GraphicsRootArguments[c.m_RootParameterIndex.Value] = CreateCommandCopy(&c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->ComputeRootArguments[c.m_RootParameterIndex.Value] = CreateCommandCopy(&c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->GraphicsRootArguments[c.m_RootParameterIndex.Value] = CreateCommandCopy(&c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->PipelineStatePso.reset();
  m_State->PipelineStateStateObject.emplace(c);
}

void CommandListStateService::StoreCommand(const ID3D12DeviceCreateCommandListCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  SetPipelineState(c.m_pInitialState.Key);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  SetPipelineState(c.m_pPipelineState.Key);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  const auto sameDescriptorHeapSet = [](const std::vector<unsigned>& a,
                                        const std::vector<unsigned>& b) {
    if (a.size() != b.size()) {
      return false;
    }
    std::vector<unsigned> sortedA = a;
    std::vector<unsigned> sortedB = b;
    std::sort(sortedA.begin(), sortedA.end());
    std::sort(sortedB.begin(), sortedB.end());
    return sortedA == sortedB;
  };
  if (!m_State->DescriptorHeaps.has_value() ||
      !sameDescriptorHeapSet(m_State->DescriptorHeaps.value().m_ppDescriptorHeaps.Keys,
                             c.m_ppDescriptorHeaps.Keys)) {
    auto removeArguments = [](std::map<unsigned, std::unique_ptr<Command>>& arguments,
                              const CommandId commandId) {
      std::erase_if(arguments, [commandId](const auto& command) {
        return command.second->GetId() == commandId;
      });
    };

    removeArguments(m_State->GraphicsRootArguments,
                    CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETGRAPHICSROOTDESCRIPTORTABLE);
    removeArguments(m_State->ComputeRootArguments,
                    CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTDESCRIPTORTABLE);
  }
  m_State->DescriptorHeaps.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->PrimitiveTopology.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->IndexBuffer.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  auto& vertexBuffers = m_State->VertexBuffers;
  const auto startSlot = c.m_StartSlot.Value;
  const auto numViews = c.m_NumViews.Value;
  vertexBuffers.erase(std::remove_if(vertexBuffers.begin(), vertexBuffers.end(),
                                     [startSlot, numViews](const auto& vb) {
                                       return startSlot <= vb->m_StartSlot.Value &&
                                              startSlot + numViews >=
                                                  vb->m_StartSlot.Value + vb->m_NumViews.Value;
                                     }),
                      vertexBuffers.end());
  vertexBuffers.emplace_back(
      std::make_unique<ID3D12GraphicsCommandListIASetVertexBuffersCommand>(c));
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListRSSetViewportsCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->Viewports.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListRSSetScissorRectsCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->ScissorRects.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListOMSetBlendFactorCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->BlendFactor.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListOMSetStencilRefCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->StencilRef.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandList1OMSetDepthBoundsCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->DepthBounds.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandList1SetSamplePositionsCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->SamplePositions.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandList1SetViewInstanceMaskCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->ViewInstanceMask.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->ProtectedResourceSession.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListSetPredicationCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->Predication.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandList5RSSetShadingRateCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->ShadingRate.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->ShadingRateImage.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandList8OMSetFrontAndBackStencilRefCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->FrontBackStencilRef.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandList9RSSetDepthBiasCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->DepthBias.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandList9IASetIndexBufferStripCutValueCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->IndexBufferStripCutValue.emplace(c);
}

void CommandListStateService::StoreCommand(const ID3D12GraphicsCommandList10SetProgramCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->SetProgram.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListPreviewSetWorkGraphMaximumGPUInputRecordsCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->WorkGraphGpuInputLimits.emplace(c);
}

void CommandListStateService::StoreCommand(
    const ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State->RenderTargets.emplace(c);
}

void CommandListStateService::StoreCommand(const ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  auto& streamOutput = m_State->StreamOutput;
  const auto startSlot = c.m_StartSlot.Value;
  const auto numViews = c.m_NumViews.Value;
  streamOutput.erase(std::remove_if(streamOutput.begin(), streamOutput.end(),
                                    [startSlot, numViews](const auto& so) {
                                      return startSlot <= so->m_StartSlot.Value &&
                                             startSlot + numViews >=
                                                 so->m_StartSlot.Value + so->m_NumViews.Value;
                                    }),
                     streamOutput.end());
  streamOutput.emplace_back(std::make_unique<ID3D12GraphicsCommandListSOSetTargetsCommand>(c));
}

void CommandListStateService::StoreCommand(const ID3D12GraphicsCommandListClearStateCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);

  std::optional<ID3D12GraphicsCommandListSetDescriptorHeapsCommand> preservedDescriptorHeaps =
      std::move(m_State->DescriptorHeaps);
  std::optional<ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand>
      preservedProtectedResourceSession = std::move(m_State->ProtectedResourceSession);

  m_State = std::make_unique<CommandListState>();

  if (preservedDescriptorHeaps) {
    m_State->DescriptorHeaps.emplace(std::move(*preservedDescriptorHeaps));
  }
  if (preservedProtectedResourceSession) {
    m_State->ProtectedResourceSession.emplace(std::move(*preservedProtectedResourceSession));
  }

  SetPipelineState(c.m_pPipelineState.Key);
}

void CommandListStateService::StoreCommand(const ID3D12GraphicsCommandListResetCommand& c) {
  GITS_ASSERT(c.m_Object.Key == m_CommandListKey);
  m_State = std::make_unique<CommandListState>();
  SetPipelineState(c.m_pInitialState.Key);
}

void CommandListStateService::SetPipelineState(unsigned pipelineStateKey) {
  if (!pipelineStateKey) {
    return;
  }

  ID3D12GraphicsCommandListSetPipelineStateCommand command(0, nullptr, nullptr);
  command.m_Object.Key = m_CommandListKey;
  command.m_pPipelineState.Key = pipelineStateKey;
  m_State->PipelineStateStateObject.reset();
  m_State->PipelineStatePso.emplace(std::move(command));
}

void CommandListStateService::AppendRootCommands(
    const std::map<unsigned, std::unique_ptr<Command>>& args,
    std::vector<std::unique_ptr<Command>>& out) const {
  for (const auto& rootArgument : args) {
    auto command = CreateCommandCopy(rootArgument.second.get());
    out.push_back(std::move(command));
  }
}

std::vector<std::unique_ptr<Command>> CommandListStateService::RestoreState() const {
  std::vector<std::unique_ptr<Command>> out;
  const CommandListState& state = *m_State;

  if (state.PipelineStateStateObject) {
    out.push_back(CreateCommandCopy(&state.PipelineStateStateObject.value()));
  } else if (state.PipelineStatePso) {
    out.push_back(CreateCommandCopy(&state.PipelineStatePso.value()));
  }

  if (state.ProtectedResourceSession) {
    out.push_back(CreateCommandCopy(&state.ProtectedResourceSession.value()));
  }

  if (state.Predication) {
    out.push_back(CreateCommandCopy(&state.Predication.value()));
  }

  if (state.SetProgram) {
    out.push_back(CreateCommandCopy(&state.SetProgram.value()));
  }

  if (state.WorkGraphGpuInputLimits) {
    out.push_back(CreateCommandCopy(&state.WorkGraphGpuInputLimits.value()));
  }

  if (state.IndexBufferStripCutValue) {
    out.push_back(CreateCommandCopy(&state.IndexBufferStripCutValue.value()));
  }

  if (state.PrimitiveTopology) {
    out.push_back(CreateCommandCopy(&state.PrimitiveTopology.value()));
  }

  if (state.IndexBuffer) {
    out.push_back(CreateCommandCopy(&state.IndexBuffer.value()));
  }

  for (const auto& command : state.VertexBuffers) {
    out.push_back(CreateCommandCopy(command.get()));
  }

  if (state.Viewports) {
    out.push_back(CreateCommandCopy(&state.Viewports.value()));
  }

  if (state.ScissorRects) {
    out.push_back(CreateCommandCopy(&state.ScissorRects.value()));
  }

  if (state.ViewInstanceMask) {
    out.push_back(CreateCommandCopy(&state.ViewInstanceMask.value()));
  }

  if (state.ShadingRate) {
    out.push_back(CreateCommandCopy(&state.ShadingRate.value()));
  }

  if (state.ShadingRateImage) {
    out.push_back(CreateCommandCopy(&state.ShadingRateImage.value()));
  }

  if (state.DepthBias) {
    out.push_back(CreateCommandCopy(&state.DepthBias.value()));
  }

  if (state.BlendFactor) {
    out.push_back(CreateCommandCopy(&state.BlendFactor.value()));
  }

  if (state.StencilRef) {
    out.push_back(CreateCommandCopy(&state.StencilRef.value()));
  }

  if (state.FrontBackStencilRef) {
    out.push_back(CreateCommandCopy(&state.FrontBackStencilRef.value()));
  }

  if (state.DepthBounds) {
    out.push_back(CreateCommandCopy(&state.DepthBounds.value()));
  }

  if (state.SamplePositions) {
    out.push_back(CreateCommandCopy(&state.SamplePositions.value()));
  }

  if (state.RenderTargets) {
    out.push_back(CreateCommandCopy(&state.RenderTargets.value()));
  }

  for (const auto& command : state.StreamOutput) {
    out.push_back(CreateCommandCopy(command.get()));
  }

  if (state.DescriptorHeaps) {
    out.push_back(CreateCommandCopy(&state.DescriptorHeaps.value()));
  }

  if (state.GraphicsRootSignature) {
    out.push_back(CreateCommandCopy(&state.GraphicsRootSignature.value()));
    AppendRootCommands(state.GraphicsRootArguments, out);
  }

  if (state.ComputeRootSignature) {
    out.push_back(CreateCommandCopy(&state.ComputeRootSignature.value()));
    AppendRootCommands(state.ComputeRootArguments, out);
  }

  return out;
}

} // namespace DirectX
} // namespace gits
