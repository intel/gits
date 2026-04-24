// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "showExecutionLayer.h"
#include "log.h"
#include "keyUtils.h"

namespace gits {
namespace DirectX {

void ShowExecutionLayer::Post(StateRestoreBeginCommand& command) {
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "StateRestoreBegin");
  m_OutBuff << "STATE_RESTORE_BEGIN\n";
  m_OutBuff.Flush();
  m_CurrentFrame = 0;
}

void ShowExecutionLayer::Post(StateRestoreEndCommand& command) {
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "StateRestoreEnd");
  m_OutBuff << "STATE_RESTORE_END\n";
  m_OutBuff.Flush();
  m_CurrentFrame = 1;
}

void ShowExecutionLayer::Post(ID3D12DeviceCreateCommandQueueCommand& command) {
  m_CommandQueueTypes[command.m_ppCommandQueue.Key] = command.m_pDesc.Value->Type;
}

void ShowExecutionLayer::Post(ID3D12Device9CreateCommandQueue1Command& command) {
  m_CommandQueueTypes[command.m_ppCommandQueue.Key] = command.m_pDesc.Value->Type;
}

void ShowExecutionLayer::Post(IDXGISwapChainPresentCommand& command) {
  if (!(command.m_Flags.Value & DXGI_PRESENT_TEST)) {
    CommandPrinter p(m_OutBuff, m_PrinterState, command, "IDXGISwapChain::Present",
                     command.m_Object.Key);
    p.addArgument(command.m_SyncInterval);
    p.addArgument(command.m_Flags);
    p.addResult(command.m_Result);
    p.print(true);

    m_ExecuteCount = 0;
    if (!IsStateRestoreKey(command.Key)) {
      ++m_CurrentFrame;
    }
  }
}

void ShowExecutionLayer::Post(IDXGISwapChain1Present1Command& command) {
  if (!(command.m_PresentFlags.Value & DXGI_PRESENT_TEST)) {
    CommandPrinter p(m_OutBuff, m_PrinterState, command, "IDXGISwapChain1::Present1",
                     command.m_Object.Key);
    p.addArgument(command.m_SyncInterval);
    p.addArgument(command.m_PresentFlags);
    p.addResult(command.m_Result);
    p.print(true);

    m_ExecuteCount = 0;
    if (!IsStateRestoreKey(command.Key)) {
      ++m_CurrentFrame;
    }
  }
}

void ShowExecutionLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& command) {
  m_OutBuff.Flush();
  m_OutBuff << "  ";

  const bool isWaiting = m_GpuExecutionTracker.IsCommandQueueWaiting(command.m_Object.Key);
  if (isWaiting) {
    m_OutBuff << "[DEFERRED] ";
  }
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12CommandQueue::ExecuteCommandLists",
                   command.m_Object.Key);
  ++m_ExecuteCount;
  p.addArgument(command.m_NumCommandLists);
  p.addArgument(command.m_ppCommandLists);
  p.print(false, false);
  if (!(IsExecutionSerializationKey(command.Key) && !m_PrinterState.stateRestorePhase)) {
    m_OutBuff << " Frame #" << m_PrinterState.frameCount << " Frame Execute #" << m_ExecuteCount
              << "\n";
  } else {
    m_OutBuff << "\n";
  }

  std::string type;
  if (m_CommandQueueTypes.find(command.m_Object.Key) != m_CommandQueueTypes.end()) {
    switch (m_CommandQueueTypes.at(command.m_Object.Key)) {
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

  for (unsigned i = 0; i < command.m_NumCommandLists.Value; ++i) {
    auto it = m_CommandListCommands.find(command.m_ppCommandLists.Keys[i]);
    if (it != m_CommandListCommands.end()) {
      unsigned drawCount{};

      m_OutBuff << "    " << type << " CommandList O" << command.m_ppCommandLists.Keys[i]
                << " Frame #" << m_CurrentFrame << " Frame Execute #" << m_ExecuteCount
                << " CommandList #" << i + 1 << "\n";

      for (Command& cmdEntry : it->second) {
        if (cmdEntry.m_IsDraw) {
          std::string baseStr = cmdEntry.m_Str;
          std::string frameDrawSuffix;
          if (auto pos = baseStr.rfind(" Frame Draw #"); pos != std::string::npos) {
            frameDrawSuffix = baseStr.substr(pos);
            baseStr.resize(pos);
          }
          m_OutBuff << "      " << baseStr << " Frame Execute #" << m_ExecuteCount
                    << " CommandList #" << i + 1 << " CommandList Draw #" << ++drawCount << " (e_"
                    << m_CurrentFrame << "_" << m_ExecuteCount << "_" << i + 1 << "_" << drawCount
                    << ")" << frameDrawSuffix << "\n";
        } else {
          m_OutBuff << "      " << cmdEntry.m_Str;
        }
      }
    }
  }

  const auto str = m_OutBuff.ExtractString();
  m_OutBuff << str;
  m_OutBuff.Flush();
  if (isWaiting) {
    StageCommandQueueEvent(command.Key, command.m_Object.Key,
                           ReplaceSubstring("  " + str, "[DEFERRED] ", ""));
  }
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListResetCommand& command) {
  // Lock just in case. This code is used from player, so a lock shouldn't be needed.
  std::lock_guard<std::mutex> lockGuard(m_PrinterState.mutex);

  auto it = m_CommandListCommands.find(command.m_Object.Key);
  if (it != m_CommandListCommands.end()) {
    it->second.clear();
  }
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListDrawInstancedCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12GraphicsCommandList::DrawInstanced",
                   command.m_Object.Key);
  p.addArgument(command.m_VertexCountPerInstance);
  p.addArgument(command.m_InstanceCount);
  p.addArgument(command.m_StartVertexLocation);
  p.addArgument(command.m_StartInstanceLocation);
  p.print(false, false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString(), true);
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList::DrawIndexedInstanced", command.m_Object.Key);
  p.addArgument(command.m_IndexCountPerInstance);
  p.addArgument(command.m_InstanceCount);
  p.addArgument(command.m_StartIndexLocation);
  p.addArgument(command.m_BaseVertexLocation);
  p.addArgument(command.m_StartInstanceLocation);
  p.print(false, false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString(), true);
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList::ClearRenderTargetView", command.m_Object.Key);
  p.addArgument(command.m_RenderTargetView);
  p.addArgument(command.m_ColorRGBA);
  p.addArgument(command.m_NumRects);
  p.addArgument(command.m_pRects);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList::ClearDepthStencilView", command.m_Object.Key);
  p.addArgument(command.m_DepthStencilView);
  p.addArgument(command.m_ClearFlags);
  p.addArgument(command.m_Depth);
  p.addArgument(command.m_Stencil);
  p.addArgument(command.m_NumRects);
  p.addArgument(command.m_pRects);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListDispatchCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12GraphicsCommandList::Dispatch",
                   command.m_Object.Key);
  p.addArgument(command.m_ThreadGroupCountX);
  p.addArgument(command.m_ThreadGroupCountY);
  p.addArgument(command.m_ThreadGroupCountZ);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListExecuteBundleCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12GraphicsCommandList::ExecuteBundle",
                   command.m_Object.Key);
  p.addArgument(command.m_pCommandList);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListExecuteIndirectCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12GraphicsCommandList::ExecuteIndirect",
                   command.m_Object.Key);
  p.addArgument(command.m_pCommandSignature);
  p.addArgument(command.m_MaxCommandCount);
  p.addArgument(command.m_pArgumentBuffer);
  p.addArgument(command.m_ArgumentBufferOffset);
  p.addArgument(command.m_pCountBuffer);
  p.addArgument(command.m_CountBufferOffset);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListCopyBufferRegionCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList::CopyBufferRegion", command.m_Object.Key);
  p.addArgument(command.m_pDstBuffer);
  p.addArgument(command.m_DstOffset);
  p.addArgument(command.m_pSrcBuffer);
  p.addArgument(command.m_SrcOffset);
  p.addArgument(command.m_NumBytes);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListCopyTextureRegionCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList::CopyTextureRegion", command.m_Object.Key);
  p.addArgument(command.m_pDst);
  p.addArgument(command.m_DstX);
  p.addArgument(command.m_DstY);
  p.addArgument(command.m_DstZ);
  p.addArgument(command.m_pSrc);
  p.addArgument(command.m_pSrcBox);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListCopyResourceCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12GraphicsCommandList::CopyResource",
                   command.m_Object.Key);
  p.addArgument(command.m_pDstResource);
  p.addArgument(command.m_pSrcResource);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListCopyTilesCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12GraphicsCommandList::CopyTiles",
                   command.m_Object.Key);
  p.addArgument(command.m_pTiledResource);
  p.addArgument(command.m_pTileRegionStartCoordinate);
  p.addArgument(command.m_pTileRegionSize);
  p.addArgument(command.m_pBuffer);
  p.addArgument(command.m_BufferStartOffsetInBytes);
  p.addArgument(command.m_Flags);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListResolveSubresourceCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList::ResolveSubresource", command.m_Object.Key);
  p.addArgument(command.m_pDstResource);
  p.addArgument(command.m_DstSubresource);
  p.addArgument(command.m_pSrcResource);
  p.addArgument(command.m_SrcSubresource);
  p.addArgument(command.m_Format);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListResourceBarrierCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12GraphicsCommandList::ResourceBarrier",
                   command.m_Object.Key);
  p.addArgument(command.m_NumBarriers);
  p.addArgument(command.m_pBarriers);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList::ClearUnorderedAccessViewFloat",
                   command.m_Object.Key);
  p.addArgument(command.m_ViewGPUHandleInCurrentHeap);
  p.addArgument(command.m_ViewCPUHandle);
  p.addArgument(command.m_pResource);
  p.addArgument(command.m_Values);
  p.addArgument(command.m_NumRects);
  p.addArgument(command.m_pRects);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList::ClearUnorderedAccessViewUint", command.m_Object.Key);
  p.addArgument(command.m_ViewGPUHandleInCurrentHeap);
  p.addArgument(command.m_ViewCPUHandle);
  p.addArgument(command.m_pResource);
  p.addArgument(command.m_Values);
  p.addArgument(command.m_NumRects);
  p.addArgument(command.m_pRects);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList1::ResolveSubresourceRegion", command.m_Object.Key);
  p.addArgument(command.m_pDstResource);
  p.addArgument(command.m_DstSubresource);
  p.addArgument(command.m_DstX);
  p.addArgument(command.m_DstY);
  p.addArgument(command.m_pSrcResource);
  p.addArgument(command.m_SrcSubresource);
  p.addArgument(command.m_pSrcRect);
  p.addArgument(command.m_Format);
  p.addArgument(command.m_ResolveMode);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList4::BuildRaytracingAccelerationStructure",
                   command.m_Object.Key);
  p.addArgument(command.m_pDesc);
  p.addArgument(command.m_NumPostbuildInfoDescs);
  p.addArgument(command.m_pPostbuildInfoDescs);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList4::CopyRaytracingAccelerationStructure",
                   command.m_Object.Key);
  p.addArgument(command.m_DestAccelerationStructureData);
  p.addArgument(command.m_SourceAccelerationStructureData);
  p.addArgument(command.m_Mode);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandList4DispatchRaysCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12GraphicsCommandList4::DispatchRays",
                   command.m_Object.Key);
  p.addArgument(command.m_pDesc);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList4::ExecuteMetaCommand", command.m_Object.Key);
  p.addArgument(command.m_pMetaCommand);
  p.addArgument(command.m_pExecutionParametersData);
  p.addArgument(command.m_ExecutionParametersDataSizeInBytes);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList4::InitializeMetaCommand", command.m_Object.Key);
  p.addArgument(command.m_pMetaCommand);
  p.addArgument(command.m_pInitializationParametersData);
  p.addArgument(command.m_InitializationParametersDataSizeInBytes);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandList6DispatchMeshCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12GraphicsCommandList6::DispatchMesh",
                   command.m_Object.Key);
  p.addArgument(command.m_ThreadGroupCountX);
  p.addArgument(command.m_ThreadGroupCountY);
  p.addArgument(command.m_ThreadGroupCountZ);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList::OMSetRenderTargets", command.m_Object.Key);
  p.addArgument(command.m_NumRenderTargetDescriptors);
  p.addArgument(command.m_pRenderTargetDescriptors);
  p.addArgument(command.m_RTsSingleHandleToDescriptorRange);
  p.addArgument(command.m_pDepthStencilDescriptor);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command,
                   "ID3D12GraphicsCommandList::SetGraphicsRootUnorderedAccessView",
                   command.m_Object.Key);
  p.addArgument(command.m_RootParameterIndex);
  p.addArgument(command.m_BufferLocation);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12GraphicsCommandList7BarrierCommand& command) {
  m_OutBuff.Flush();
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12GraphicsCommandList7::Barrier",
                   command.m_Object.Key);
  p.addArgument(command.m_NumBarrierGroups);
  p.addArgument(command.m_pBarrierGroups);
  p.print(false);

  m_CommandListCommands[command.m_Object.Key].emplace_back(m_OutBuff.ExtractString());
}

