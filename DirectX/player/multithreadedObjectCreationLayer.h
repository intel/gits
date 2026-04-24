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

class MultithreadedObjectCreationLayer : public Layer {
public:
  MultithreadedObjectCreationLayer(PlayerManager& manager);

  void Pre(ID3D12DeviceCreateComputePipelineStateCommand& command) override;
  void Pre(ID3D12DeviceCreateGraphicsPipelineStateCommand& command) override;
  void Pre(ID3D12Device2CreatePipelineStateCommand& command) override;
  void Pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& command) override;
  void Pre(ID3D12PipelineLibraryLoadComputePipelineCommand& command) override;
  void Pre(ID3D12PipelineLibrary1LoadPipelineCommand& command) override;
  void Pre(ID3D12Device5CreateStateObjectCommand& command) override;

private:
  template <typename CommandT>
  void ScheduleCreate(CommandT& c);
  template <typename CommandT>
  void ScheduleLoad(CommandT& c);

private:
  PlayerManager& m_Manager;
};

} // namespace DirectX
} // namespace gits
