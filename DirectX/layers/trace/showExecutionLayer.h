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
      : Layer("ShowExecution"), printerState_(mutex_), outBuff_(outBuff) {}

  void post(ID3D12DeviceCreateCommandQueueCommand& command) override;
  void post(ID3D12Device9CreateCommandQueue1Command& command) override;
  void post(IDXGISwapChainPresentCommand& command) override;
  void post(IDXGISwapChain1Present1Command& command) override;
  void post(ID3D12CommandQueueExecuteCommandListsCommand& command) override;
  void post(ID3D12GraphicsCommandListResetCommand& command) override;
  void post(ID3D12GraphicsCommandListDrawInstancedCommand& command) override;
  void post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& command) override;
  void post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& command) override;
  void post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& command) override;
  void post(ID3D12GraphicsCommandListDispatchCommand& command) override;
  void post(ID3D12GraphicsCommandListExecuteBundleCommand& command) override;
  void post(ID3D12GraphicsCommandListExecuteIndirectCommand& command) override;
  void post(ID3D12GraphicsCommandListCopyBufferRegionCommand& command) override;
  void post(ID3D12GraphicsCommandListCopyTextureRegionCommand& command) override;
  void post(ID3D12GraphicsCommandListCopyResourceCommand& command) override;
  void post(ID3D12GraphicsCommandListCopyTilesCommand& command) override;
  void post(ID3D12GraphicsCommandListResolveSubresourceCommand& command) override;
  void post(ID3D12GraphicsCommandListResourceBarrierCommand& command) override;
  void post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& command) override;
  void post(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) override;
  void post(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) override;
  void post(ID3D12GraphicsCommandList4DispatchRaysCommand& command) override;
  void post(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& command) override;
  void post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& command) override;
  void post(ID3D12GraphicsCommandList6DispatchMeshCommand& command) override;
  void post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& command) override;
  void post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) override;
  void post(ID3D12GraphicsCommandList7BarrierCommand& command) override;
  void post(ID3D12DeviceCreateFenceCommand& command) override;
  void post(ID3D12FenceSignalCommand& command) override;
  void post(ID3D12CommandQueueSignalCommand& command) override;
  void post(ID3D12Device3EnqueueMakeResidentCommand& command) override;
  void post(ID3D12CommandQueueWaitCommand& command) override;

private:
  CommandPrinterState printerState_;
  FastOStringStream& outBuff_;
  std::mutex mutex_;

  struct Command {
    Command(const std::string& s, bool draw = false) : str(s), isDraw(draw) {}
    std::string str;
    bool isDraw;
  };

  struct CommandQueueEvent : public GpuExecutionTracker::Executable {
    std::string str;
  };
  void fenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue);
  void stageCommandQueueEvent(unsigned commandKey,
                              unsigned commandQueueKey,
                              const std::string& str);
  void dumpReadyCommandQueueEvents();
  std::string replaceSubstring(const std::string& str,
                               const std::string& from,
                               const std::string& to);

  std::map<unsigned, std::vector<Command>> commandListCommands_;
  std::map<unsigned, D3D12_COMMAND_LIST_TYPE> commandQueueTypes_;
  unsigned executeCount{};
  GpuExecutionTracker gpuExecutionTracker_;
};

} // namespace DirectX
} // namespace gits
