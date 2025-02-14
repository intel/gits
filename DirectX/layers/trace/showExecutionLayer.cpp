// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "showExecutionLayer.h"
#include "gits.h"

namespace gits {
namespace DirectX {

void ShowExecutionLayer::post(ID3D12DeviceCreateCommandQueueCommand& command) {
  commandQueueTypes_[command.ppCommandQueue_.key] = command.pDesc_.value->Type;
}

void ShowExecutionLayer::post(ID3D12Device9CreateCommandQueue1Command& command) {
  commandQueueTypes_[command.ppCommandQueue_.key] = command.pDesc_.value->Type;
}

void ShowExecutionLayer::post(IDXGISwapChainPresentCommand& command) {
  if (!(command.Flags_.value & DXGI_PRESENT_TEST)) {
    CommandPrinter p(outBuff_, printerState_, command, "IDXGISwapChain::Present",
                     command.object_.key);
    p.addArgument(command.SyncInterval_);
    p.addArgument(command.Flags_);
    p.addResult(command.result_);
    p.print(true);

    executeCount = 0;
  }
}

void ShowExecutionLayer::post(IDXGISwapChain1Present1Command& command) {
  if (!(command.PresentFlags_.value & DXGI_PRESENT_TEST)) {
    CommandPrinter p(outBuff_, printerState_, command, "IDXGISwapChain1::Present1",
                     command.object_.key);
    p.addArgument(command.SyncInterval_);
    p.addArgument(command.PresentFlags_);
    p.addResult(command.result_);
    p.print(true);

    executeCount = 0;
  }
}

void ShowExecutionLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& command) {

  outBuff_ << "  ";
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12CommandQueue::ExecuteCommandLists",
                   command.object_.key);
  ++executeCount;
  p.addArgument(command.NumCommandLists_);
  p.addArgument(command.ppCommandLists_);
  p.print(true);

  std::string type;
  if (commandQueueTypes_.find(command.object_.key) != commandQueueTypes_.end()) {
    switch (commandQueueTypes_.at(command.object_.key)) {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
      type = "Direct";
      break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
      type = "Compute";
      break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
      type = "Copy";
      break;
    }
  }

