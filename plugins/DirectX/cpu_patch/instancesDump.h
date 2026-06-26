// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "configurationLib.h"
#include "resourceDump.h"
#include "bit_range.h"

#include <filesystem>

namespace gits {
namespace DirectX {

class InstancesDump : public ResourceDump {
public:
  InstancesDump(const Configuration& gitsConfig);
  ~InstancesDump();

  InstancesDump(const InstancesDump&) = delete;
  InstancesDump& operator=(const InstancesDump&) = delete;

  void BuildTlas(ID3D12GraphicsCommandList* commandList,
                 ID3D12Resource* resource,
                 unsigned offset,
                 unsigned size,
                 D3D12_RESOURCE_STATES state,
                 unsigned commandKey);
  void ExecuteCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum,
                           unsigned frameCount,
                           unsigned executeCount);

protected:
  void DumpStagedResource(DumpInfo& dumpInfo);

private:
  struct InstancesDumpInfo : DumpInfo {
    unsigned FrameCount{};
    unsigned ExecuteCount{};
  };

  void Initialize();

  BitRange m_Frames;
  BitRange m_Executions;
  bool m_Initialized{false};
  std::filesystem::path m_DumpDir;
  size_t m_NumFiles{};
  size_t m_FilesTotalSize{};
};

} // namespace DirectX
} // namespace gits
