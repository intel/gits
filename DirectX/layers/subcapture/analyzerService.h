// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "commandsAuto.h"
#include "gpuExecutionTracker.h"
#include "analyzerCommandListService.h"
#include "analyzerRaytracingService.h"
#include "analyzerExecuteIndirectService.h"
#include "subcaptureRange.h"
#include "raytracingOptimizationService.h"

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
                  AnalyzerExecuteIndirectService& executeIndirectService,
                  RaytracingOptimizationService& raytracingOptimizationService);
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

  void NotifyObject(GITSKey objectKey);
  void NotifyObjects(const std::vector<GITSKey>& objectKeys);

  void CommandListCommand(GITSKey commandListKey);
  void Present(CommandKey callKey, GITSKey swapChainKey);
  void ExecuteCommandLists(CommandKey callKey,
                           GITSKey commandQueueKey,
                           std::vector<GITSKey>& commandListKeys);
  void CommandListReset(GITSKey commandListKey, GITSKey allocatorKey, GITSKey initialStateKey);
  void ExecutionStart();
  void ExecutionEnd();
  void CommandQueueWait(CommandKey callKey,
                        GITSKey commandQueueKey,
                        GITSKey fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(CommandKey callKey,
                          GITSKey commandQueueKey,
                          GITSKey fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(CommandKey callKey, GITSKey fenceKey, UINT64 fenceValue);
  void MappedDataMeta(GITSKey resourceKey);
  void CreateXessContext(xessD3D12CreateContextCommand& c);
  void CreateXellContext(xellD3D12CreateContextCommand& c);
  void CreateXefgContext(xefgSwapChainD3D12CreateContextCommand& c);
  void ForceApplicationSwapChainRestore(GITSKey key);
  void CreateDeviceExtensionContext(INTC_D3D12_CreateDeviceExtensionContextCommand& c);
  void CreateDeviceExtensionContext(INTC_D3D12_CreateDeviceExtensionContext1Command& c);
  void CreateDeviceExtensionContext(INTC_D3D12_CreateDeviceExtensionContext2Command& c);

  void AddParent(GITSKey key, GITSKey parentKey);

private:
  void FindParents(GITSKey key, std::set<GITSKey>& objectKeys);
  void ClearReadyExecutables();
  void DumpAnalysisFile();

private:
  SubcaptureRange& m_SubcaptureRange;
  AnalyzerCommandListService& m_CommandListService;
  AnalyzerRaytracingService& m_RaytracingService;
  AnalyzerExecuteIndirectService& m_ExecuteIndirectService;
  RaytracingOptimizationService& m_RaytracingOptimizationService;
  bool m_Optimize{};

  std::unordered_map<GITSKey, std::vector<GITSKey>> m_ParentKeys;

  struct ExecuteCommandListCommand : public GpuExecutionTracker::Executable {
    std::vector<GITSKey> CommandListKeys;
  };

  GpuExecutionTracker m_GpuExecutionTracker;
  bool m_BeforeRange{true};
  bool m_InRange{};

  std::set<GITSKey> m_CommandListsResetBeforeExecution;
  std::set<GITSKey> m_CommandListsExecuted;
  std::set<GITSKey> m_CommandListsReset;
  std::set<GITSKey> m_CommandListsForRestore;

  std::map<CommandKey, std::vector<GITSKey>> m_CommandQueueCommandsForRestore;

  std::set<GITSKey> m_ObjectsForRestore;
};

} // namespace DirectX
} // namespace gits
