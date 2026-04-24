// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
      : m_ExecuteIndirectService(executeIndirectService) {}
  void DumpArgumentBuffer(ID3D12GraphicsCommandList* commandList,
                          const D3D12_COMMAND_SIGNATURE_DESC* commandSignature,
                          unsigned maxCommandCount,
                          ID3D12Resource* argumentBuffer,
                          unsigned argumentBufferOffset,
                          ID3D12Resource* countBuffer,
                          unsigned countBufferOffset);

  std::unordered_set<unsigned>& GetArgumentBuffersResources() {
    return m_ArgumentBuffersResources;
  }

private:
  struct ExecuteIndirectDumpInfo : public DumpInfo {
    const D3D12_COMMAND_SIGNATURE_DESC* commandSignature{};
    DumpInfo countDumpInfo;
  };

  void dumpStagedResource(DumpInfo& dumpInfo) override;
  void DumpArgumentBuffer(ExecuteIndirectDumpInfo& dumpInfo, unsigned argumentCount, void* data);

private:
  AnalyzerExecuteIndirectService& m_ExecuteIndirectService;
  std::mutex m_Mutex;
  std::unordered_set<unsigned> m_ArgumentBuffersResources;
};

} // namespace DirectX
} // namespace gits
