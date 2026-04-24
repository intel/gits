// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuPatchCommandListService.h"

namespace gits {
namespace DirectX {

void GpuPatchCommandListService::StoreCommand(
    ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  m_CommandLists[c.m_Object.Key].CurrentRootSignature = c.m_pRootSignature.Value;
  m_CommandLists[c.m_Object.Key].CurrentRootArguments.clear();
}

void GpuPatchCommandListService::StoreCommand(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  auto* state = new SetComputeRootDescriptorTableState();
  state->Id = c.GetId();
  state->RootParameterIndex = c.m_RootParameterIndex.Value;
  state->BaseDescriptor = c.m_BaseDescriptor.Value;

  m_CommandLists[c.m_Object.Key].CurrentRootArguments[c.m_RootParameterIndex.Value].reset(state);
}

void GpuPatchCommandListService::StoreCommand(
    ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) {
  SetComputeRoot32BitConstantState* state = new SetComputeRoot32BitConstantState();
  state->Id = c.GetId();
  state->RootParameterIndex = c.m_RootParameterIndex.Value;
  state->SrcData = c.m_SrcData.Value;
  state->DestOffsetIn32BitValues = c.m_DestOffsetIn32BitValues.Value;

  m_CommandLists[c.m_Object.Key].CurrentRootArguments[c.m_RootParameterIndex.Value].reset(state);
}

void GpuPatchCommandListService::StoreCommand(
    ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c) {
  SetComputeRoot32BitConstantsState* state = new SetComputeRoot32BitConstantsState();
  state->Id = c.GetId();
  state->RootParameterIndex = c.m_RootParameterIndex.Value;
  state->SrcData.resize(c.m_Num32BitValuesToSet.Value);
  memcpy(state->SrcData.data(), c.m_pSrcData.Value,
         c.m_Num32BitValuesToSet.Value * sizeof(unsigned));
  state->DestOffsetIn32BitValues = c.m_DestOffsetIn32BitValues.Value;

  m_CommandLists[c.m_Object.Key].CurrentRootArguments[c.m_RootParameterIndex.Value].reset(state);
}

void GpuPatchCommandListService::StoreCommand(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  SetViewState* state = new SetViewState();
  state->Id = c.GetId();
  state->RootParameterIndex = c.m_RootParameterIndex.Value;
  state->BufferLocation = c.m_BufferLocation.Value;

  m_CommandLists[c.m_Object.Key].CurrentRootArguments[c.m_RootParameterIndex.Value].reset(state);
}

void GpuPatchCommandListService::StoreCommand(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  SetViewState* state = new SetViewState();
  state->Id = c.GetId();
  state->RootParameterIndex = c.m_RootParameterIndex.Value;
  state->BufferLocation = c.m_BufferLocation.Value;

  m_CommandLists[c.m_Object.Key].CurrentRootArguments[c.m_RootParameterIndex.Value].reset(state);
}

void GpuPatchCommandListService::StoreCommand(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  SetViewState* state = new SetViewState();
  state->Id = c.GetId();
  state->RootParameterIndex = c.m_RootParameterIndex.Value;
  state->BufferLocation = c.m_BufferLocation.Value;

  m_CommandLists[c.m_Object.Key].CurrentRootArguments[c.m_RootParameterIndex.Value].reset(state);
}

void GpuPatchCommandListService::StoreCommand(
    ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  m_CommandLists[c.m_Object.Key].CurrentPipelineState = {c.m_pStateObject.Value, true};
}

void GpuPatchCommandListService::StoreCommand(ID3D12DeviceCreateCommandListCommand& c) {
  if (!c.m_pInitialState.Value) {
    return;
  }

  m_CommandLists[c.m_ppCommandList.Key].CurrentPipelineState = {c.m_pInitialState.Value, false};
}

void GpuPatchCommandListService::StoreCommand(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  m_CommandLists[c.m_Object.Key].CurrentPipelineState = {c.m_pPipelineState.Value, false};
}

void GpuPatchCommandListService::Remove(unsigned commandListKey) {
  m_CommandLists.erase(commandListKey);
}

void GpuPatchCommandListService::Reset(unsigned commandListKey, ID3D12PipelineState* initialState) {
  if (initialState) {
    m_CommandLists[commandListKey] = CommandListState{};
    m_CommandLists[commandListKey].CurrentPipelineState = {initialState, false};
  } else {
    m_CommandLists.erase(commandListKey);
  }
}

void GpuPatchCommandListService::RestoreState(unsigned commandListKey,
                                              ID3D12GraphicsCommandList* commandList) {
  auto itCommandLists = m_CommandLists.find(commandListKey);
  if (itCommandLists == m_CommandLists.end()) {
    return;
  }

  const CommandListState& state = itCommandLists->second;

  if (state.CurrentPipelineState.Object) {
    if (state.CurrentPipelineState.IsStateObject) {
      static_cast<ID3D12GraphicsCommandList4*>(commandList)
          ->SetPipelineState1(static_cast<ID3D12StateObject*>(state.CurrentPipelineState.Object));
    } else {
      commandList->SetPipelineState(
          static_cast<ID3D12PipelineState*>(state.CurrentPipelineState.Object));
    }
  }

  if (state.CurrentRootSignature) {
    commandList->SetComputeRootSignature(state.CurrentRootSignature);
  }

  for (const auto& rootArgument : state.CurrentRootArguments) {
    switch (rootArgument.second->Id) {
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTDESCRIPTORTABLE: {
      auto* state = static_cast<SetComputeRootDescriptorTableState*>(rootArgument.second.get());
      commandList->SetComputeRootDescriptorTable(state->RootParameterIndex, state->BaseDescriptor);
      break;
    }
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOT32BITCONSTANT: {
      auto* state = static_cast<SetComputeRoot32BitConstantState*>(rootArgument.second.get());
      commandList->SetComputeRoot32BitConstant(state->RootParameterIndex, state->SrcData,
                                               state->DestOffsetIn32BitValues);
      break;
    }
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOT32BITCONSTANTS: {
      auto* state = static_cast<SetComputeRoot32BitConstantsState*>(rootArgument.second.get());
      commandList->SetComputeRoot32BitConstants(state->RootParameterIndex, state->SrcData.size(),
                                                state->SrcData.data(),
                                                state->DestOffsetIn32BitValues);
      break;
    }
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSHADERRESOURCEVIEW: {
      auto* state = static_cast<SetViewState*>(rootArgument.second.get());
      commandList->SetComputeRootShaderResourceView(state->RootParameterIndex,
                                                    state->BufferLocation);
      break;
    }
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTCONSTANTBUFFERVIEW: {
      auto* state = static_cast<SetViewState*>(rootArgument.second.get());
      commandList->SetComputeRootConstantBufferView(state->RootParameterIndex,
                                                    state->BufferLocation);
      break;
    }
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTUNORDEREDACCESSVIEW: {
      auto* state = static_cast<SetViewState*>(rootArgument.second.get());
      commandList->SetComputeRootUnorderedAccessView(state->RootParameterIndex,
                                                     state->BufferLocation);
      break;
    }
    }
  }
}

} // namespace DirectX
} // namespace gits
