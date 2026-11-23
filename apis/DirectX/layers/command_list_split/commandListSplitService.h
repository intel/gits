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

  void CreateCommandList(ObjectKey commandListKey, ObjectKey allocatorKey, ObjectKey initialState);
  void CommandListCommand(ObjectKey commandListKey, const Command& command);
  void CommandListReset(ObjectKey commandListKey, ObjectKey allocatorKey, ObjectKey initialState);
  void ExecuteCommandLists(ObjectKey commandQueueKey, std::vector<ObjectKey>& commandListKeys);
  void CommandQueueSignal(ObjectKey commandQueueKey, ObjectKey fenceKey, uint64_t fenceValue);

  ExecutionSerializationKeyAllocator& GetKeyAllocator() {
    return m_KeyAllocator;
  }
  CommandKey GetUniqueCommandKey() {
    return m_KeyAllocator.GetUniqueCommandKey();
  }

private:
  struct CommandList {
    ObjectKey CommandListKey{};
    ObjectKey InitialState{};
    bool Split{};
    std::vector<std::unique_ptr<Command>> Commands;
  };

  std::vector<CommandList> SplitCommandList(CommandList commandList);
  bool IsBeginEndCommand(const Command& command);
  void AddInterval(CommandKey a, CommandKey b);
  std::optional<std::pair<CommandKey, CommandKey>> GetInterval(CommandKey key);

  CommandListSplitRecorder& m_Recorder;
  ExecutionSerializationKeyAllocator m_KeyAllocator;
  std::unordered_map<ObjectKey, CommandList> m_CommandListsByKey;
  std::unordered_map<ObjectKey, ObjectKey> m_AllocatorByCommandList;
  std::string m_Split;
  std::map<CommandKey, CommandKey> m_SplitIntervals;
  std::unordered_set<CommandKey> m_ExecutedIntervalStarts;

  struct ExecuteInfo {
    ObjectKey commandQueueKey{};
    ObjectKey commandListKey{};
  } m_LastExecuteInfo;

  std::unordered_map<ObjectKey, uint64_t> m_FenceValueByFenceKey;
};

} // namespace DirectX
} // namespace gits
