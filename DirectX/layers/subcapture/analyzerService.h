// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "gpuExecutionTracker.h"
#include "bindingService.h"
#include "analyzerCommandListRestoreService.h"
#include "analyzerRaytracingService.h"
#include "subcaptureRange.h"

#include <set>
#include <string>

namespace gits {
namespace DirectX {

class AnalyzerService {
public:
  AnalyzerService(SubcaptureRange& subcaptureRange,
                  BindingService& bindingService,
                  AnalyzerCommandListRestoreService& commandListRestoreService,
                  AnalyzerRaytracingService& raytracingService);
  ~AnalyzerService();
  AnalyzerService(const AnalyzerService&) = delete;
  AnalyzerService& operator=(const AnalyzerService&) = delete;

  bool inRange() {
    return inRange_;
  }

  void notifyObject(unsigned objectKey);

  void commandListCommand(unsigned commandListKey);
  void present(unsigned callKey, unsigned swapChainKey);
  void executeCommandLists(unsigned callKey,
                           unsigned commandQueueKey,
                           std::vector<unsigned>& commandListKeys);
  void commandListReset(unsigned commandListKey, unsigned allocatorKey, unsigned initialStateKey);
  void executionStart();
  void executionEnd();
  void commandQueueWait(unsigned callKey,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void commandQueueSignal(unsigned callKey,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void fenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue);
  void mappedDataMeta(unsigned resourceKey);
  void createXessContext(xessD3D12CreateContextCommand& c);
  void createDeviceExtensionContext(INTC_D3D12_CreateDeviceExtensionContextCommand& c);
  void createDeviceExtensionContext(INTC_D3D12_CreateDeviceExtensionContext1Command& c);

private:
  void clearReadyExecutables();
  void dumpAnalysisFile();

private:
  SubcaptureRange& subcaptureRange_;
  BindingService& bindingService_;
  AnalyzerCommandListRestoreService& commandListRestoreService_;
  AnalyzerRaytracingService& raytracingService_;
  bool optimize_{};

  struct ExecuteCommandListCommand : public GpuExecutionTracker::Executable {
    std::vector<unsigned> commandListKeys;
  };

  GpuExecutionTracker gpuExecutionTracker_;
  bool beforeRange_{true};
  bool inRange_{};

  std::set<unsigned> commandListsResetBeforeExecution_;
  std::set<unsigned> commandListsExecuted_;
  std::set<unsigned> commandListsReset_;
  std::set<unsigned> commandListsForRestore_;

  std::set<unsigned> commandQueueCommandsForRestore_;

  std::set<unsigned> objectsForRestore_;
};

} // namespace DirectX
} // namespace gits
