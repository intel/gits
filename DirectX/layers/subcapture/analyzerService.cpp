// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerService.h"
#include "analyzerResults.h"
#include "gits.h"

#include <fstream>
#include <sstream>

namespace gits {
namespace DirectX {

AnalyzerService::AnalyzerService(SubcaptureRange& subcaptureRange,
                                 BindingService& bindingService,
                                 AnalyzerRaytracingService& raytracingService)
    : subcaptureRange_(subcaptureRange),
      bindingService_(bindingService),
      raytracingService_(raytracingService) {}

AnalyzerService::~AnalyzerService() {
  try {
    if (inRange_) {
      Log(ERR) << "Subcapture analysis terminated prematurely";
      dumpAnalysisFile();
    }
  } catch (...) {
    topmost_exception_handler("AnalyzerService::~AnalyzerService");
  }
}

void AnalyzerService::notifyObject(unsigned objectKey) {
  if (inRange_) {
    objectsForRestore_.insert(objectKey);
  }
}

void AnalyzerService::commandListCommand(unsigned commandListKey) {
  if (subcaptureRange_.commandListSubcapture()) {
    return;
  }
  if (inRange_) {
    auto it = commandListsReset_.find(commandListKey);
    if (it == commandListsReset_.end()) {
      commandListsForRestore_.insert(commandListKey);
    }
  }
}

void AnalyzerService::present(unsigned callKey, unsigned swapChainKey) {
  if (subcaptureRange_.commandListSubcapture()) {
    objectsForRestore_.insert(swapChainKey);
    subcaptureRange_.frameEnd(callKey & Command::stateRestoreKeyMask);
    return;
  }

  if (subcaptureRange_.isFrameRangeStart()) {
    inRange_ = true;
    beforeRange_ = false;
    clearReadyExecutables();
    auto& queueEvents = gpuExecutionTracker_.getQueueEvents();
    for (auto& commandQueue : queueEvents) {
      for (GpuExecutionTracker::QueueEvent* event : commandQueue.second) {
        commandQueueCommandsForRestore_.insert(event->callKey);
        ExecuteCommandListCommand* execute = dynamic_cast<ExecuteCommandListCommand*>(event);
        if (execute) {
          for (unsigned commandListKey : execute->commandListKeys) {
            commandListsForRestore_.insert(commandListKey);
          }
        }
        delete event;
      }
    }
    subcaptureRange_.frameEnd(callKey & Command::stateRestoreKeyMask);
    return;
  }

  subcaptureRange_.frameEnd(callKey & Command::stateRestoreKeyMask);
  if (!subcaptureRange_.inRange() && inRange_) {
    bindingService_.commandListsRestore(commandListsForRestore_);
    inRange_ = false;
    dumpAnalysisFile();
  }
}

void AnalyzerService::executeCommandLists(unsigned callKey,
                                          unsigned commandQueueKey,
                                          std::vector<unsigned>& commandListKeys) {
  if (subcaptureRange_.commandListSubcapture()) {
    return;
  }
  if (beforeRange_) {
    clearReadyExecutables();
    if (gpuExecutionTracker_.isCommandQueueWaiting(commandQueueKey)) {
      ExecuteCommandListCommand* executable = new ExecuteCommandListCommand{};
      executable->commandListKeys = commandListKeys;
      gpuExecutionTracker_.execute(callKey, commandQueueKey, executable);
    }
  } else if (inRange_) {
    for (unsigned commandListKey : commandListKeys) {
      auto it = commandListsResetBeforeExecution_.find(commandListKey);
      if (it == commandListsResetBeforeExecution_.end()) {
        commandListsForRestore_.insert(commandListKey);
      }
      commandListsExecuted_.insert(commandListKey);
    }
  }
}

void AnalyzerService::commandListReset(unsigned commandListKey,
                                       unsigned allocatorKey,
                                       unsigned initialStateKey) {
  if (subcaptureRange_.commandListSubcapture()) {
    if (inRange_) {
      objectsForRestore_.insert(commandListKey);
      objectsForRestore_.insert(allocatorKey);
      objectsForRestore_.insert(initialStateKey);
    }
    return;
  }
  if (inRange_) {
    auto it = commandListsExecuted_.find(commandListKey);
    if (it == commandListsExecuted_.end()) {
      commandListsResetBeforeExecution_.insert(commandListKey);
    }
    commandListsReset_.insert(commandListKey);
  }
}

void AnalyzerService::executionStart() {
  if (subcaptureRange_.isExecutionRangeStart()) {
    inRange_ = true;
    beforeRange_ = false;
  }
}

void AnalyzerService::executionEnd() {
  if (subcaptureRange_.commandListSubcapture() && !subcaptureRange_.inRange() && inRange_) {
    inRange_ = false;
    dumpAnalysisFile();
  }
}

void AnalyzerService::commandQueueWait(unsigned callKey,
                                       unsigned commandQueueKey,
                                       unsigned fenceKey,
                                       UINT64 fenceValue) {
  if (beforeRange_) {
    gpuExecutionTracker_.commandQueueWait(callKey, commandQueueKey, fenceKey, fenceValue);
  }
}

void AnalyzerService::commandQueueSignal(unsigned callKey,
                                         unsigned commandQueueKey,
                                         unsigned fenceKey,
                                         UINT64 fenceValue) {
  if (beforeRange_) {
    gpuExecutionTracker_.commandQueueSignal(callKey, commandQueueKey, fenceKey, fenceValue);
  }
}

void AnalyzerService::fenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue) {
  if (beforeRange_) {
    gpuExecutionTracker_.fenceSignal(callKey, fenceKey, fenceValue);
  }
}

void AnalyzerService::copyTileMappings(unsigned callKey, unsigned commandQueueKey) {
  if (beforeRange_) {
    GpuExecutionTracker::Executable* executable = new GpuExecutionTracker::Executable{};
    gpuExecutionTracker_.execute(callKey, commandQueueKey, executable);
  }
}

void AnalyzerService::updateTileMappings(unsigned callKey, unsigned commandQueueKey) {
  if (beforeRange_) {
    GpuExecutionTracker::Executable* executable = new GpuExecutionTracker::Executable{};
    gpuExecutionTracker_.execute(callKey, commandQueueKey, executable);
  }
}

void AnalyzerService::clearReadyExecutables() {
  std::vector<GpuExecutionTracker::Executable*>& executables =
      gpuExecutionTracker_.getReadyExecutables();
  for (GpuExecutionTracker::Executable* executable : executables) {
    delete executable;
  }
  executables.clear();
}

void AnalyzerService::dumpAnalysisFile() {
  std::ofstream out(AnalyzerResults::getAnalysisFileName());
  out << "COMMAND_LIST_KEYS\n";
  for (unsigned key : commandListsForRestore_) {
    out << key << "\n";
  }
  out << "COMMAND_QUEUE_COMMANDS\n";
  for (unsigned key : commandQueueCommandsForRestore_) {
    out << key << "\n";
  }
  out << "OBJECTS\n";
  std::set<unsigned> keys;
  for (unsigned key : objectsForRestore_) {
    keys.insert(key);
  }
  for (unsigned key : bindingService_.getObjectsForRestore()) {
    keys.insert(key);
  }
  for (unsigned key : keys) {
    if (key) {
      out << key << "\n";
    }
  }
  out << "DESCRIPTORS\n";
  for (auto& [heapKey, index] : bindingService_.getDescriptors()) {
    out << heapKey << " " << index << "\n";
  }
  out << "ACCELERATION_STRUCTURES\n";
  std::set<std::pair<unsigned, unsigned>> ases;
  raytracingService_.flush();
  AnalyzerRaytracingService::BlasesByTlas& blasesByTlas = raytracingService_.getBlases();
  for (auto& itTlas : blasesByTlas) {
    for (auto& blas : itTlas.second) {
      ases.insert(blas);
    }
  }
  std::set<std::pair<unsigned, unsigned>>& tlases = raytracingService_.getTlases();
  for (auto& it : tlases) {
    ases.insert(it);
  }
  std::set<std::pair<unsigned, unsigned>>& sources = raytracingService_.getSources();
  for (auto& it : sources) {
    ases.insert(it);
  }
  for (auto& [resourceKey, offset] : ases) {
    out << resourceKey << " " << offset << "\n";
  }
}

} // namespace DirectX
} // namespace gits
