// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "renderTargetsDump.h"

namespace gits {
namespace DirectX {

RenderTargetsDump::~RenderTargetsDump() {
  waitUntilDumped();
}

void RenderTargetsDump::dumpResource(ID3D12GraphicsCommandList* commandList,
                                     ID3D12Resource* resource,
                                     unsigned subresource,
                                     D3D12_RESOURCE_STATES resourceState,
                                     const std::wstring& dumpName,
                                     unsigned mipLevel,
                                     DXGI_FORMAT format,
                                     unsigned commandListDrawCount) {

  RenderTargetDumpInfo* dumpInfo = new RenderTargetDumpInfo();
  dumpInfo->subresource = subresource;
  dumpInfo->dumpName = dumpName;
  dumpInfo->mipLevel = mipLevel;
  dumpInfo->subresourceFormat = format;
  dumpInfo->commandListDrawCount = commandListDrawCount;

  stageResource(commandList, resource, resourceState, *dumpInfo);
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
    auto it = stagedResources_.find(commandLists[i]);
    if (it != stagedResources_.end()) {
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

  ResourceDump::executeCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                    commandListNum);
}

void RenderTargetsDump::dumpTexture(DumpInfo& dumpInfo, void* data) {
  RenderTargetDumpInfo& info = static_cast<RenderTargetDumpInfo&>(dumpInfo);
  if (!info.executionCount.empty()) {
    size_t pos = dumpInfo.dumpName.find(dumpNameExecutionMarker);
    info.dumpName = info.dumpName.replace(pos, dumpNameExecutionMarker.size(), info.executionCount);
  }
  ResourceDump::dumpTexture(dumpInfo, data);
}

} // namespace DirectX
} // namespace gits
