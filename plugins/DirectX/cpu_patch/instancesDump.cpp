// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "instancesDump.h"
#include "pluginUtils.h"
#include "log.h"

#include <fstream>

namespace gits {
namespace DirectX {

InstancesDump::InstancesDump(const Configuration& gitsConfig) : ResourceDump() {
  if (gitsConfig.directx.features.subcapture.enabled) {
    m_DumpDir = gitsConfig.common.player.subcapturePath;
    m_Frames = BitRange(gitsConfig.directx.features.subcapture.frames);
    if (gitsConfig.directx.features.subcapture.commandListExecutions.empty()) {
      m_Executions = BitRange("all");
    } else {
      m_Executions = BitRange(gitsConfig.directx.features.subcapture.commandListExecutions);
    }
  } else {
    m_DumpDir = gitsConfig.common.player.streamDir;
    m_Frames = BitRange("all");
    m_Executions = BitRange("all");
  }
  m_DumpDir /= "cpu_patch";
  m_DumpDir /= "instances_dump";
  LOG_INFO << "CpuPatch - InstancesDump location: " << m_DumpDir.string();
}

InstancesDump::~InstancesDump() {
  WaitUntilDumped();
  LOG_INFO << "CpuPatch - InstancesDump - files dumped: " << std::to_string(m_NumFiles)
           << ", total size: " << FormatMemorySize(m_FilesTotalSize);
}

void InstancesDump::BuildTlas(ID3D12GraphicsCommandList* commandList,
                              ID3D12Resource* resource,
                              unsigned offset,
                              unsigned size,
                              D3D12_RESOURCE_STATES state,
                              unsigned commandKey) {
  Initialize();

  InstancesDumpInfo* info = new InstancesDumpInfo();
  info->Offset = offset;
  info->Size = size;
  info->DumpName = (m_DumpDir / std::to_string(commandKey)).wstring();
  StageResource(commandList, resource, state, *info);
}

void InstancesDump::ExecuteCommandLists(unsigned key,
                                        unsigned commandQueueKey,
                                        ID3D12CommandQueue* commandQueue,
                                        ID3D12CommandList** commandLists,
                                        unsigned commandListNum,
                                        unsigned frameCount,
                                        unsigned executeCount) {
  for (unsigned i = 0; i < commandListNum; ++i) {
    auto it = m_StagedResources.find(commandLists[i]);
    if (it != m_StagedResources.end()) {
      for (DumpInfo* dumpInfo : it->second) {
        auto* info = static_cast<InstancesDumpInfo*>(dumpInfo);
        info->FrameCount = frameCount;
        info->ExecuteCount = executeCount;
      }
    }
  }

  ResourceDump::ExecuteCommandLists(key, commandQueueKey, commandQueue, commandLists,
                                    commandListNum);
}

void InstancesDump::DumpStagedResource(DumpInfo& dumpInfo) {
  {
    const auto& info = static_cast<InstancesDumpInfo&>(dumpInfo);
    if (!m_Frames[info.FrameCount] || !m_Executions[info.ExecuteCount]) {
      return;
    }
  }

  void* data{};
  HRESULT hr = dumpInfo.StagingBuffer->Map(0, nullptr, &data);

  std::ofstream stream(dumpInfo.DumpName, std::ios_base::binary);
  stream.write(static_cast<char*>(data), dumpInfo.Size);

  dumpInfo.StagingBuffer->Unmap(0, nullptr);

  ++m_NumFiles;
  m_FilesTotalSize += dumpInfo.Size;
}

void InstancesDump::Initialize() {
  if (m_Initialized) {
    return;
  }

  if (!m_DumpDir.empty() && !std::filesystem::exists(m_DumpDir)) {
    std::filesystem::create_directories(m_DumpDir);
  }
  m_Initialized = true;
}

} // namespace DirectX
} // namespace gits
