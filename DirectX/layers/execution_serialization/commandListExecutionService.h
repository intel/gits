// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "command.h"
#include "commandSerializer.h"
#include "executionSerializationRecorder.h"
#include "gpuExecutionTracker.h"
#include "keyUtils.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class CpuDescriptorsService;

class CommandListExecutionService {
public:
  CommandListExecutionService(ExecutionSerializationRecorder& recorder,
                              CpuDescriptorsService& cpuDescriptorsService)
      : m_Recorder(recorder), m_CpuDescriptorsService(cpuDescriptorsService) {}
  void CommandListCommand(GITSKey commandListKey, const Command& command);
  void ExecuteCommandLists(GITSKey callKey,
                           GITSKey commandQueueKey,
                           std::vector<GITSKey>& commandListKeys);
  void CreateCommandList(GITSKey commandListKey, GITSKey allocatorKey);
  void CommandListReset(GITSKey commandKey, GITSKey commandListKey, GITSKey allocatorKey);
  void CommandQueueWait(GITSKey callKey,
                        GITSKey commandQueueKey,
                        GITSKey fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(GITSKey callKey,
                          GITSKey commandQueueKey,
                          GITSKey fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(GITSKey callKey, GITSKey fenceKey, UINT64 fenceValue);
  void CreateCommandQueue(GITSKey deviceKey, GITSKey commandQueueKey);
  GITSKey GetUniqueCommandKey() {
    return ++m_RestoreCommandKey;
  };
  GITSKey GetUniqueObjectKey() {
    return ++m_RestoreObjectKey;
  };

private:
  struct CommandList {
    GITSKey CommandListKey{};
    bool Reset{};
    std::vector<std::unique_ptr<stream::CommandSerializer>> Commands;
  };

  struct Execute : public GpuExecutionTracker::Executable {
    std::vector<CommandList> CommandLists;
  };

  void ExecuteReadyExecutables();
  void ExecuteExecutable(Execute& executeCommandLists);

  ExecutionSerializationRecorder& m_Recorder;
  CpuDescriptorsService& m_CpuDescriptorsService;
  GpuExecutionTracker m_ExecutionTracker;
  std::unordered_map<GITSKey, CommandList> m_CommandListsByKey;
  std::unordered_map<GITSKey, GITSKey> m_DeviceByCommandQueue;
  std::unordered_map<GITSKey, std::pair<unsigned, UINT64>> m_FenceByCommandQueue;
  std::unordered_map<GITSKey, GITSKey> m_CommandListCreationAllocators;
  GITSKey m_RestoreCommandKey{EXECUTION_SERIALIZATION_KEY_MASK};
  GITSKey m_RestoreObjectKey{EXECUTION_SERIALIZATION_KEY_MASK};
};

} // namespace DirectX
} // namespace gits