void ShowExecutionLayer::Post(ID3D12DeviceCreateFenceCommand& command) {
  m_OutBuff << "  ";
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12Device::CreateFence",
                   command.m_Object.Key);
  p.addArgument(command.m_InitialValue);
  p.addArgument(command.m_Flags);
  p.addArgument(command.m_riid);
  p.addArgument(command.m_ppFence);
  p.addResult(command.m_Result);
  p.print(true);

  FenceSignal(command.Key, command.m_ppFence.Key, command.m_InitialValue.Value);
}

void ShowExecutionLayer::Post(ID3D12FenceSignalCommand& command) {
  m_OutBuff << "  ";
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12Fence::Signal", command.m_Object.Key);
  p.addArgument(command.m_Value);
  p.addResult(command.m_Result);
  p.print(true);

  FenceSignal(command.Key, command.m_Object.Key, command.m_Value.Value);
}

void ShowExecutionLayer::Post(ID3D12CommandQueueSignalCommand& command) {
  m_OutBuff.Flush();
  m_OutBuff << "  ";

  const bool isWaiting = m_GpuExecutionTracker.IsCommandQueueWaiting(command.m_Object.Key);
  if (isWaiting) {
    m_OutBuff << "[DEFERRED] ";
  }
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12CommandQueue::Signal",
                   command.m_Object.Key);
  p.addArgument(command.m_pFence);
  p.addArgument(command.m_Value);
  p.addResult(command.m_Result);
  p.print(false);

  const auto str = m_OutBuff.ExtractString();
  m_OutBuff << str;
  m_OutBuff.Flush();

  m_GpuExecutionTracker.CommandQueueSignal(command.Key, command.m_Object.Key, command.m_pFence.Key,
                                           command.m_Value.Value);

  if (isWaiting) {
    StageCommandQueueEvent(command.Key, command.m_Object.Key,
                           ReplaceSubstring("  " + str, "[DEFERRED] ", ""));
  } else {
    auto count = m_GpuExecutionTracker.GetReadyExecutables().size();
    if (count > 0) {
      m_OutBuff << "  CommandQueueSignal unlocking " << count << " events:\n";
      m_OutBuff.Flush();
    }
    DumpReadyCommandQueueEvents();
  }
}

void ShowExecutionLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& command) {
  m_OutBuff << "  ";
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12Device3::EnqueueMakeResident",
                   command.m_Object.Key);
  p.addArgument(command.m_Flags);
  p.addArgument(command.m_NumObjects);
  p.addArgument(command.m_ppObjects);
  p.addArgument(command.m_pFenceToSignal);
  p.addArgument(command.m_FenceValueToSignal);
  p.addResult(command.m_Result);
  p.print(true);

  FenceSignal(command.Key, command.m_pFenceToSignal.Key, command.m_FenceValueToSignal.Value);
}

void ShowExecutionLayer::Post(ID3D12CommandQueueWaitCommand& command) {
  m_OutBuff.Flush();
  m_OutBuff << "  ";

  m_GpuExecutionTracker.CommandQueueWait(command.Key, command.m_Object.Key, command.m_pFence.Key,
                                         command.m_Value.Value);
  const bool isWaiting = m_GpuExecutionTracker.IsCommandQueueWaiting(command.m_Object.Key);
  if (isWaiting) {
    m_OutBuff << "[DEFERRED] ";
  }
  CommandPrinter p(m_OutBuff, m_PrinterState, command, "ID3D12CommandQueue::Wait",
                   command.m_Object.Key);
  p.addArgument(command.m_pFence);
  p.addArgument(command.m_Value);
  p.addResult(command.m_Result);
  p.print(false);

  const auto str = m_OutBuff.ExtractString();
  m_OutBuff << str;
  m_OutBuff.Flush();
  if (isWaiting) {
    StageCommandQueueEvent(command.Key, command.m_Object.Key,
                           ReplaceSubstring("  " + str, "[DEFERRED] ", ""));
  }
}

void ShowExecutionLayer::FenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue) {
  m_GpuExecutionTracker.FenceSignal(callKey, fenceKey, fenceValue);
  auto count = m_GpuExecutionTracker.GetReadyExecutables().size();
  if (count > 0) {
    m_OutBuff << "  Signal unlocking: " << count << " events:\n";
    m_OutBuff.Flush();
  }
  DumpReadyCommandQueueEvents();
}

void ShowExecutionLayer::StageCommandQueueEvent(unsigned commandKey,
                                                unsigned commandQueueKey,
                                                const std::string& str) {
  auto* event = new CommandQueueEvent{};
  event->m_Str = str;
  m_GpuExecutionTracker.Execute(commandKey, commandQueueKey, event);
}

void ShowExecutionLayer::DumpReadyCommandQueueEvents() {
  std::vector<GpuExecutionTracker::Executable*>& executables =
      m_GpuExecutionTracker.GetReadyExecutables();
  for (GpuExecutionTracker::Executable* executable : executables) {
    CommandQueueEvent* event = static_cast<CommandQueueEvent*>(executable);
    m_OutBuff << event->m_Str;
    m_OutBuff.Flush();
    delete event;
  }
  executables.clear();
}

std::string ShowExecutionLayer::ReplaceSubstring(const std::string& str,
                                                 const std::string& from,
                                                 const std::string& to) {
  if (from.empty()) {
    return str;
  }

  std::string result = str;
  size_t startPos = 0;

  while ((startPos = result.find(from, startPos)) != std::string::npos) {
    result.replace(startPos, from.length(), to);
    startPos += to.length();
  }

  return result;
}

} // namespace DirectX
} // namespace gits
