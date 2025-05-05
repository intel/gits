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

AnalyzerService::AnalyzerService() {
  const std::string& frames = Configurator::Get().directx.features.subcapture.frames;
  size_t pos = frames.find("-");
  try {
    if (pos != std::string::npos) {
      startFrame_ = std::stoi(frames.substr(0, pos));
      endFrame_ = std::stoi(frames.substr(pos + 1));
    } else {
      startFrame_ = std::stoi(frames);
      endFrame_ = startFrame_;
    }
  } catch (...) {
    throw Exception("Invalid subcapture range: '" +
                    Configurator::Get().directx.features.subcapture.frames + "'");
  }
}

void AnalyzerService::commandListCommand(unsigned commandListKey) {
  if (inRange_) {
    auto it = commandListsReset_.find(commandListKey);
    if (it == commandListsReset_.end()) {
      commandListsForRestore_.insert(commandListKey);
    }
  }
}

void AnalyzerService::present(unsigned callKey) {
  unsigned currentFrame = CGits::Instance().CurrentFrame();
  if (currentFrame == startFrame_ - 1) {
    Log(INFOV) << "Start analysis frame " << currentFrame + 1 << " call " << callKey;
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
  } else if (currentFrame == endFrame_) {
    if (inRange_) {
      inRange_ = false;
      std::ofstream out(AnalyzerResults::getAnalysisFileName());
      out << "COMMAND_LIST_KEYS\n";
      for (unsigned key : commandListsForRestore_) {
        out << key << "\n";
      }
      out << "COMMAND_QUEUE_COMMANDS\n";
      for (unsigned key : commandQueueCommandsForRestore_) {
        out << key << "\n";
      }
    }
  }
}

void AnalyzerService::executeCommandLists(unsigned callKey,
                                          unsigned commandQueueKey,
                                          std::vector<unsigned>& commandListKeys) {
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

void AnalyzerService::commandListReset(unsigned commandListKey) {
  if (inRange_) {
    auto it = commandListsExecuted_.find(commandListKey);
    if (it == commandListsExecuted_.end()) {
      commandListsResetBeforeExecution_.insert(commandListKey);
    }
    commandListsReset_.insert(commandListKey);
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

} // namespace DirectX
} // namespace gits
