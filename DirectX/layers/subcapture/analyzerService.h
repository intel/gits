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

#include <set>
#include <string>

namespace gits {
namespace DirectX {

class AnalyzerService {
public:
  AnalyzerService();
  ~AnalyzerService();

  AnalyzerService(const AnalyzerService&) = delete;
  AnalyzerService& operator=(const AnalyzerService&) = delete;

  void commandListCommand(unsigned commandListKey);
  void present(unsigned callKey);
  void executeCommandLists(unsigned callKey,
                           unsigned commandQueueKey,
                           std::vector<unsigned>& commandListKeys);
  void commandListReset(unsigned commandListKey);
  void commandQueueWait(unsigned callKey,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void commandQueueSignal(unsigned callKey,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void fenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue);
  void copyTileMappings(unsigned callKey, unsigned commandQueueKey);
  void updateTileMappings(unsigned callKey, unsigned commandQueueKey);

private:
  void clearReadyExecutables();
  void dumpAnalysisFile();

private:
  struct ExecuteCommandListCommand : public GpuExecutionTracker::Executable {
    std::vector<unsigned> commandListKeys;
  };

  GpuExecutionTracker gpuExecutionTracker_;
  bool inRange_{};
  bool beforeRange_{true};
  unsigned startFrame_;
  unsigned endFrame_;

  std::set<unsigned> commandListsResetBeforeExecution_;
  std::set<unsigned> commandListsExecuted_;
  std::set<unsigned> commandListsReset_;
  std::set<unsigned> commandListsForRestore_;

  std::set<unsigned> commandQueueCommandsForRestore_;
};

} // namespace DirectX
} // namespace gits
