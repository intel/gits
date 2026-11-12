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

#include <map>

namespace gits {
namespace DirectX {

class MultithreadedShaderCompilationResolveLayer : public Layer {
public:
  MultithreadedShaderCompilationResolveLayer(PlayerManager& manager);

  void Pre(StateRestoreEndCommand& command) override;

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
  bool ResolveObject(ObjectKey key);
  bool ResolvePending(ObjectKey key);
  template <typename T>
  void ResolveArgument(InterfaceArgument<T>& argument);
  template <typename T>
  void ResolveArrayArgument(InterfaceArrayArgument<T>& arrayArgument);
  void ResolveExistingCollections(const std::map<unsigned, ObjectKey>& interfaceKeysBySubobject,
                                  const D3D12_STATE_OBJECT_DESC* desc,
                                  bool& requiresUpdate);

private:
  PlayerManager& m_Manager;
};

} // namespace DirectX
} // namespace gits
