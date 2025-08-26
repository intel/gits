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
#include "log2.h"

#include <fstream>
#include <sstream>

namespace gits {
namespace DirectX {

AnalyzerService::AnalyzerService(SubcaptureRange& subcaptureRange,
                                 BindingService& bindingService,
                                 AnalyzerCommandListRestoreService& commandListRestoreService,
                                 AnalyzerRaytracingService& raytracingService)
    : subcaptureRange_(subcaptureRange),
      bindingService_(bindingService),
      commandListRestoreService_(commandListRestoreService),
      raytracingService_(raytracingService) {
  optimize_ = Configurator::Get().directx.features.subcapture.optimize;
}

AnalyzerService::~AnalyzerService() {
  try {
    if (inRange_) {
      LOG_ERROR << "Subcapture analysis terminated prematurely";
      dumpAnalysisFile();
    }
  } catch (...) {
    topmost_exception_handler("AnalyzerService::~AnalyzerService");
  }
}

void AnalyzerService::notifyObject(unsigned objectKey) {
  if (optimize_ && inRange_) {
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
    commandListRestoreService_.commandListsRestore(commandListsForRestore_);
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

void AnalyzerService::mappedDataMeta(unsigned resourceKey) {
  if (inRange_) {
    objectsForRestore_.insert(resourceKey);
  }
}

void AnalyzerService::createXessContext(xessD3D12CreateContextCommand& c) {
  objectsForRestore_.insert(c.phContext_.key);
}

void AnalyzerService::createDeviceExtensionContext(
    INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  objectsForRestore_.insert(c.ppExtensionContext_.key);
}

void AnalyzerService::createDeviceExtensionContext(
    INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  objectsForRestore_.insert(c.ppExtensionContext_.key);
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
  for (unsigned key : commandListRestoreService_.getObjectsForRestore()) {
    keys.insert(key);
  }
  for (unsigned key : bindingService_.getBindingTablesResources()) {
    keys.insert(key);
  }
  for (unsigned key : keys) {
    if (key) {
      out << key << "\n";
    }
  }
  out << "DESCRIPTORS\n";
  std::set<std::pair<unsigned, unsigned>> descriptors;
  for (auto& [heapKey, index] : bindingService_.getDescriptors()) {
    descriptors.insert({heapKey, index});
  }
  for (auto& [heapKey, index] : bindingService_.getBindingTablesDescriptors()) {
    descriptors.insert({heapKey, index});
  }
  for (auto& [heapKey, index] : descriptors) {
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
