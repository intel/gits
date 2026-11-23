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
#include "multithreadedShaderCompilationService.h"

#include <functional>
#include <vector>

namespace gits {
namespace DirectX {

class MultithreadedShaderCompilationScheduleLayer : public Layer {
public:
  MultithreadedShaderCompilationScheduleLayer(PlayerManager& manager);

  void Pre(ID3D12DeviceCreateGraphicsPipelineStateCommand& command) override;
  void Pre(ID3D12DeviceCreateComputePipelineStateCommand& command) override;
  void Pre(ID3D12Device2CreatePipelineStateCommand& command) override;
  void Pre(ID3D12Device5CreateStateObjectCommand& command) override;
  void Pre(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& command) override;
  void Pre(ID3D12PipelineLibraryLoadComputePipelineCommand& command) override;
  void Pre(ID3D12PipelineLibrary1LoadPipelineCommand& command) override;

private:
  template <typename CommandT>
  bool CanSchedule(const CommandT& c) const;
  template <typename CommandT>
  void ScheduleLibraryLoad(CommandT& c, IUnknown* rootSignature);
  void Enqueue(ObjectKey key,
               bool& skip,
               void**& outputValue,
               std::vector<IUnknown*> keepAlive,
               std::function<HRESULT(void**)> create);

private:
  PlayerManager& m_Manager;
};

} // namespace DirectX
} // namespace gits
