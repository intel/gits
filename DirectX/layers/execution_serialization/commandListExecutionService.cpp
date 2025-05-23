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
#include "cpuDescriptorsService.h"
#include "gits.h"

#include <filesystem>

namespace gits {
namespace DirectX {

void CommandListExecutionService::copyAuxiliaryFiles() {
  std::filesystem::path streamDir = Configurator::Get().common.player.streamDir;
  std::filesystem::path subcapturePath = Configurator::Get().common.player.subcapturePath;
  if (std::filesystem::exists(streamDir / "raytracingArraysOfPointers.dat")) {
    std::filesystem::copy(streamDir / "raytracingArraysOfPointers.dat", subcapturePath,
                          std::filesystem::copy_options::overwrite_existing);
  }
  if (std::filesystem::exists(streamDir / "executeIndirectRaytracing.txt")) {
    std::filesystem::copy(streamDir / "executeIndirectRaytracing.txt", subcapturePath,
                          std::filesystem::copy_options::overwrite_existing);
  }
  if (std::filesystem::exists(streamDir / "resourcePlacementData.dat")) {
    std::filesystem::copy(streamDir / "resourcePlacementData.dat", subcapturePath,
                          std::filesystem::copy_options::overwrite_existing);
  }
}

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

void CommandListExecutionService::createCommandList(unsigned commandListKey,
                                                    unsigned allocatorKey) {
  commandListCreationAllocators_[commandListKey] = allocatorKey;
  {
    ID3D12GraphicsCommandListCloseCommand closeCommand;
    closeCommand.key = getUniqueCommandKey();
    closeCommand.object_.key = commandListKey;
    recorder_.record(new ID3D12GraphicsCommandListCloseWriter(closeCommand));
  }
}

void CommandListExecutionService::commandListReset(unsigned commandKey,
                                                   unsigned commandListKey,
                                                   unsigned allocatorKey) {
  CommandList& commandList = commandListsByKey_[commandListKey];
  commandList.commands.clear();
  commandList.reset = true;
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
    fenceKey = getUniqueObjectKey();
    fenceByCommandQueue_[executable.commandQueueKey].first = fenceKey;
    unsigned deviceKey = deviceByCommandQueue_[executable.commandQueueKey];
    GITS_ASSERT(deviceKey);
    ID3D12DeviceCreateFenceCommand createFence;
    createFence.key = getUniqueCommandKey();
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
    if (!commandList.reset) {
      auto it = commandListCreationAllocators_.find(commandList.commandListKey);
      if (it != commandListCreationAllocators_.end()) {
        ID3D12GraphicsCommandListResetCommand resetCommand;
        resetCommand.key = getUniqueCommandKey();
        resetCommand.object_.key = commandList.commandListKey;
        resetCommand.pAllocator_.key = it->second;
        recorder_.record(new ID3D12GraphicsCommandListResetWriter(resetCommand));
      }
    }
    for (CommandWriter* command : commandList.commands) {
      recorder_.record(command);
    }
    {
      ID3D12GraphicsCommandListCloseCommand closeCommand;
      closeCommand.key = getUniqueCommandKey();
      closeCommand.object_.key = commandList.commandListKey;
      recorder_.record(new ID3D12GraphicsCommandListCloseWriter(closeCommand));
    }
    {
      ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
      executeCommandLists.key = getUniqueCommandKey();
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
      commandQueueSignal.key = getUniqueCommandKey();
      commandQueueSignal.object_.key = executable.commandQueueKey;
      commandQueueSignal.pFence_.key = fenceKey;
      commandQueueSignal.Value_.value = ++fenceValue;
      recorder_.record(new ID3D12CommandQueueSignalWriter(commandQueueSignal));
    }
    {
      ID3D12FenceGetCompletedValueCommand getCompletedValue;
      getCompletedValue.key = getUniqueCommandKey();
      getCompletedValue.object_.key = fenceKey;
      getCompletedValue.result_.value = fenceValue;
      recorder_.record(new ID3D12FenceGetCompletedValueWriter(getCompletedValue));
    }
  }

  std::vector<unsigned> commandListKeys;
  for (CommandList& commandList : executable.commandLists) {
    commandListKeys.push_back(commandList.commandListKey);
  }
  cpuDescriptorsService_.executeCommandLists(commandListKeys);
}

} // namespace DirectX
} // namespace gits
