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

  void NotifyObject(ObjectKey objectKey);
  void NotifyObjects(const std::vector<ObjectKey>& objectKeys);

  void CommandListCommand(ObjectKey commandListKey);
  void Present(CommandKey callKey, ObjectKey swapChainKey);
  void ExecuteCommandLists(CommandKey callKey,
                           ObjectKey commandQueueKey,
                           std::vector<ObjectKey>& commandListKeys);
  void CommandListReset(ObjectKey commandListKey,
                        ObjectKey allocatorKey,
                        ObjectKey initialStateKey);
  void StateRestoreBegin();
  void StateRestoreEnd();
  void ExecutionStart();
  void ExecutionEnd();
  void CommandQueueWait(CommandKey callKey,
                        ObjectKey commandQueueKey,
                        ObjectKey fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(CommandKey callKey,
                          ObjectKey commandQueueKey,
                          ObjectKey fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(CommandKey callKey, ObjectKey fenceKey, UINT64 fenceValue);
  void MappedDataMeta(ObjectKey resourceKey);
  void CreateXessContext(xessD3D12CreateContextCommand& c);
  void CreateXellContext(xellD3D12CreateContextCommand& c);
  void CreateXefgContext(xefgSwapChainD3D12CreateContextCommand& c);
  void ForceApplicationSwapChainRestore(ObjectKey key);
  void CreateDeviceExtensionContext(INTC_D3D12_CreateDeviceExtensionContextCommand& c);
  void CreateDeviceExtensionContext(INTC_D3D12_CreateDeviceExtensionContext1Command& c);
  void CreateDeviceExtensionContext(INTC_D3D12_CreateDeviceExtensionContext2Command& c);

  void AddParent(ObjectKey key, ObjectKey parentKey);

private:
  void FindParents(ObjectKey key, std::set<ObjectKey>& objectKeys);
  void ClearReadyExecutables();
  void DumpAnalysisFile();

private:
  SubcaptureRange& m_SubcaptureRange;
  AnalyzerCommandListService& m_CommandListService;
  AnalyzerRaytracingService& m_RaytracingService;
  AnalyzerExecuteIndirectService& m_ExecuteIndirectService;
  RaytracingOptimizationService& m_RaytracingOptimizationService;
  bool m_Optimize{};

  std::unordered_map<ObjectKey, std::vector<ObjectKey>> m_ParentKeys;

  struct ExecuteCommandListCommand : public GpuExecutionTracker::Executable {
    std::vector<ObjectKey> CommandListKeys;
  };

  GpuExecutionTracker m_GpuExecutionTracker;
  bool m_BeforeRange{true};
  bool m_InRange{};
  bool m_InsideExecution{};
  bool m_StateRestore{};

  std::set<ObjectKey> m_CommandListsResetBeforeExecution;
  std::set<ObjectKey> m_CommandListsExecuted;
  std::set<ObjectKey> m_CommandListsReset;
  std::set<ObjectKey> m_CommandListsForRestore;

  std::map<CommandKey, std::vector<ObjectKey>> m_CommandQueueCommandsForRestore;

  std::set<ObjectKey> m_ObjectsForRestore;
};

} // namespace DirectX
} // namespace gits
