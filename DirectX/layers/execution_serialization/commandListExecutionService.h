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
  void commandListCommand(unsigned commandListKey, const Command& command);
  void executeCommandLists(unsigned callKey,
                           unsigned commandQueueKey,
                           std::vector<unsigned>& commandListKeys);
  void createCommandList(unsigned commandListKey, unsigned allocatorKey);
  void commandListReset(unsigned commandKey, unsigned commandListKey, unsigned allocatorKey);
  void commandQueueWait(unsigned callKey,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void commandQueueSignal(unsigned callKey,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void fenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue);
  void createCommandQueue(unsigned DeviceKey, unsigned commandQueueKey);
  unsigned getUniqueCommandKey() {
    return ++m_RestoreCommandKey;
  };
  unsigned getUniqueObjectKey() {
    return ++m_RestoreObjectKey;
  };

private:
  struct CommandList {
    unsigned m_CommandListKey{};
    bool m_Reset{};
    std::vector<std::unique_ptr<stream::CommandSerializer>> m_Commands;
  };

  struct ExecuteCommandLists : public GpuExecutionTracker::Executable {
    std::vector<CommandList> m_CommandLists;
  };

  void executeReadyExecutables();
  void executeExecutable(ExecuteCommandLists& executeCommandLists);

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
