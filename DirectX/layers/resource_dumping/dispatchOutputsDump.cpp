// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "dispatchOutputsDump.h"

namespace gits {
namespace DirectX {

DispatchOutputsDump::~DispatchOutputsDump() {
  WaitUntilDumped();
}

void DispatchOutputsDump::DumpResource(ID3D12GraphicsCommandList* commandList,
                                       ID3D12Resource* resource,
                                       unsigned subresource,
                                       BarrierState resourceState,
                                       const std::wstring& dumpName,
                                       unsigned mipLevel,
                                       DXGI_FORMAT format,
                                       unsigned commandListDispatchCount) {
  auto* dumpInfo = new RenderTargetDumpInfo{};
  dumpInfo->Subresource = subresource;
  dumpInfo->DumpName = dumpName;
  dumpInfo->MipLevel = mipLevel;
  dumpInfo->SubresourceFormat = format;
  dumpInfo->CommandListDispatchCount = commandListDispatchCount;

  StageResource(commandList, resource, resourceState, *dumpInfo);
}

void DispatchOutputsDump::ExecuteCommandLists(unsigned key,
                                              unsigned commandQueueKey,
                                              ID3D12CommandQueue* commandQueue,
                                              ID3D12CommandList** commandLists,
                                              unsigned commandListNum,
                                              unsigned frameCount,
                                              unsigned executeCount) {
  bool found = false;
  for (unsigned i = 0; i < commandListNum; ++i) {
    auto it = m_StagedResources.find(commandLists[i]);
    if (it != m_StagedResources.end()) {
      found = true;
      if (frameCount && executeCount) {
        for (DumpInfo* dumpInfo : it->second) {
          auto* info = static_cast<RenderTargetDumpInfo*>(dumpInfo);
          info->ExecutionCount = std::to_wstring(frameCount) + L"." +
                                 std::to_wstring(executeCount) + L"." + std::to_wstring(i + 1) +
                                 L"." + std::to_wstring(info->CommandListDispatchCount);
        }
      } else {
        break;
      }
    }
  }
  if (!found) {
    return;
  }

  ResourceDump::ExecuteCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                    commandListNum);
}

void DispatchOutputsDump::DumpTexture(DumpInfo& dumpInfo, void* data) {
  auto& info = static_cast<RenderTargetDumpInfo&>(dumpInfo);
  if (!info.ExecutionCount.empty()) {
    const size_t pos = dumpInfo.DumpName.find(m_DumpNameExecutionMarker);
    info.DumpName.replace(pos, m_DumpNameExecutionMarker.size(), info.ExecutionCount);
  }
  ResourceDump::DumpTexture(dumpInfo, data);
}

} // namespace DirectX
} // namespace gits
