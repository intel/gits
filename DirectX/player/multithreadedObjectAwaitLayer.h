// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

  void pre(IUnknownAddRefCommand& command) override;
  void pre(IUnknownReleaseCommand& command) override;
  void pre(IUnknownQueryInterfaceCommand& command) override;
  void pre(ID3D12ObjectGetPrivateDataCommand& command) override;
  void pre(ID3D12ObjectSetNameCommand& command) override;
  void pre(ID3D12ObjectSetPrivateDataCommand& command) override;
  void pre(ID3D12ObjectSetPrivateDataInterfaceCommand& command) override;
  void pre(ID3D12DeviceChildGetDeviceCommand& command) override;
  void pre(ID3D12PipelineStateGetCachedBlobCommand& command) override;
  void pre(ID3D12DeviceCreateCommandListCommand& command) override;
  void pre(ID3D12GraphicsCommandListResetCommand& command) override;
  void pre(ID3D12GraphicsCommandListClearStateCommand& command) override;
  void pre(ID3D12GraphicsCommandListSetPipelineStateCommand& command) override;
  void pre(ID3D12GraphicsCommandList4SetPipelineState1Command& command) override;
  void pre(ID3D12DeviceMakeResidentCommand& command) override;
  void pre(ID3D12DeviceEvictCommand& command) override;
  void pre(ID3D12Device1SetResidencyPriorityCommand& command) override;
  void pre(ID3D12Device3EnqueueMakeResidentCommand& command) override;
  void pre(ID3D12Device5CreateStateObjectCommand& command) override;
  void pre(ID3D12Device7AddToStateObjectCommand& command) override;
  void pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& command) override;
  void pre(ID3D12PipelineLibraryLoadComputePipelineCommand& command) override;
  void pre(ID3D12PipelineLibrary1LoadPipelineCommand& command) override;

private:
  std::optional<MultithreadedObjectCreationService::ObjectCreationOutput> collectResult(
      unsigned objectKey);
  bool completeObject(unsigned key);
  template <typename T>
  void completeArgument(InterfaceArgument<T>& commandObject);
  template <typename T>
  void completeArrayArgument(InterfaceArrayArgument<T>& commandObjects);

private:
  PlayerManager& manager_;
  std::unordered_map<unsigned, MultithreadedObjectCreationService::ObjectCreationOutput>
      preCollectedOutputs_;
};

} // namespace DirectX
} // namespace gits
