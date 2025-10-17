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
                                 AnalyzerCommandListService& commandListService,
                                 AnalyzerRaytracingService& raytracingService)
    : subcaptureRange_(subcaptureRange),
      commandListService_(commandListService),
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
    if (objectKey) {
      objectsForRestore_.insert(objectKey);
    }
  }
}

void AnalyzerService::notifyObjects(const std::vector<unsigned>& objectKeys) {
  if (optimize_ && inRange_) {
    for (unsigned key : objectKeys) {
      if (key) {
        objectsForRestore_.insert(key);
      }
    }
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

  if (subcaptureRange_.isFrameRangeStart(callKey & Command::stateRestoreKeyMask)) {
    inRange_ = true;
    beforeRange_ = false;
    clearReadyExecutables();
    auto& queueEvents = gpuExecutionTracker_.getQueueEvents();
    for (auto& commandQueue : queueEvents) {
      for (GpuExecutionTracker::QueueEvent* event : commandQueue.second) {
        std::vector<unsigned>& objectKeys = commandQueueCommandsForRestore_[event->callKey];
        if (event->commandQueueKey) {
          objectKeys.push_back(event->commandQueueKey);
        }
        switch (event->type) {
        case GpuExecutionTracker::QueueEvent::Execute: {
          auto* execute = static_cast<ExecuteCommandListCommand*>(event);
          for (unsigned commandListKey : execute->commandListKeys) {
            commandListsForRestore_.insert(commandListKey);
          }
        } break;
        case GpuExecutionTracker::QueueEvent::Signal: {
          auto* signal = static_cast<GpuExecutionTracker::SignalEvent*>(event);
          objectKeys.push_back(signal->fence.key);
        } break;
        case GpuExecutionTracker::QueueEvent::Wait: {
          auto* wait = static_cast<GpuExecutionTracker::WaitEvent*>(event);
          objectKeys.push_back(wait->fence.key);
        } break;
        }
        delete event;
      }
    }
    subcaptureRange_.frameEnd(callKey & Command::stateRestoreKeyMask);
    return;
  }

  subcaptureRange_.frameEnd(callKey & Command::stateRestoreKeyMask);
  if (!subcaptureRange_.inRange() && inRange_) {
    commandListService_.commandListsRestore(commandListsForRestore_);
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

  std::set<unsigned> objectKeys;

  out << "COMMAND_LIST_KEYS\n";
  for (unsigned key : commandListsForRestore_) {
    out << key << "\n";
    if (optimize_) {
      objectKeys.insert(key);
    }
  }

  out << "COMMAND_QUEUE_COMMANDS\n";
  for (auto& it : commandQueueCommandsForRestore_) {
    out << it.first << "\n";
    if (optimize_) {
      for (unsigned key : it.second) {
        objectKeys.insert(key);
      }
    }
  }

  out << "OBJECTS\n";
  for (unsigned key : objectsForRestore_) {
    objectKeys.insert(key);
  }
  for (unsigned key : commandListService_.getObjectsForRestore()) {
    objectKeys.insert(key);
  }
  for (unsigned key : commandListService_.getBindingTablesResources()) {
    objectKeys.insert(key);
  }
  for (unsigned key : objectKeys) {
    if (key) {
      out << key << "\n";
    }
  }

  out << "DESCRIPTORS\n";
  std::set<std::pair<unsigned, unsigned>> descriptors;
  for (auto& [heapKey, index] : commandListService_.getDescriptors()) {
    descriptors.insert({heapKey, index});
  }
  for (auto& [heapKey, index] : commandListService_.getBindingTablesDescriptors()) {
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
