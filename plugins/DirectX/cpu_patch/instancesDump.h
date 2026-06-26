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

  void buildTlas(ID3D12GraphicsCommandList* commandList,
                 ID3D12Resource* resource,
                 unsigned offset,
                 unsigned size,
                 D3D12_RESOURCE_STATES state,
                 unsigned commandKey);
  void executeCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum,
                           unsigned frameCount,
                           unsigned executeCount);

protected:
  void dumpStagedResource(DumpInfo& dumpInfo);

private:
  struct InstancesDumpInfo : DumpInfo {
    unsigned frameCount{};
    unsigned executeCount{};
  };

  void initialize();

  BitRange frames_;
  BitRange executions_;
  bool initialized_{false};
  std::filesystem::path dumpDir_;
  size_t numFiles_{};
  size_t filesTotalSize_{};
};

} // namespace DirectX
} // namespace gits
