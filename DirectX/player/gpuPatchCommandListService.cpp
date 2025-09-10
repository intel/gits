// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuPatchCommandListService.h"

namespace gits {
namespace DirectX {

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  commandLists_[c.object_.key].currentRootSignature = c.pRootSignature_.value;
  commandLists_[c.object_.key].currentRootArguments.clear();
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  auto* state = new SetComputeRootDescriptorTableState();
  state->id = c.getId();
  state->rootParameterIndex = c.RootParameterIndex_.value;
  state->baseDescriptor = c.BaseDescriptor_.value;

  commandLists_[c.object_.key].currentRootArguments[c.RootParameterIndex_.value].reset(state);
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) {
  SetComputeRoot32BitConstantState* state = new SetComputeRoot32BitConstantState();
  state->id = c.getId();
  state->rootParameterIndex = c.RootParameterIndex_.value;
  state->srcData = c.SrcData_.value;
  state->destOffsetIn32BitValues = c.DestOffsetIn32BitValues_.value;

  commandLists_[c.object_.key].currentRootArguments[c.RootParameterIndex_.value].reset(state);
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c) {
  SetComputeRoot32BitConstantsState* state = new SetComputeRoot32BitConstantsState();
  state->id = c.getId();
  state->rootParameterIndex = c.RootParameterIndex_.value;
  state->pSrcData.resize(c.Num32BitValuesToSet_.value);
  memcpy(state->pSrcData.data(), c.pSrcData_.value,
         c.Num32BitValuesToSet_.value * sizeof(unsigned));
  state->destOffsetIn32BitValues = c.DestOffsetIn32BitValues_.value;

  commandLists_[c.object_.key].currentRootArguments[c.RootParameterIndex_.value].reset(state);
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  SetViewState* state = new SetViewState();
  state->id = c.getId();
  state->rootParameterIndex = c.RootParameterIndex_.value;
  state->bufferLocation = c.BufferLocation_.value;

  commandLists_[c.object_.key].currentRootArguments[c.RootParameterIndex_.value].reset(state);
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  SetViewState* state = new SetViewState();
  state->id = c.getId();
  state->rootParameterIndex = c.RootParameterIndex_.value;
  state->bufferLocation = c.BufferLocation_.value;

  commandLists_[c.object_.key].currentRootArguments[c.RootParameterIndex_.value].reset(state);
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  SetViewState* state = new SetViewState();
  state->id = c.getId();
  state->rootParameterIndex = c.RootParameterIndex_.value;
  state->bufferLocation = c.BufferLocation_.value;

  commandLists_[c.object_.key].currentRootArguments[c.RootParameterIndex_.value].reset(state);
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  commandLists_[c.object_.key].currentPipelineState = {c.pStateObject_.value, true};
}

void GpuPatchCommandListService::storeCommand(ID3D12DeviceCreateCommandListCommand& c) {
  if (!c.pInitialState_.value) {
    return;
  }

  commandLists_[c.ppCommandList_.key].currentPipelineState = {c.pInitialState_.value, false};
}

void GpuPatchCommandListService::storeCommand(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  commandLists_[c.object_.key].currentPipelineState = {c.pPipelineState_.value, false};
}

void GpuPatchCommandListService::remove(unsigned commandListKey) {
  commandLists_.erase(commandListKey);
}

void GpuPatchCommandListService::reset(unsigned commandListKey, ID3D12PipelineState* initialState) {
  if (initialState) {
    commandLists_[commandListKey] = CommandListState{};
    commandLists_[commandListKey].currentPipelineState = {initialState, false};
  } else {
    commandLists_.erase(commandListKey);
  }
}

void GpuPatchCommandListService::restoreState(unsigned commandListKey,
                                              ID3D12GraphicsCommandList* commandList) {
  auto itCommandLists = commandLists_.find(commandListKey);
  if (itCommandLists == commandLists_.end()) {
    return;
  }

  const CommandListState& state = itCommandLists->second;

  if (state.currentPipelineState.object) {
    if (state.currentPipelineState.isStateObject) {
      static_cast<ID3D12GraphicsCommandList4*>(commandList)
          ->SetPipelineState1(static_cast<ID3D12StateObject*>(state.currentPipelineState.object));
    } else {
      commandList->SetPipelineState(
          static_cast<ID3D12PipelineState*>(state.currentPipelineState.object));
    }
  }

  if (state.currentRootSignature) {
    commandList->SetComputeRootSignature(state.currentRootSignature);
  }

  for (const auto& rootArgument : state.currentRootArguments) {
    switch (rootArgument.second->id) {
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTDESCRIPTORTABLE: {
      auto* state = static_cast<SetComputeRootDescriptorTableState*>(rootArgument.second.get());
      commandList->SetComputeRootDescriptorTable(state->rootParameterIndex, state->baseDescriptor);
      break;
    }
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOT32BITCONSTANT: {
      auto* state = static_cast<SetComputeRoot32BitConstantState*>(rootArgument.second.get());
      commandList->SetComputeRoot32BitConstant(state->rootParameterIndex, state->srcData,
                                               state->destOffsetIn32BitValues);
      break;
    }
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOT32BITCONSTANTS: {
      auto* state = static_cast<SetComputeRoot32BitConstantsState*>(rootArgument.second.get());
      commandList->SetComputeRoot32BitConstants(state->rootParameterIndex, state->pSrcData.size(),
                                                state->pSrcData.data(),
                                                state->destOffsetIn32BitValues);
      break;
    }
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSHADERRESOURCEVIEW: {
      auto* state = static_cast<SetViewState*>(rootArgument.second.get());
      commandList->SetComputeRootShaderResourceView(state->rootParameterIndex,
                                                    state->bufferLocation);
      break;
    }
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTCONSTANTBUFFERVIEW: {
      auto* state = static_cast<SetViewState*>(rootArgument.second.get());
      commandList->SetComputeRootConstantBufferView(state->rootParameterIndex,
                                                    state->bufferLocation);
      break;
    }
    case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTUNORDEREDACCESSVIEW: {
      auto* state = static_cast<SetViewState*>(rootArgument.second.get());
      commandList->SetComputeRootUnorderedAccessView(state->rootParameterIndex,
                                                     state->bufferLocation);
      break;
    }
    }
  }
}

} // namespace DirectX
} // namespace gits
