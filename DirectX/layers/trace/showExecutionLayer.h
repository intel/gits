// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "commandPrinter.h"
#include "gpuExecutionTracker.h"

#include <iostream>
#include <map>
#include <sstream>
#include <mutex>

namespace gits {
namespace DirectX {

class ShowExecutionLayer : public Layer {
public:
  ShowExecutionLayer(FastOStringStream& outBuff)
      : Layer("ShowExecution"), m_PrinterState(m_Mutex), m_OutBuff(outBuff) {}

  void Post(StateRestoreBeginCommand& command) override;
  void Post(StateRestoreEndCommand& command) override;
  void Post(ID3D12DeviceCreateCommandQueueCommand& command) override;
  void Post(ID3D12Device9CreateCommandQueue1Command& command) override;
  void Post(IDXGISwapChainPresentCommand& command) override;
  void Post(IDXGISwapChain1Present1Command& command) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& command) override;
  void Post(ID3D12GraphicsCommandListResetCommand& command) override;
  void Post(ID3D12GraphicsCommandListDrawInstancedCommand& command) override;
  void Post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& command) override;
  void Post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& command) override;
  void Post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& command) override;
  void Post(ID3D12GraphicsCommandListDispatchCommand& command) override;
  void Post(ID3D12GraphicsCommandListExecuteBundleCommand& command) override;
  void Post(ID3D12GraphicsCommandListExecuteIndirectCommand& command) override;
  void Post(ID3D12GraphicsCommandListCopyBufferRegionCommand& command) override;
  void Post(ID3D12GraphicsCommandListCopyTextureRegionCommand& command) override;
  void Post(ID3D12GraphicsCommandListCopyResourceCommand& command) override;
  void Post(ID3D12GraphicsCommandListCopyTilesCommand& command) override;
  void Post(ID3D12GraphicsCommandListResolveSubresourceCommand& command) override;
  void Post(ID3D12GraphicsCommandListResourceBarrierCommand& command) override;
  void Post(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& command) override;
  void Post(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& command) override;
  void Post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& command) override;
  void Post(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void Post(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) override;
  void Post(ID3D12GraphicsCommandList4DispatchRaysCommand& command) override;
  void Post(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& command) override;
  void Post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& command) override;
  void Post(ID3D12GraphicsCommandList6DispatchMeshCommand& command) override;
  void Post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& command) override;
  void Post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) override;
  void Post(ID3D12GraphicsCommandList7BarrierCommand& command) override;
  void Post(ID3D12DeviceCreateFenceCommand& command) override;
  void Post(ID3D12FenceSignalCommand& command) override;
  void Post(ID3D12CommandQueueSignalCommand& command) override;
  void Post(ID3D12Device3EnqueueMakeResidentCommand& command) override;
  void Post(ID3D12CommandQueueWaitCommand& command) override;

private:
  CommandPrinterState m_PrinterState;
  FastOStringStream& m_OutBuff;
  std::mutex m_Mutex;

  struct Command {
    Command(const std::string& s, bool draw = false) : m_Str(s), m_IsDraw(draw) {}
    std::string m_Str;
    bool m_IsDraw;
  };

  struct CommandQueueEvent : public GpuExecutionTracker::Executable {
    std::string m_Str;
  };
  void FenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue);
  void StageCommandQueueEvent(unsigned commandKey,
                              unsigned commandQueueKey,
                              const std::string& str);
  void DumpReadyCommandQueueEvents();
  std::string ReplaceSubstring(const std::string& str,
                               const std::string& from,
                               const std::string& to);

  std::map<unsigned, std::vector<Command>> m_CommandListCommands;
  std::map<unsigned, D3D12_COMMAND_LIST_TYPE> m_CommandQueueTypes;
  unsigned m_ExecuteCount{};
  unsigned m_CurrentFrame{1};
  GpuExecutionTracker m_GpuExecutionTracker;
};

} // namespace DirectX
} // namespace gits
