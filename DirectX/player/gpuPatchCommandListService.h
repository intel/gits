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
#include <vector>
#include <d3d12.h>
#include <memory>

namespace gits {
namespace DirectX {

class GpuPatchCommandListService {
public:
  void storeCommand(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c);
  void storeCommand(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c);
  void storeCommand(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c);
  void storeCommand(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c);
  void storeCommand(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c);
  void storeCommand(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c);
  void storeCommand(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c);
  void storeCommand(ID3D12GraphicsCommandList4SetPipelineState1Command& c);
  void storeCommand(ID3D12DeviceCreateCommandListCommand& c);
  void storeCommand(ID3D12GraphicsCommandListSetPipelineStateCommand& c);
  void remove(unsigned commandListKey);
  void reset(unsigned commandListKey);
  void restoreState(unsigned commandListKey, ID3D12GraphicsCommandList* commandList);

private:
  struct CommandState {
    virtual ~CommandState() {}
    CommandId id;
  };
  struct SetComputeRootSignatureState : public CommandState {
    ID3D12RootSignature* rootSignature;
  };
  struct SetPipelineState1State : public CommandState {
    ID3D12StateObject* stateObject;
  };
  struct SetComputeRootDescriptorTableState : public CommandState {
    unsigned rootParameterIndex;
    D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor;
  };
  struct SetViewState : public CommandState {
    unsigned rootParameterIndex;
    D3D12_GPU_VIRTUAL_ADDRESS bufferLocation;
  };
  struct SetComputeRoot32BitConstantState : public CommandState {
    unsigned rootParameterIndex;
    unsigned srcData;
    unsigned destOffsetIn32BitValues;
  };
  struct SetComputeRoot32BitConstantsState : public CommandState {
    unsigned rootParameterIndex;
    std::vector<unsigned> pSrcData;
    unsigned destOffsetIn32BitValues;
  };
  struct SetPipelineStateState : public CommandState {
    ID3D12PipelineState* pipelineState;
  };

private:
  std::unordered_map<unsigned, std::vector<std::unique_ptr<CommandState>>> commandLists_;
  std::unordered_map<unsigned, std::vector<std::unique_ptr<CommandState>>> commandListsCreated_;
};

} // namespace DirectX
} // namespace gits
