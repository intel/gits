// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandListExecutionService.h"
#include "commandsAuto.h"
#include "commandWritersAuto.h"
#include "gits.h"

namespace gits {
namespace DirectX {

void CommandListExecutionService::commandListCommand(unsigned commandListKey,
                                                     CommandWriter* command) {
  CommandList& commandList = commandListsByKey_[commandListKey];
  commandList.commandListKey = commandListKey;
  commandList.commands.push_back(command);
}

void CommandListExecutionService::executeCommandLists(unsigned callKey,
                                                      unsigned commandQueueKey,
                                                      std::vector<unsigned>& commandListKeys) {
  ExecuteCommandLists* execute = new ExecuteCommandLists();
  execute->callKey = callKey;
  execute->commandQueueKey = commandQueueKey;
  for (unsigned commandListKey : commandListKeys) {
    auto it = commandListsByKey_.find(commandListKey);
    if (it != commandListsByKey_.end()) {
      execute->commandLists.push_back(std::move(it->second));
      commandListsByKey_.erase(commandListKey);
    }
  }
  executionTracker_.execute(callKey, commandQueueKey, execute);
  executeReadyExecutables();
}

void CommandListExecutionService::commandListReset(unsigned commandKey,
                                                   unsigned commandListKey,
                                                   unsigned allocatorKey) {
  CommandList& commandList = commandListsByKey_[commandListKey];
  commandList.commands.clear();
}

bool CommandListExecutionService::isCommandListEmpty(unsigned commandListKey) {
  CommandList& commandList = commandListsByKey_[commandListKey];
  return commandList.commands.empty();
}

void CommandListExecutionService::commandQueueWait(unsigned callKey,
                                                   unsigned commandQueueKey,
                                                   unsigned fenceKey,
                                                   UINT64 fenceValue) {
  executionTracker_.commandQueueWait(callKey, commandQueueKey, fenceKey, fenceValue);
}

void CommandListExecutionService::commandQueueSignal(unsigned callKey,
                                                     unsigned commandQueueKey,
                                                     unsigned fenceKey,
                                                     UINT64 fenceValue) {
  executionTracker_.commandQueueSignal(callKey, commandQueueKey, fenceKey, fenceValue);
  executeReadyExecutables();
}

void CommandListExecutionService::fenceSignal(unsigned callKey,
                                              unsigned fenceKey,
                                              UINT64 fenceValue) {
  executionTracker_.fenceSignal(callKey, fenceKey, fenceValue);
  executeReadyExecutables();
}

void CommandListExecutionService::createCommandQueue(unsigned deviceKey, unsigned commandQueueKey) {
  deviceByCommandQueue_[commandQueueKey] = deviceKey;
}

void CommandListExecutionService::executeReadyExecutables() {
  std::vector<GpuExecutionTracker::Executable*>& executables =
      executionTracker_.getReadyExecutables();
  for (GpuExecutionTracker::Executable* executable : executables) {
    ExecuteCommandLists* executeCommandLists = static_cast<ExecuteCommandLists*>(executable);
    executeExecutable(*executeCommandLists);
    delete executable;
  }
  executables.clear();
}

void CommandListExecutionService::executeExecutable(ExecuteCommandLists& executable) {
  unsigned fenceKey{};
  auto it = fenceByCommandQueue_.find(executable.commandQueueKey);
  if (it == fenceByCommandQueue_.end()) {
    fenceKey = ++restoreObjectKey_;
    fenceByCommandQueue_[executable.commandQueueKey].first = fenceKey;
    unsigned deviceKey = deviceByCommandQueue_[executable.commandQueueKey];
    GITS_ASSERT(deviceKey);
    ID3D12DeviceCreateFenceCommand createFence;
    createFence.key = ++restoreCommandKey_;
    createFence.object_.key = deviceKey;
    createFence.InitialValue_.value = 0;
    createFence.Flags_.value = D3D12_FENCE_FLAG_NONE;
    createFence.riid_.value = IID_ID3D12Fence;
    createFence.ppFence_.key = fenceKey;
    recorder_.record(new ID3D12DeviceCreateFenceWriter(createFence));
  } else {
    fenceKey = it->second.first;
  }

  UINT64& fenceValue = fenceByCommandQueue_[executable.commandQueueKey].second;
  for (CommandList& commandList : executable.commandLists) {
    for (CommandWriter* command : commandList.commands) {
      recorder_.record(command);
    }
    {
      ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
      executeCommandLists.key = ++restoreCommandKey_;
      executeCommandLists.object_.key = executable.commandQueueKey;
      executeCommandLists.NumCommandLists_.value = 1;
      executeCommandLists.ppCommandLists_.value = reinterpret_cast<ID3D12CommandList**>(1);
      executeCommandLists.ppCommandLists_.size = 1;
      executeCommandLists.ppCommandLists_.keys.resize(1);
      executeCommandLists.ppCommandLists_.keys[0] = commandList.commandListKey;
      recorder_.record(new ID3D12CommandQueueExecuteCommandListsWriter(executeCommandLists));
    }
    {
      ID3D12CommandQueueSignalCommand commandQueueSignal;
      commandQueueSignal.key = ++restoreCommandKey_;
      commandQueueSignal.object_.key = executable.commandQueueKey;
      commandQueueSignal.pFence_.key = fenceKey;
      commandQueueSignal.Value_.value = ++fenceValue;
      recorder_.record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));
    }
    {
      ID3D12FenceGetCompletedValueCommand getCompletedValue;
      getCompletedValue.key = ++restoreCommandKey_;
      getCompletedValue.object_.key = fenceKey;
      getCompletedValue.result_.value = fenceValue;
      recorder_.record(new ID3D12FenceGetCompletedValueWriter(getCompletedValue));
    }
  }
}

} // namespace DirectX
} // namespace gits
