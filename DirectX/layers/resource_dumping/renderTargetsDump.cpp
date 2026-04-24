// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "renderTargetsDump.h"

namespace gits {
namespace DirectX {

RenderTargetsDump::~RenderTargetsDump() {
  WaitUntilDumped();
}

void RenderTargetsDump::dumpResource(ID3D12GraphicsCommandList* commandList,
                                     ID3D12Resource* resource,
                                     unsigned subresource,
                                     BarrierState resourceState,
                                     const std::wstring& dumpName,
                                     unsigned mipLevel,
                                     DXGI_FORMAT format,
                                     unsigned commandListDrawCount) {

  RenderTargetDumpInfo* dumpInfo = new RenderTargetDumpInfo();
  dumpInfo->Subresource = subresource;
  dumpInfo->DumpName = dumpName;
  dumpInfo->MipLevel = mipLevel;
  dumpInfo->SubresourceFormat = format;
  dumpInfo->commandListDrawCount = commandListDrawCount;

  StageResource(commandList, resource, resourceState, *dumpInfo);
}

void RenderTargetsDump::executeCommandLists(unsigned key,
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
          RenderTargetDumpInfo* info = static_cast<RenderTargetDumpInfo*>(dumpInfo);
          info->executionCount = std::to_wstring(frameCount) + L"." +
                                 std::to_wstring(executeCount) + L"." + std::to_wstring(i + 1) +
                                 L"." + std::to_wstring(info->commandListDrawCount);
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

void RenderTargetsDump::DumpTexture(DumpInfo& dumpInfo, void* data) {
  RenderTargetDumpInfo& info = static_cast<RenderTargetDumpInfo&>(dumpInfo);
  if (!info.executionCount.empty()) {
    size_t pos = dumpInfo.DumpName.find(dumpNameExecutionMarker);
    info.DumpName = info.DumpName.replace(pos, dumpNameExecutionMarker.size(), info.executionCount);
  }
  ResourceDump::DumpTexture(dumpInfo, data);
}

} // namespace DirectX
} // namespace gits
