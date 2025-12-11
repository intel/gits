// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourceDump.h"

#include <unordered_set>

namespace gits {
namespace DirectX {

class AnalyzerExecuteIndirectService;

class AnalyzerExecuteIndirectDump : public ResourceDump {
public:
  AnalyzerExecuteIndirectDump(AnalyzerExecuteIndirectService& executeIndirectService)
      : executeIndirectService_(executeIndirectService) {}
  void dumpArgumentBuffer(ID3D12GraphicsCommandList* commandList,
                          const D3D12_COMMAND_SIGNATURE_DESC* commandSignature,
                          unsigned maxCommandCount,
                          ID3D12Resource* argumentBuffer,
                          unsigned argumentBufferOffset,
                          ID3D12Resource* countBuffer,
                          unsigned countBufferOffset);

  std::unordered_set<unsigned>& getArgumentBuffersResources() {
    return argumentBuffersResources_;
  }

private:
  struct ExecuteIndirectDumpInfo : public DumpInfo {
    const D3D12_COMMAND_SIGNATURE_DESC* commandSignature{};
    DumpInfo countDumpInfo;
  };

  void dumpStagedResource(DumpInfo& dumpInfo) override;
  void dumpArgumentBuffer(ExecuteIndirectDumpInfo& dumpInfo, unsigned argumentCount, void* data);

private:
  AnalyzerExecuteIndirectService& executeIndirectService_;
  std::mutex mutex_;
  std::unordered_set<unsigned> argumentBuffersResources_;
};

} // namespace DirectX
} // namespace gits
