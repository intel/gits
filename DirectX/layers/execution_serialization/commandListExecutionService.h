// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

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
  void CommandListCommand(unsigned commandListKey, const Command& command);
  void ExecuteCommandLists(unsigned callKey,
                           unsigned commandQueueKey,
                           std::vector<unsigned>& commandListKeys);
  void CreateCommandList(unsigned commandListKey, unsigned allocatorKey);
  void CommandListReset(unsigned commandKey, unsigned commandListKey, unsigned allocatorKey);
  void CommandQueueWait(unsigned callKey,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(unsigned callKey,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue);
  void CreateCommandQueue(unsigned deviceKey, unsigned commandQueueKey);
  unsigned GetUniqueCommandKey() {
    return ++m_RestoreCommandKey;
  };
  unsigned GetUniqueObjectKey() {
    return ++m_RestoreObjectKey;
  };

private:
  struct CommandList {
    unsigned CommandListKey{};
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
  std::unordered_map<unsigned, CommandList> m_CommandListsByKey;
  std::unordered_map<unsigned, unsigned> m_DeviceByCommandQueue;
  std::unordered_map<unsigned, std::pair<unsigned, UINT64>> m_FenceByCommandQueue;
  std::unordered_map<unsigned, unsigned> m_CommandListCreationAllocators;
  unsigned m_RestoreCommandKey{EXECUTION_SERIALIZATION_KEY_MASK};
  unsigned m_RestoreObjectKey{EXECUTION_SERIALIZATION_KEY_MASK};
};

} // namespace DirectX
} // namespace gits
