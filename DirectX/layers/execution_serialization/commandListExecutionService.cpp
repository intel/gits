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

void CommandListExecutionService::CommandListCommand(unsigned commandListKey,
                                                     const Command& command) {
  CommandList& commandList = m_CommandListsByKey[commandListKey];
  commandList.CommandListKey = commandListKey;
  commandList.Commands.push_back(createCommandSerializer(&command));
}

void CommandListExecutionService::ExecuteCommandLists(unsigned callKey,
                                                      unsigned commandQueueKey,
                                                      std::vector<unsigned>& commandListKeys) {
  Execute* execute = new Execute();
  for (unsigned commandListKey : commandListKeys) {
    auto it = m_CommandListsByKey.find(commandListKey);
    if (it != m_CommandListsByKey.end()) {
      execute->CommandLists.push_back(std::move(it->second));
      m_CommandListsByKey.erase(commandListKey);
    }
  }
  m_ExecutionTracker.Execute(callKey, commandQueueKey, execute);
  ExecuteReadyExecutables();
}

void CommandListExecutionService::CreateCommandList(unsigned commandListKey,
                                                    unsigned allocatorKey) {
  m_CommandListCreationAllocators[commandListKey] = allocatorKey;
  {
    ID3D12GraphicsCommandListCloseCommand closeCommand;
    closeCommand.Key = GetUniqueCommandKey();
    closeCommand.m_Object.Key = commandListKey;
    m_Recorder.Record(ID3D12GraphicsCommandListCloseSerializer(closeCommand));
  }
}

void CommandListExecutionService::CommandListReset(unsigned commandKey,
                                                   unsigned commandListKey,
                                                   unsigned allocatorKey) {
  CommandList& commandList = m_CommandListsByKey[commandListKey];
  commandList.Commands.clear();
  commandList.Reset = true;
}

void CommandListExecutionService::CommandQueueWait(unsigned callKey,
                                                   unsigned commandQueueKey,
                                                   unsigned fenceKey,
                                                   UINT64 fenceValue) {
  m_ExecutionTracker.CommandQueueWait(callKey, commandQueueKey, fenceKey, fenceValue);
}

void CommandListExecutionService::CommandQueueSignal(unsigned callKey,
                                                     unsigned commandQueueKey,
                                                     unsigned fenceKey,
                                                     UINT64 fenceValue) {
  m_ExecutionTracker.CommandQueueSignal(callKey, commandQueueKey, fenceKey, fenceValue);
  ExecuteReadyExecutables();
}

void CommandListExecutionService::FenceSignal(unsigned callKey,
                                              unsigned fenceKey,
                                              UINT64 fenceValue) {
  m_ExecutionTracker.FenceSignal(callKey, fenceKey, fenceValue);
  ExecuteReadyExecutables();
}

void CommandListExecutionService::CreateCommandQueue(unsigned DeviceKey, unsigned commandQueueKey) {
  m_DeviceByCommandQueue[commandQueueKey] = DeviceKey;
}

void CommandListExecutionService::ExecuteReadyExecutables() {
  std::vector<GpuExecutionTracker::Executable*>& executables =
      m_ExecutionTracker.GetReadyExecutables();
  for (GpuExecutionTracker::Executable* executable : executables) {
    Execute* executeCommandLists = static_cast<Execute*>(executable);
    ExecuteExecutable(*executeCommandLists);
    delete executable;
  }
  executables.clear();
}

void CommandListExecutionService::ExecuteExecutable(Execute& executable) {
  unsigned fenceKey{};
  auto it = m_FenceByCommandQueue.find(executable.CommandQueueKey);
  if (it == m_FenceByCommandQueue.end()) {
    fenceKey = GetUniqueObjectKey();
    m_FenceByCommandQueue[executable.CommandQueueKey].first = fenceKey;
    unsigned DeviceKey = m_DeviceByCommandQueue[executable.CommandQueueKey];
    GITS_ASSERT(DeviceKey);
    ID3D12DeviceCreateFenceCommand createFence;
    createFence.Key = GetUniqueCommandKey();
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
  for (CommandList& commandList : executable.CommandLists) {
    if (!commandList.Reset) {
      auto it = m_CommandListCreationAllocators.find(commandList.CommandListKey);
      if (it != m_CommandListCreationAllocators.end()) {
        ID3D12GraphicsCommandListResetCommand resetCommand;
        resetCommand.Key = GetUniqueCommandKey();
        resetCommand.m_Object.Key = commandList.CommandListKey;
        resetCommand.m_pAllocator.Key = it->second;
        m_Recorder.Record(ID3D12GraphicsCommandListResetSerializer(resetCommand));
      }
    }
    for (const auto& command : commandList.Commands) {
      m_Recorder.Record(*command);
    }
    commandList.Commands.clear();
    {
      ID3D12GraphicsCommandListCloseCommand closeCommand;
      closeCommand.Key = GetUniqueCommandKey();
      closeCommand.m_Object.Key = commandList.CommandListKey;
      m_Recorder.Record(ID3D12GraphicsCommandListCloseSerializer(closeCommand));
    }
    {
      ID3D12CommandQueueExecuteCommandListsCommand executeCommandLists;
      executeCommandLists.Key = GetUniqueCommandKey();
      executeCommandLists.m_Object.Key = executable.CommandQueueKey;
      executeCommandLists.m_NumCommandLists.Value = 1;
      executeCommandLists.m_ppCommandLists.Value = reinterpret_cast<ID3D12CommandList**>(1);
      executeCommandLists.m_ppCommandLists.Size = 1;
      executeCommandLists.m_ppCommandLists.Keys.resize(1);
      executeCommandLists.m_ppCommandLists.Keys[0] = commandList.CommandListKey;
      m_Recorder.Record(ID3D12CommandQueueExecuteCommandListsSerializer(executeCommandLists));
    }
    {
      ID3D12CommandQueueSignalCommand commandQueueSignal;
      commandQueueSignal.Key = GetUniqueCommandKey();
      commandQueueSignal.m_Object.Key = executable.CommandQueueKey;
      commandQueueSignal.m_pFence.Key = fenceKey;
      commandQueueSignal.m_Value.Value = ++fenceValue;
      m_Recorder.Record(ID3D12CommandQueueSignalSerializer(commandQueueSignal));
    }
    {
      ID3D12FenceGetCompletedValueCommand getCompletedValue;
      getCompletedValue.Key = GetUniqueCommandKey();
      getCompletedValue.m_Object.Key = fenceKey;
      getCompletedValue.m_Result.Value = fenceValue;
      m_Recorder.Record(ID3D12FenceGetCompletedValueSerializer(getCompletedValue));
    }
  }

  std::vector<unsigned> commandListKeys;
  for (CommandList& commandList : executable.CommandLists) {
    commandListKeys.push_back(commandList.CommandListKey);
  }
  m_CpuDescriptorsService.executeCommandLists(commandListKeys);
}

} // namespace DirectX
} // namespace gits
