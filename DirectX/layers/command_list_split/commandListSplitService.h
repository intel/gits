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

  void CreateCommandList(GITSKey commandListKey, GITSKey allocatorKey, GITSKey initialState);
  void CommandListCommand(GITSKey commandListKey, const Command& command);
  void CommandListReset(GITSKey commandListKey, GITSKey allocatorKey, GITSKey initialState);
  void ExecuteCommandLists(GITSKey commandQueueKey, std::vector<GITSKey>& commandListKeys);
  void CommandQueueSignal(GITSKey commandQueueKey, GITSKey fenceKey, uint64_t fenceValue);

  ExecutionSerializationKeyAllocator& GetKeyAllocator() {
    return m_KeyAllocator;
  }
  GITSKey GetUniqueCommandKey() {
    return m_KeyAllocator.GetUniqueCommandKey();
  }

private:
  struct CommandList {
    GITSKey CommandListKey{};
    GITSKey InitialState{};
    bool Split{};
    std::vector<std::unique_ptr<Command>> Commands;
  };

  std::vector<CommandList> SplitCommandList(CommandList commandList);
  bool IsBeginEndCommand(const Command& command);
  void AddInterval(GITSKey a, GITSKey b);
  std::optional<std::pair<GITSKey, GITSKey>> GetInterval(GITSKey key);

  CommandListSplitRecorder& m_Recorder;
  ExecutionSerializationKeyAllocator m_KeyAllocator;
  std::unordered_map<GITSKey, CommandList> m_CommandListsByKey;
  std::unordered_map<GITSKey, GITSKey> m_AllocatorByCommandList;
  std::string m_Split;
  std::map<GITSKey, GITSKey> m_SplitIntervals;
  std::unordered_set<GITSKey> m_ExecutedIntervalStarts;

  struct ExecuteInfo {
    GITSKey commandQueueKey{};
    GITSKey commandListKey{};
  } m_LastExecuteInfo;

  std::unordered_map<GITSKey, uint64_t> m_FenceValueByFenceKey;
};

} // namespace DirectX
} // namespace gits
