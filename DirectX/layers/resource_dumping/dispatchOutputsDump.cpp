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
  waitUntilDumped();
}

void DispatchOutputsDump::dumpResource(ID3D12GraphicsCommandList* commandList,
                                       ID3D12Resource* resource,
                                       unsigned subresource,
                                       D3D12_RESOURCE_STATES resourceState,
                                       const std::wstring& dumpName,
                                       unsigned mipLevel,
                                       DXGI_FORMAT format,
                                       unsigned commandListDispatchCount) {
  auto* dumpInfo = new RenderTargetDumpInfo{};
  dumpInfo->subresource = subresource;
  dumpInfo->dumpName = dumpName;
  dumpInfo->mipLevel = mipLevel;
  dumpInfo->subresourceFormat = format;
  dumpInfo->CommandListDispatchCount = commandListDispatchCount;

  stageResource(commandList, resource, resourceState, *dumpInfo);
}

void DispatchOutputsDump::executeCommandLists(unsigned key,
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

  ResourceDump::executeCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                    commandListNum);
}

void DispatchOutputsDump::dumpTexture(DumpInfo& dumpInfo, void* data) {
  auto& info = static_cast<RenderTargetDumpInfo&>(dumpInfo);
  if (!info.ExecutionCount.empty()) {
    const size_t pos = dumpInfo.dumpName.find(m_DumpNameExecutionMarker);
    info.dumpName.replace(pos, m_DumpNameExecutionMarker.size(), info.ExecutionCount);
  }
  ResourceDump::dumpTexture(dumpInfo, data);
}

} // namespace DirectX
} // namespace gits
