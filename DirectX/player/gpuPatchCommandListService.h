// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"

#include <d3d12.h>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace gits {
namespace DirectX {

class GpuPatchCommandListService {
public:
  void StoreCommand(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c);
  void StoreCommand(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c);
  void StoreCommand(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c);
  void StoreCommand(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c);
  void StoreCommand(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c);
  void StoreCommand(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c);
  void StoreCommand(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c);
  void StoreCommand(ID3D12GraphicsCommandList4SetPipelineState1Command& c);
  void StoreCommand(ID3D12DeviceCreateCommandListCommand& c);
  void StoreCommand(ID3D12GraphicsCommandListSetPipelineStateCommand& c);
  void Remove(unsigned commandListKey);
  void Reset(unsigned commandListKey, ID3D12PipelineState* initialState);
  void RestoreState(unsigned commandListKey, ID3D12GraphicsCommandList* commandList);

private:
  struct CommandState {
    virtual ~CommandState() = default;
    CommandId Id{};
  };
  struct SetComputeRootDescriptorTableState : public CommandState {
    unsigned RootParameterIndex{};
    D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor{};
  };
  struct SetViewState : public CommandState {
    unsigned RootParameterIndex{};
    D3D12_GPU_VIRTUAL_ADDRESS BufferLocation{};
  };
  struct SetComputeRoot32BitConstantState : public CommandState {
    unsigned RootParameterIndex{};
    unsigned SrcData{};
    unsigned DestOffsetIn32BitValues{};
  };
  struct SetComputeRoot32BitConstantsState : public CommandState {
    unsigned RootParameterIndex{};
    std::vector<unsigned> SrcData;
    unsigned DestOffsetIn32BitValues{};
  };

  struct CommandListState {
    struct PipelineState {
      ID3D12Pageable* Object{};
      bool IsStateObject{};
    };
    PipelineState CurrentPipelineState{};
    ID3D12RootSignature* CurrentRootSignature{};
    std::map<unsigned, std::unique_ptr<CommandState>> CurrentRootArguments;
  };

  std::unordered_map<unsigned, CommandListState> m_CommandLists;
};

} // namespace DirectX
} // namespace gits
