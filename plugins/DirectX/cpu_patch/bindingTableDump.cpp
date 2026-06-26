// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "bindingTableDump.h"
#include "pluginUtils.h"
#include "log.h"

#include <fstream>

namespace gits {
namespace DirectX {

BindingTableDump::BindingTableDump(const Configuration& gitsConfig) : ResourceDump() {
  if (gitsConfig.directx.features.subcapture.enabled) {
    dumpDir_ = gitsConfig.common.player.subcapturePath;
    frames_ = BitRange(gitsConfig.directx.features.subcapture.frames);
    if (gitsConfig.directx.features.subcapture.commandListExecutions.empty()) {
      executions_ = BitRange("all");
    } else {
      executions_ = BitRange(gitsConfig.directx.features.subcapture.commandListExecutions);
    }
  } else {
    dumpDir_ = gitsConfig.common.player.streamDir;
    frames_ = BitRange("all");
    executions_ = BitRange("all");
  }
  dumpDir_ /= "cpu_patch";
  dumpDir_ /= "binding_table_dump";
  LOG_INFO << "CpuPatch - BindingTableDump location: " << dumpDir_.string();
}

BindingTableDump::~BindingTableDump() {
  waitUntilDumped();
  LOG_INFO << "CpuPatch - BindingTableDump - files dumped: " << std::to_string(numFiles_)
           << ", total size: " << FormatMemorySize(filesTotalSize_);
}

void BindingTableDump::dispatchRays(ID3D12GraphicsCommandList* commandList,
                                    ID3D12Resource* resource,
                                    unsigned offset,
                                    unsigned size,
                                    D3D12_RESOURCE_STATES state,
                                    unsigned commandKey,
                                    std::string type) {
  initialize();

  BindingTableDumpInfo* info = new BindingTableDumpInfo();
  info->offset = offset;
  info->size = size;
  info->dumpName = (dumpDir_ / (std::to_string(commandKey) + "-" + type)).wstring();
  stageResource(commandList, resource, state, *info);
}

void BindingTableDump::executeCommandLists(unsigned key,
                                           unsigned commandQueueKey,
                                           ID3D12CommandQueue* commandQueue,
                                           ID3D12CommandList** commandLists,
                                           unsigned commandListNum,
                                           unsigned frameCount,
                                           unsigned executeCount) {
  for (unsigned i = 0; i < commandListNum; ++i) {
    auto it = stagedResources_.find(commandLists[i]);
    if (it != stagedResources_.end()) {
      for (DumpInfo* dumpInfo : it->second) {
        auto* info = static_cast<BindingTableDumpInfo*>(dumpInfo);
        info->frameCount = frameCount;
        info->executeCount = executeCount;
      }
    }
  }

  ResourceDump::executeCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                    commandListNum);
}

void BindingTableDump::dumpStagedResource(DumpInfo& dumpInfo) {
  {
    const auto& info = static_cast<BindingTableDumpInfo&>(dumpInfo);
    if (!frames_[info.frameCount] || !executions_[info.executeCount]) {
      return;
    }
  }

  void* data{};
  HRESULT hr = dumpInfo.stagingBuffer->Map(0, nullptr, &data);

  std::ofstream stream(dumpInfo.dumpName, std::ios_base::binary);
  stream.write(static_cast<char*>(data), dumpInfo.size);

  dumpInfo.stagingBuffer->Unmap(0, nullptr);

  ++numFiles_;
  filesTotalSize_ += dumpInfo.size;
}

void BindingTableDump::initialize() {
  if (initialized_) {
    return;
  }

  if (!dumpDir_.empty() && !std::filesystem::exists(dumpDir_)) {
    std::filesystem::create_directories(dumpDir_);
  }
  initialized_ = true;
}

} // namespace DirectX
} // namespace gits
