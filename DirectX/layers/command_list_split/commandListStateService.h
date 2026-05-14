// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "commandsCustom.h"
#include "commandIdsAuto.h"

#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace gits {
namespace DirectX {

class CommandListStateService {
public:
  explicit CommandListStateService(unsigned commandListKey);
  static bool IsStateCommand(CommandId id);
  void StoreCommand(const Command& c);
  std::vector<std::unique_ptr<Command>> RestoreState() const;

private:
  void StoreCommand(const ID3D12GraphicsCommandList4SetPipelineState1Command& c);
  void StoreCommand(const ID3D12DeviceCreateCommandListCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetPipelineStateCommand& c);

  void StoreCommand(const ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c);

  void StoreCommand(const ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c);

  void StoreCommand(const ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c);

  void StoreCommand(const ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListIASetIndexBufferCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListIASetVertexBuffersCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListRSSetViewportsCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListRSSetScissorRectsCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListOMSetBlendFactorCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListOMSetStencilRefCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandList1OMSetDepthBoundsCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandList1SetSamplePositionsCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandList1SetViewInstanceMaskCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSetPredicationCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandList5RSSetShadingRateCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandList8OMSetFrontAndBackStencilRefCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandList9RSSetDepthBiasCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandList9IASetIndexBufferStripCutValueCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandList10SetProgramCommand& c);
  void StoreCommand(
      const ID3D12GraphicsCommandListPreviewSetWorkGraphMaximumGPUInputRecordsCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListSOSetTargetsCommand& c);

  void StoreCommand(const ID3D12GraphicsCommandListClearStateCommand& c);
  void StoreCommand(const ID3D12GraphicsCommandListResetCommand& c);

  struct CommandListState {
    std::optional<ID3D12GraphicsCommandListSetPipelineStateCommand> PipelineStatePso{};
    std::optional<ID3D12GraphicsCommandList4SetPipelineState1Command> PipelineStateStateObject{};

    std::optional<ID3D12GraphicsCommandListSetDescriptorHeapsCommand> DescriptorHeaps{};

    std::optional<ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand> GraphicsRootSignature{};
    std::map<unsigned, std::unique_ptr<Command>> GraphicsRootArguments;

    std::optional<ID3D12GraphicsCommandListSetComputeRootSignatureCommand> ComputeRootSignature{};
    std::map<unsigned, std::unique_ptr<Command>> ComputeRootArguments;

    std::optional<ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand> PrimitiveTopology{};

    std::optional<ID3D12GraphicsCommandListIASetIndexBufferCommand> IndexBuffer{};
    std::vector<std::unique_ptr<ID3D12GraphicsCommandListIASetVertexBuffersCommand>> VertexBuffers;

    std::optional<ID3D12GraphicsCommandListRSSetViewportsCommand> Viewports{};
    std::optional<ID3D12GraphicsCommandListRSSetScissorRectsCommand> ScissorRects{};
    std::optional<ID3D12GraphicsCommandListOMSetBlendFactorCommand> BlendFactor{};

    std::optional<ID3D12GraphicsCommandListOMSetStencilRefCommand> StencilRef{};

    std::optional<ID3D12GraphicsCommandList1OMSetDepthBoundsCommand> DepthBounds{};

    std::optional<ID3D12GraphicsCommandList1SetSamplePositionsCommand> SamplePositions{};

    std::optional<ID3D12GraphicsCommandList1SetViewInstanceMaskCommand> ViewInstanceMask{};

    std::optional<ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand>
        ProtectedResourceSession{};

    std::optional<ID3D12GraphicsCommandListSetPredicationCommand> Predication{};

    std::optional<ID3D12GraphicsCommandList5RSSetShadingRateCommand> ShadingRate{};
    std::optional<ID3D12GraphicsCommandList5RSSetShadingRateImageCommand> ShadingRateImage{};

    std::optional<ID3D12GraphicsCommandList8OMSetFrontAndBackStencilRefCommand>
        FrontBackStencilRef{};

    std::optional<ID3D12GraphicsCommandList9RSSetDepthBiasCommand> DepthBias{};

    std::optional<ID3D12GraphicsCommandList9IASetIndexBufferStripCutValueCommand>
        IndexBufferStripCutValue{};

    std::optional<ID3D12GraphicsCommandList10SetProgramCommand> SetProgram{};
    std::optional<ID3D12GraphicsCommandListPreviewSetWorkGraphMaximumGPUInputRecordsCommand>
        WorkGraphGpuInputLimits{};

    std::optional<ID3D12GraphicsCommandListOMSetRenderTargetsCommand> RenderTargets{};
    std::vector<std::unique_ptr<ID3D12GraphicsCommandListSOSetTargetsCommand>> StreamOutput;
  };

  void SetPipelineState(unsigned pipelineStateKey);
  void AppendRootCommands(const std::map<unsigned, std::unique_ptr<Command>>& args,
                          std::vector<std::unique_ptr<Command>>& out) const;

  unsigned m_CommandListKey{};
  std::unique_ptr<CommandListState> m_State{std::make_unique<CommandListState>()};
};

} // namespace DirectX
} // namespace gits
