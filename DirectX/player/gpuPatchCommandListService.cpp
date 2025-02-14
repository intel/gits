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

  commandLists_.erase(c.object_.key);

  auto* state = new SetComputeRootSignatureState();
  state->id = c.getId();
  state->rootSignature = c.pRootSignature_.value;

  commandLists_[c.object_.key].emplace_back(state);
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  auto* state = new SetComputeRootDescriptorTableState();
  state->id = c.getId();
  state->rootParameterIndex = c.RootParameterIndex_.value;
  state->baseDescriptor = c.BaseDescriptor_.value;

  commandLists_[c.object_.key].emplace_back(state);
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) {
  SetComputeRoot32BitConstantState* state = new SetComputeRoot32BitConstantState();
  state->id = c.getId();
  state->rootParameterIndex = c.RootParameterIndex_.value;
  state->srcData = c.SrcData_.value;
  state->destOffsetIn32BitValues = c.DestOffsetIn32BitValues_.value;

  commandLists_[c.object_.key].emplace_back(state);
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

  commandLists_[c.object_.key].emplace_back(state);
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  SetViewState* state = new SetViewState();
  state->id = c.getId();
  state->rootParameterIndex = c.RootParameterIndex_.value;
  state->bufferLocation = c.BufferLocation_.value;

  commandLists_[c.object_.key].emplace_back(state);
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  SetViewState* state = new SetViewState();
  state->id = c.getId();
  state->rootParameterIndex = c.RootParameterIndex_.value;
  state->bufferLocation = c.BufferLocation_.value;

  commandLists_[c.object_.key].emplace_back(state);
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  SetViewState* state = new SetViewState();
  state->id = c.getId();
  state->rootParameterIndex = c.RootParameterIndex_.value;
  state->bufferLocation = c.BufferLocation_.value;

  commandLists_[c.object_.key].emplace_back(state);
}

void GpuPatchCommandListService::storeCommand(
    ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  SetPipelineState1State* state = new SetPipelineState1State();
  state->id = c.getId();
  state->stateObject = c.pStateObject_.value;

  commandLists_[c.object_.key].emplace_back(state);
}

void GpuPatchCommandListService::storeCommand(ID3D12DeviceCreateCommandListCommand& c) {
  if (!c.pInitialState_.value) {
    return;
  }
  SetPipelineStateState* state = new SetPipelineStateState();
  state->id = c.getId();
  state->pipelineState = c.pInitialState_.value;

  commandListsCreated_[c.ppCommandList_.key].emplace_back(state);
}

void GpuPatchCommandListService::storeCommand(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  SetPipelineStateState* state = new SetPipelineStateState();
  state->id = c.getId();
  state->pipelineState = c.pPipelineState_.value;

  commandLists_[c.object_.key].emplace_back(state);
}

void GpuPatchCommandListService::remove(unsigned commandListKey) {
  commandLists_.erase(commandListKey);
  commandListsCreated_.erase(commandListKey);
}

void GpuPatchCommandListService::reset(unsigned commandListKey) {
  commandLists_.erase(commandListKey);
}

void GpuPatchCommandListService::restoreState(unsigned commandListKey,
                                              ID3D12GraphicsCommandList* commandList) {
  auto itCommandListsCreated = commandListsCreated_.find(commandListKey);
  if (itCommandListsCreated != commandListsCreated_.end()) {
    for (auto& it : itCommandListsCreated->second) {
      auto* state = static_cast<SetPipelineStateState*>(it.get());
      commandList->SetPipelineState(state->pipelineState);
    }
  }

  auto itCommandLists = commandLists_.find(commandListKey);
  if (itCommandLists != commandLists_.end()) {
    for (auto& it : itCommandLists->second) {
      switch (it->id) {
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSIGNATURE: {
        auto* state = static_cast<SetComputeRootSignatureState*>(it.get());
        commandList->SetComputeRootSignature(state->rootSignature);
        break;
      }
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_SETPIPELINESTATE1: {
        auto* state = static_cast<SetPipelineState1State*>(it.get());
        static_cast<ID3D12GraphicsCommandList4*>(commandList)
            ->SetPipelineState1(state->stateObject);
        break;
      }
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTDESCRIPTORTABLE: {
        auto* state = static_cast<SetComputeRootDescriptorTableState*>(it.get());
        commandList->SetComputeRootDescriptorTable(state->rootParameterIndex,
                                                   state->baseDescriptor);
        break;
      }
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOT32BITCONSTANT: {
        auto* state = static_cast<SetComputeRoot32BitConstantState*>(it.get());
        commandList->SetComputeRoot32BitConstant(state->rootParameterIndex, state->srcData,
                                                 state->destOffsetIn32BitValues);
        break;
      }
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOT32BITCONSTANTS: {
        auto* state = static_cast<SetComputeRoot32BitConstantsState*>(it.get());
        commandList->SetComputeRoot32BitConstants(state->rootParameterIndex, state->pSrcData.size(),
                                                  state->pSrcData.data(),
                                                  state->destOffsetIn32BitValues);
        break;
      }
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTSHADERRESOURCEVIEW: {
        auto* state = static_cast<SetViewState*>(it.get());
        commandList->SetComputeRootShaderResourceView(state->rootParameterIndex,
                                                      state->bufferLocation);
        break;
      }
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTCONSTANTBUFFERVIEW: {
        auto* state = static_cast<SetViewState*>(it.get());
        commandList->SetComputeRootConstantBufferView(state->rootParameterIndex,
                                                      state->bufferLocation);
        break;
      }
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETCOMPUTEROOTUNORDEREDACCESSVIEW: {
        auto* state = static_cast<SetViewState*>(it.get());
        commandList->SetComputeRootUnorderedAccessView(state->rootParameterIndex,
                                                       state->bufferLocation);
        break;
      }
      case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_SETPIPELINESTATE: {
        auto* state = static_cast<SetPipelineStateState*>(it.get());
        commandList->SetPipelineState(state->pipelineState);
        break;
      }
      }
    }
  }
}

} // namespace DirectX
} // namespace gits
