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

class MultithreadedObjectCreationLayer : public Layer {
public:
  MultithreadedObjectCreationLayer(PlayerManager& manager);

  void pre(ID3D12DeviceCreateComputePipelineStateCommand& command) override;
  void pre(ID3D12DeviceCreateGraphicsPipelineStateCommand& command) override;
  void pre(ID3D12Device2CreatePipelineStateCommand& command) override;
  void pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& command) override;
  void pre(ID3D12PipelineLibraryLoadComputePipelineCommand& command) override;
  void pre(ID3D12PipelineLibrary1LoadPipelineCommand& command) override;
  void pre(ID3D12Device5CreateStateObjectCommand& command) override;

private:
  template <typename CommandT>
  void scheduleCreate(CommandT& c);
  template <typename CommandT>
  void scheduleLoad(CommandT& c);

private:
  PlayerManager& manager_;
};

} // namespace DirectX
} // namespace gits
