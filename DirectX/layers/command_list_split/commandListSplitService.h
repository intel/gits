// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"
#include "commandListSplitRecorder.h"
#include "executionSerializationKeyAllocator.h"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace gits {
namespace DirectX {

class CommandListSplitService {
public:
  explicit CommandListSplitService(CommandListSplitRecorder& recorder);

  void CreateCommandList(unsigned commandListKey, unsigned allocatorKey, unsigned initialState);
  void CommandListCommand(unsigned commandListKey, const Command& command);
  void CommandListReset(unsigned commandListKey, unsigned allocatorKey, unsigned initialState);
  void ExecuteCommandLists(unsigned commandQueueKey, std::vector<unsigned>& commandListKeys);
  void CommandQueueSignal(unsigned commandQueueKey, unsigned fenceKey, uint64_t fenceValue);

  ExecutionSerializationKeyAllocator& GetKeyAllocator() {
    return m_KeyAllocator;
  }
  unsigned GetUniqueCommandKey() {
    return m_KeyAllocator.GetUniqueCommandKey();
  }

private:
  struct CommandList {
    unsigned CommandListKey{};
    unsigned InitialState{};
    bool Split{};
    std::vector<std::unique_ptr<Command>> Commands;
  };

  std::vector<CommandList> SplitCommandList(CommandList commandList);
  bool IsBeginEndCommand(const Command& command);
  void AddInterval(unsigned a, unsigned b);
  std::optional<std::pair<unsigned, unsigned>> GetInterval(unsigned key);

  CommandListSplitRecorder& m_Recorder;
  ExecutionSerializationKeyAllocator m_KeyAllocator;
  std::unordered_map<unsigned, CommandList> m_CommandListsByKey;
  std::unordered_map<unsigned, unsigned> m_AllocatorByCommandList;
  std::string m_Split;
  std::map<unsigned, unsigned> m_SplitIntervals;
  std::unordered_set<unsigned> m_ExecutedIntervalStarts;

  struct ExecuteInfo {
    unsigned commandQueueKey{};
    unsigned commandListKey{};
  } m_LastExecuteInfo;

  std::unordered_map<unsigned, uint64_t> m_FenceValueByFenceKey;
};

} // namespace DirectX
} // namespace gits
