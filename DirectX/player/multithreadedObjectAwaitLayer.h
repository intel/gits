// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "playerManager.h"

namespace gits {
namespace DirectX {

class PlayerManager;

class MultithreadedObjectAwaitLayer : public Layer {
public:
  MultithreadedObjectAwaitLayer(PlayerManager& manager);

  void Pre(IUnknownAddRefCommand& command) override;
  void Pre(IUnknownReleaseCommand& command) override;
  void Pre(IUnknownQueryInterfaceCommand& command) override;
  void Pre(ID3D12ObjectGetPrivateDataCommand& command) override;
  void Pre(ID3D12ObjectSetNameCommand& command) override;
  void Pre(ID3D12ObjectSetPrivateDataCommand& command) override;
  void Pre(ID3D12ObjectSetPrivateDataInterfaceCommand& command) override;
  void Pre(ID3D12DeviceChildGetDeviceCommand& command) override;
  void Pre(ID3D12PipelineStateGetCachedBlobCommand& command) override;
  void Pre(ID3D12DeviceCreateCommandListCommand& command) override;
  void Pre(ID3D12GraphicsCommandListResetCommand& command) override;
  void Pre(ID3D12GraphicsCommandListClearStateCommand& command) override;
  void Pre(ID3D12GraphicsCommandListSetPipelineStateCommand& command) override;
  void Pre(ID3D12GraphicsCommandList4SetPipelineState1Command& command) override;
  void Pre(ID3D12DeviceMakeResidentCommand& command) override;
  void Pre(ID3D12DeviceEvictCommand& command) override;
  void Pre(ID3D12Device1SetResidencyPriorityCommand& command) override;
  void Pre(ID3D12Device3EnqueueMakeResidentCommand& command) override;
  void Pre(ID3D12Device5CreateStateObjectCommand& command) override;
  void Pre(ID3D12Device7AddToStateObjectCommand& command) override;
  void Pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& command) override;
  void Pre(ID3D12PipelineLibraryLoadComputePipelineCommand& command) override;
  void Pre(ID3D12PipelineLibrary1LoadPipelineCommand& command) override;

private:
  std::optional<MultithreadedObjectCreationService::ObjectCreationOutput> CollectResult(
      unsigned objectKey);
  bool CompleteObject(unsigned key);
  template <typename T>
  void CompleteArgument(InterfaceArgument<T>& commandObject);
  template <typename T>
  void CompleteArrayArgument(InterfaceArrayArgument<T>& commandObjects);

private:
  PlayerManager& m_Manager;
  std::unordered_map<unsigned, MultithreadedObjectCreationService::ObjectCreationOutput>
      m_PreCollectedOutputs;
};

} // namespace DirectX
} // namespace gits
