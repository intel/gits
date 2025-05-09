// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandWriter.h"
#include "executionSerializationRecorder.h"
#include "gpuExecutionTracker.h"
#include "command.h"

#include <unordered_map>
#include <vector>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class CommandListExecutionService {
public:
  CommandListExecutionService(ExecutionSerializationRecorder& recorder) : recorder_(recorder) {}
  void commandListCommand(unsigned commandListKey, CommandWriter* command);
  void executeCommandLists(unsigned callKey,
                           unsigned commandQueueKey,
                           std::vector<unsigned>& commandListKeys);
  void createCommandList(unsigned commandListKey, unsigned allocatorKey);
  void commandAllocatorReset(unsigned allocatorKey);
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
  void createCommandQueue(unsigned deviceKey, unsigned commandQueueKey);

private:
  struct ExecuteCommandLists : public GpuExecutionTracker::Executable {
    unsigned callKey;
    unsigned commandQueueKey;
    std::vector<std::pair<unsigned, std::vector<CommandWriter*>>> commandsByCommandList;
  };

private:
  void executeReadyExecutables();
  void executeExecutable(ExecuteCommandLists& executeCommandLists);

private:
  ExecutionSerializationRecorder& recorder_;
  GpuExecutionTracker executionTracker_;
  std::unordered_map<unsigned, std::vector<CommandWriter*>> commandsByCommandList_;
  std::unordered_map<unsigned, unsigned> deviceByCommandQueue_;
  std::unordered_map<unsigned, std::pair<unsigned, UINT64>> fenceByCommandQueue_;
  std::unordered_map<unsigned, unsigned> commandListByAllocator_;
  unsigned restoreCommandKey_{Command::executionSerializationKeyMask};
  unsigned restoreObjectKey_{Command::executionSerializationKeyMask};
};

} // namespace DirectX
} // namespace gits
