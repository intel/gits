// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "gpuExecutionTracker.h"
#include "analyzerCommandListService.h"
#include "analyzerRaytracingService.h"
#include "analyzerExecuteIndirectService.h"
#include "subcaptureRange.h"

#include <set>
#include <string>
#include <vector>

namespace gits {
namespace DirectX {

class AnalyzerService {
public:
  AnalyzerService(SubcaptureRange& subcaptureRange,
                  AnalyzerCommandListService& commandListService,
                  AnalyzerRaytracingService& raytracingService,
                  AnalyzerExecuteIndirectService& executeIndirectService);
  ~AnalyzerService();
  AnalyzerService(const AnalyzerService&) = delete;
  AnalyzerService& operator=(const AnalyzerService&) = delete;

  bool InRange() {
    return m_InRange;
  }
  bool BeforeRange() {
    return m_BeforeRange;
  }
  bool AfterRange() {
    return !m_BeforeRange && !m_InRange;
  }

  void NotifyObject(unsigned objectKey);
  void NotifyObjects(const std::vector<unsigned>& objectKeys);

  void CommandListCommand(unsigned commandListKey);
  void Present(unsigned callKey, unsigned swapChainKey);
  void ExecuteCommandLists(unsigned callKey,
                           unsigned commandQueueKey,
                           std::vector<unsigned>& commandListKeys);
  void CommandListReset(unsigned commandListKey, unsigned allocatorKey, unsigned initialStateKey);
  void ExecutionStart();
  void ExecutionEnd();
  void CommandQueueWait(unsigned callKey,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(unsigned callKey,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue);
  void MappedDataMeta(unsigned resourceKey);
  void CreateXessContext(xessD3D12CreateContextCommand& c);
  void CreateXellContext(xellD3D12CreateContextCommand& c);
  void CreateXefgContext(xefgSwapChainD3D12CreateContextCommand& c);
  void ForceApplicationSwapChainRestore(unsigned key);
  void CreateDeviceExtensionContext(INTC_D3D12_CreateDeviceExtensionContextCommand& c);
  void CreateDeviceExtensionContext(INTC_D3D12_CreateDeviceExtensionContext1Command& c);

  void AddParent(unsigned key, unsigned parentKey);

private:
  void FindParents(unsigned key, std::set<unsigned>& objectKeys);
  void ClearReadyExecutables();
  void DumpAnalysisFile();

private:
  SubcaptureRange& m_SubcaptureRange;
  AnalyzerCommandListService& m_CommandListService;
  AnalyzerRaytracingService& m_RaytracingService;
  AnalyzerExecuteIndirectService& m_ExecuteIndirectService;
  bool m_Optimize{};

  std::unordered_map<unsigned, std::vector<unsigned>> m_ParentKeys;

  struct ExecuteCommandListCommand : public GpuExecutionTracker::Executable {
    std::vector<unsigned> CommandListKeys;
  };

  GpuExecutionTracker m_GpuExecutionTracker;
  bool m_BeforeRange{true};
  bool m_InRange{};

  std::set<unsigned> m_CommandListsResetBeforeExecution;
  std::set<unsigned> m_CommandListsExecuted;
  std::set<unsigned> m_CommandListsReset;
  std::set<unsigned> m_CommandListsForRestore;

  std::map<unsigned, std::vector<unsigned>> m_CommandQueueCommandsForRestore;

  std::set<unsigned> m_ObjectsForRestore;
};

} // namespace DirectX
} // namespace gits
