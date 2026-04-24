// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandListExecutionService.h"
#include "commandsAuto.h"
#include "commandSerializersAuto.h"
#include "commandSerializersFactory.h"
#include "cpuDescriptorsService.h"
#include "log.h"

#include <filesystem>

namespace gits {
namespace DirectX {

void CommandListExecutionService::commandListCommand(unsigned commandListKey,
                                                     const Command& command) {
  CommandList& commandList = m_CommandListsByKey[commandListKey];
  commandList.m_CommandListKey = commandListKey;
  commandList.m_Commands.push_back(createCommandSerializer(&command));
}

void CommandListExecutionService::executeCommandLists(unsigned callKey,
                                                      unsigned commandQueueKey,
                                                      std::vector<unsigned>& commandListKeys) {
  ExecuteCommandLists* execute = new ExecuteCommandLists();
  for (unsigned commandListKey : commandListKeys) {
    auto it = m_CommandListsByKey.find(commandListKey);
    if (it != m_CommandListsByKey.end()) {
      execute->m_CommandLists.push_back(std::move(it->second));
      m_CommandListsByKey.erase(commandListKey);
    }
  }
  m_ExecutionTracker.Execute(callKey, commandQueueKey, execute);
  executeReadyExecutables();
}

void CommandListExecutionService::createCommandList(unsigned commandListKey,
                                                    unsigned allocatorKey) {
  m_CommandListCreationAllocators[commandListKey] = allocatorKey;
  {
    ID3D12GraphicsCommandListCloseCommand closeCommand;
    closeCommand.Key = getUniqueCommandKey();
    closeCommand.m_Object.Key = commandListKey;
    m_Recorder.Record(ID3D12GraphicsCommandListCloseSerializer(closeCommand));
  }
}

void CommandListExecutionService::commandListReset(unsigned commandKey,
                                                   unsigned commandListKey,
                                                   unsigned allocatorKey) {
  CommandList& commandList = m_CommandListsByKey[commandListKey];
  commandList.m_Commands.clear();
  commandList.m_Reset = true;
}

void CommandListExecutionService::commandQueueWait(unsigned callKey,
                                                   unsigned commandQueueKey,
                                                   unsigned fenceKey,
                                                   UINT64 fenceValue) {
  m_ExecutionTracker.CommandQueueWait(callKey, commandQueueKey, fenceKey, fenceValue);
}

void CommandListExecutionService::commandQueueSignal(unsigned callKey,
                                                     unsigned commandQueueKey,
                                                     unsigned fenceKey,
                                                     UINT64 fenceValue) {
  m_ExecutionTracker.CommandQueueSignal(callKey, commandQueueKey, fenceKey, fenceValue);
  executeReadyExecutables();
}

void CommandListExecutionService::fenceSignal(unsigned callKey,
                                              unsigned fenceKey,
                                              UINT64 fenceValue) {
  m_ExecutionTracker.FenceSignal(callKey, fenceKey, fenceValue);
  executeReadyExecutables();
}

void CommandListExecutionService::createCommandQueue(unsigned DeviceKey, unsigned commandQueueKey) {
  m_DeviceByCommandQueue[commandQueueKey] = DeviceKey;
}

void CommandListExecutionService::executeReadyExecutables() {
  std::vector<GpuExecutionTracker::Executable*>& executables =
      m_ExecutionTracker.GetReadyExecutables();
  for (GpuExecutionTracker::Executable* executable : executables) {
    ExecuteCommandLists* executeCommandLists = static_cast<ExecuteCommandLists*>(executable);
    executeExecutable(*executeCommandLists);
    delete executable;
  }
  executables.clear();
}

void CommandListExecutionService::executeExecutable(ExecuteCommandLists& executable) {
  unsigned fenceKey{};
  auto it = m_FenceByCommandQueue.find(executable.CommandQueueKey);
  if (it == m_FenceByCommandQueue.end()) {
    fenceKey = getUniqueObjectKey();
    m_FenceByCommandQueue[executable.CommandQueueKey].first = fenceKey;
    unsigned DeviceKey = m_DeviceByCommandQueue[executable.CommandQueueKey];
    GITS_ASSERT(DeviceKey);
    ID3D12DeviceCreateFenceCommand createFence;
    createFence.Key = getUniqueCommandKey();
    createFence.m_Object.Key = DeviceKey;
    createFence.m_InitialValue.Value = 0;
    createFence.m_Flags.Value = D3D12_FENCE_FLAG_NONE;
    createFence.m_riid.Value = IID_ID3D12Fence;
    createFence.m_ppFence.Key = fenceKey;
    m_Recorder.Record(ID3D12DeviceCreateFenceSerializer(createFence));
  } else {
    fenceKey = it->second.first;
  }

  UINT64& fenceValue = m_FenceByCommandQueue[executable.CommandQueueKey].second;
  for (CommandList& commandList : executable.m_CommandLists) {
    if (!commandList.m_Reset) {
      auto it = m_CommandListCreationAllocators.find(commandList.m_CommandListKey);
      if (it != m_CommandListCreationAllocators.end()) {
        ID3D12GraphicsCommandListResetCommand resetCommand;
        resetCommand.Key = getUniqueCommandKey();
        resetCommand.m_Object.Key = commandList.m_CommandListKey;
        resetCommand.m_pAllocator.Key = it->second;
        m_Recorder.Record(ID3D12GraphicsCommandListResetSerializer(resetCommand));
      }
    }
    for (const auto& command : commandList.m_Commands) {
      m_Recorder.Record(*command);
    }
    commandList.m_Commands.clear();
    {
      ID3D12GraphicsCommandListCloseCommand closeCommand;
      closeCommand.Key = getUniqueCommandKey();
      closeCommand.m_Object.Key = commandList.m_CommandListKey;
      m_Recorder.Record(ID3D12GraphicsCommandListCloseSerializer(closeCommand));
    }
    {
      ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
      executeCommandLists.Key = getUniqueCommandKey();
      executeCommandLists.m_Object.Key = executable.CommandQueueKey;
      executeCommandLists.m_NumCommandLists.Value = 1;
      executeCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
      executeCommandLists.m_ppCommandLists.Size = 1;
      executeCommandLists.m_ppCommandLists.Keys.resize(1);
      executeCommandLists.m_ppCommandLists.Keys[0] = commandList.m_CommandListKey;
      m_Recorder.Record(ID3D12CommandQueueExecuteCommandListsSerializer(executeCommandLists));
    }
    {
      ID3D12CommandQueueSignalCommand commandQueueSignal;
      commandQueueSignal.Key = getUniqueCommandKey();
      commandQueueSignal.m_Object.Key = executable.CommandQueueKey;
      commandQueueSignal.m_pFence.Key = fenceKey;
      commandQueueSignal.m_Value.Value = ++fenceValue;
      m_Recorder.Record(ID3D12CommandQueueSignalSerializer(commandQueueSignal));
    }
    {
      ID3D12FenceGetCompletedValueCommand getCompletedValue;
      getCompletedValue.Key = getUniqueCommandKey();
      getCompletedValue.m_Object.Key = fenceKey;
      getCompletedValue.m_Result.Value = fenceValue;
      m_Recorder.Record(ID3D12FenceGetCompletedValueSerializer(getCompletedValue));
    }
  }

  std::vector<unsigned> commandListKeys;
  for (CommandList& commandList : executable.m_CommandLists) {
    commandListKeys.push_back(commandList.m_CommandListKey);
  }
  m_CpuDescriptorsService.executeCommandLists(commandListKeys);
}

} // namespace DirectX
} // namespace gits