  for (unsigned i = 0; i < command.NumCommandLists_.value; ++i) {
    auto it = commandListCommands_.find(command.ppCommandLists_.keys[i]);
    if (it != commandListCommands_.end()) {
      outBuff_ << "    " << type << " CommandList O" << command.ppCommandLists_.keys[i] << ":\n";
      outBuff_.flush();

      unsigned drawCount{};
      unsigned frameCount = printerState_.stateRestorePhase ? 0 : CGits::Instance().CurrentFrame();
      for (Command& c : it->second) {
        outBuff_ << "      " << c.str;
        if (c.isDraw) {
          outBuff_ << " execution #" << frameCount << "." << executeCount << "." << i + 1 << "."
                   << ++drawCount << "\n";
        }
        outBuff_.flush();
      }
    }
  }
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListResetCommand& command) {
  // Lock just in case. This code is used from player, so a lock shouldn't be needed.
  std::lock_guard<std::mutex> lock_(printerState_.mutex);

  auto it = commandListCommands_.find(command.object_.key);
  if (it != commandListCommands_.end()) {
    it->second.clear();
  }
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListDrawInstancedCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12GraphicsCommandList::DrawInstanced",
                   command.object_.key);
  p.addArgument(command.VertexCountPerInstance_);
  p.addArgument(command.InstanceCount_);
  p.addArgument(command.StartVertexLocation_);
  p.addArgument(command.StartInstanceLocation_);
  p.print(false, false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString(), true);
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command,
                   "ID3D12GraphicsCommandList::DrawIndexedInstanced", command.object_.key);
  p.addArgument(command.IndexCountPerInstance_);
  p.addArgument(command.InstanceCount_);
  p.addArgument(command.StartIndexLocation_);
  p.addArgument(command.BaseVertexLocation_);
  p.addArgument(command.StartInstanceLocation_);
  p.print(false, false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString(), true);
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command,
                   "ID3D12GraphicsCommandList::ClearRenderTargetView", command.object_.key);
  p.addArgument(command.RenderTargetView_);
  p.addArgument(command.ColorRGBA_);
  p.addArgument(command.NumRects_);
  p.addArgument(command.pRects_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command,
                   "ID3D12GraphicsCommandList::ClearDepthStencilView", command.object_.key);
  p.addArgument(command.DepthStencilView_);
  p.addArgument(command.ClearFlags_);
  p.addArgument(command.Depth_);
  p.addArgument(command.Stencil_);
  p.addArgument(command.NumRects_);
  p.addArgument(command.pRects_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListDispatchCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12GraphicsCommandList::Dispatch",
                   command.object_.key);
  p.addArgument(command.ThreadGroupCountX_);
  p.addArgument(command.ThreadGroupCountY_);
  p.addArgument(command.ThreadGroupCountZ_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListExecuteBundleCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12GraphicsCommandList::ExecuteBundle",
                   command.object_.key);
  p.addArgument(command.pCommandList_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListExecuteIndirectCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12GraphicsCommandList::ExecuteIndirect",
                   command.object_.key);
  p.addArgument(command.pCommandSignature_);
  p.addArgument(command.MaxCommandCount_);
  p.addArgument(command.pArgumentBuffer_);
  p.addArgument(command.ArgumentBufferOffset_);
  p.addArgument(command.pCountBuffer_);
  p.addArgument(command.CountBufferOffset_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListCopyBufferRegionCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12GraphicsCommandList::CopyBufferRegion",
                   command.object_.key);
  p.addArgument(command.pDstBuffer_);
  p.addArgument(command.DstOffset_);
  p.addArgument(command.pSrcBuffer_);
  p.addArgument(command.SrcOffset_);
  p.addArgument(command.NumBytes_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListCopyTextureRegionCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12GraphicsCommandList::CopyTextureRegion",
                   command.object_.key);
  p.addArgument(command.pDst_);
  p.addArgument(command.DstX_);
  p.addArgument(command.DstY_);
  p.addArgument(command.DstZ_);
  p.addArgument(command.pSrc_);
  p.addArgument(command.pSrcBox_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListCopyResourceCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12GraphicsCommandList::CopyResource",
                   command.object_.key);
  p.addArgument(command.pDstResource_);
  p.addArgument(command.pSrcResource_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListCopyTilesCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12GraphicsCommandList::CopyTiles",
                   command.object_.key);
  p.addArgument(command.pTiledResource_);
  p.addArgument(command.pTileRegionStartCoordinate_);
  p.addArgument(command.pTileRegionSize_);
  p.addArgument(command.pBuffer_);
  p.addArgument(command.BufferStartOffsetInBytes_);
  p.addArgument(command.Flags_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListResolveSubresourceCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command,
                   "ID3D12GraphicsCommandList::ResolveSubresource", command.object_.key);
  p.addArgument(command.pDstResource_);
  p.addArgument(command.DstSubresource_);
  p.addArgument(command.pSrcResource_);
  p.addArgument(command.SrcSubresource_);
  p.addArgument(command.Format_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command,
                   "ID3D12GraphicsCommandList1::ResolveSubresourceRegion", command.object_.key);
  p.addArgument(command.pDstResource_);
  p.addArgument(command.DstSubresource_);
  p.addArgument(command.DstX_);
  p.addArgument(command.DstY_);
  p.addArgument(command.pSrcResource_);
  p.addArgument(command.SrcSubresource_);
  p.addArgument(command.pSrcRect_);
  p.addArgument(command.Format_);
  p.addArgument(command.ResolveMode_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command,
                   "ID3D12GraphicsCommandList4::BuildRaytracingAccelerationStructure",
                   command.object_.key);
  p.addArgument(command.pDesc_);
  p.addArgument(command.NumPostbuildInfoDescs_);
  p.addArgument(command.pPostbuildInfoDescs_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command,
                   "ID3D12GraphicsCommandList4::CopyRaytracingAccelerationStructure",
                   command.object_.key);
  p.addArgument(command.DestAccelerationStructureData_);
  p.addArgument(command.SourceAccelerationStructureData_);
  p.addArgument(command.Mode_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandList4DispatchRaysCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12GraphicsCommandList4::DispatchRays",
                   command.object_.key);
  p.addArgument(command.pDesc_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command,
                   "ID3D12GraphicsCommandList4::ExecuteMetaCommand", command.object_.key);
  p.addArgument(command.pMetaCommand_);
  p.addArgument(command.pExecutionParametersData_);
  p.addArgument(command.ExecutionParametersDataSizeInBytes_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command,
                   "ID3D12GraphicsCommandList4::InitializeMetaCommand", command.object_.key);
  p.addArgument(command.pMetaCommand_);
  p.addArgument(command.pInitializationParametersData_);
  p.addArgument(command.InitializationParametersDataSizeInBytes_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandList6DispatchMeshCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12GraphicsCommandList6::DispatchMesh",
                   command.object_.key);
  p.addArgument(command.ThreadGroupCountX_);
  p.addArgument(command.ThreadGroupCountY_);
  p.addArgument(command.ThreadGroupCountZ_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command,
                   "ID3D12GraphicsCommandList::OMSetRenderTargets", command.object_.key);
  p.addArgument(command.NumRenderTargetDescriptors_);
  p.addArgument(command.pRenderTargetDescriptors_);
  p.addArgument(command.RTsSingleHandleToDescriptorRange_);
  p.addArgument(command.pDepthStencilDescriptor_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) {
  outBuff_.flush();
  CommandPrinter p(outBuff_, printerState_, command,
                   "ID3D12GraphicsCommandList::SetGraphicsRootUnorderedAccessView",
                   command.object_.key);
  p.addArgument(command.RootParameterIndex_);
  p.addArgument(command.BufferLocation_);
  p.print(false);

  commandListCommands_[command.object_.key].emplace_back(outBuff_.extractString());
}

void ShowExecutionLayer::post(ID3D12FenceSignalCommand& command) {
  outBuff_ << "  ";
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12Fence::Signal", command.object_.key);
  p.addArgument(command.Value_);
  p.addResult(command.result_);
  p.print(true);
}

void ShowExecutionLayer::post(ID3D12CommandQueueSignalCommand& command) {
  outBuff_ << "  ";
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12CommandQueue::Signal",
                   command.object_.key);
  p.addArgument(command.pFence_);
  p.addArgument(command.Value_);
  p.addResult(command.result_);
  p.print(true);
}

void ShowExecutionLayer::post(ID3D12Device3EnqueueMakeResidentCommand& command) {
  outBuff_ << "  ";
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12Device3::EnqueueMakeResident",
                   command.object_.key);
  p.addArgument(command.Flags_);
  p.addArgument(command.NumObjects_);
  p.addArgument(command.ppObjects_);
  p.addArgument(command.pFenceToSignal_);
  p.addArgument(command.FenceValueToSignal_);
  p.addResult(command.result_);
  p.print(true);
}

void ShowExecutionLayer::post(ID3D12CommandQueueWaitCommand& command) {
  outBuff_ << "  ";
  CommandPrinter p(outBuff_, printerState_, command, "ID3D12CommandQueue::Wait",
                   command.object_.key);
  p.addArgument(command.pFence_);
  p.addArgument(command.Value_);
  p.addResult(command.result_);
  p.print(true);
}

} // namespace DirectX
} // namespace gits
