// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandListSplitService.h"
#include "commandListStateService.h"
#include "commandsAuto.h"
#include "commandSerializersAuto.h"
#include "commandSerializersFactory.h"
#include "commandCopyFactory.h"
#include "configurator.h"
#include "log.h"
#include "keyUtils.h"

#include <algorithm>
#include <unordered_set>

namespace gits {
namespace DirectX {

CommandListSplitService::CommandListSplitService(CommandListSplitRecorder& recorder)
    : m_Recorder(recorder),
      m_Split(
          ParseConfigKeys(Configurator::Get().common.player.subcapture.directx.commandListSplit)) {
  if (m_Split.empty() || m_Split == "all") {
    return;
  }

  try {
    const char* p = m_Split.data();
    do {
      const char* begin = p;
      while (*p != ',' && *p) {
        ++p;
      }
      const std::string token(begin, p);
      GITS_ASSERT(!token.empty(), "CommandListSplit - Failed to parse split intervals");
      const size_t dash = token.find('-');
      if (dash == std::string::npos) {
        AddInterval(static_cast<unsigned>(std::stoul(token)),
                    static_cast<unsigned>(std::stoul(token)));
      } else {
        GITS_ASSERT(token.find('-', dash + 1) == std::string::npos,
                    "CommandListSplit - Failed to parse split intervals (multiple '-' in token)");
        AddInterval(static_cast<unsigned>(std::stoul(token.substr(0, dash))),
                    static_cast<unsigned>(std::stoul(token.substr(dash + 1))));
      }
    } while (*p++);
  } catch (...) {
    GITS_ASSERT(false, "CommandListSplit - Failed to parse split intervals");
  }
}

void CommandListSplitService::CreateCommandList(unsigned commandListKey,
                                                unsigned allocatorKey,
                                                unsigned initialState) {
  m_AllocatorByCommandList[commandListKey] = allocatorKey;
  CommandList& commandList = m_CommandListsByKey[commandListKey];
  commandList.CommandListKey = commandListKey;
  commandList.InitialState = initialState;
  {
    ID3D12GraphicsCommandListCloseCommand closeCommand;
    closeCommand.Key = GetUniqueCommandKey();
    closeCommand.m_Object.Key = commandListKey;
    m_Recorder.Record(ID3D12GraphicsCommandListCloseSerializer(closeCommand));
  }
}

void CommandListSplitService::CommandListCommand(unsigned commandListKey, const Command& command) {
  CommandList& commandList = m_CommandListsByKey[commandListKey];
  commandList.CommandListKey = commandListKey;
  if (m_Split == "all" && !CommandListStateService::IsStateCommand(command.GetId()) &&
      !IsBeginEndCommand(command)) {
    AddInterval(command.Key, command.Key);
    commandList.Split = true;
  } else if (!commandList.Split && GetInterval(command.Key).has_value()) {
    commandList.Split = true;
  }
  commandList.Commands.push_back(CreateCommandCopy(&command));
}

void CommandListSplitService::CommandListReset(unsigned commandListKey,
                                               unsigned allocatorKey,
                                               unsigned initialState) {
  m_AllocatorByCommandList[commandListKey] = allocatorKey;
  CommandList& commandList = m_CommandListsByKey[commandListKey];
  commandList.CommandListKey = commandListKey;
  commandList.InitialState = initialState;
  commandList.Split = false;
  commandList.Commands.clear();
}

void CommandListSplitService::ExecuteCommandLists(unsigned commandQueueKey,
                                                  std::vector<unsigned>& commandListKeys) {
  GITS_ASSERT(commandListKeys.size() == 1);
  m_LastExecuteInfo = {commandQueueKey, commandListKeys.back()};
}

void CommandListSplitService::CommandQueueSignal(unsigned commandQueueKey,
                                                 unsigned fenceKey,
                                                 uint64_t fenceValue) {
  GITS_ASSERT(m_LastExecuteInfo.commandQueueKey == commandQueueKey);

  auto& currentFenceValue = m_FenceValueByFenceKey[fenceKey];
  if (!currentFenceValue) {
    currentFenceValue = fenceValue;
  }

  auto it = m_CommandListsByKey.find(m_LastExecuteInfo.commandListKey);
  GITS_ASSERT(it != m_CommandListsByKey.end());

  CommandList originalCommandList = std::move(it->second);
  m_CommandListsByKey.erase(it);

  if (originalCommandList.Split) {
    std::unordered_set<unsigned> intervalStartsInThisExecution;
    for (const auto& command : originalCommandList.Commands) {
      if (auto interval = GetInterval(command->Key)) {
        intervalStartsInThisExecution.insert(interval->first);
      }
    }
    for (unsigned start : intervalStartsInThisExecution) {
      auto [insertedIt, inserted] = m_ExecutedIntervalStarts.insert(start);
      GITS_ASSERT(inserted,
                  "CommandListSplit - Interval spans across multiple command list executions");
    }
  }

  std::vector<CommandList> commandLists = SplitCommandList(std::move(originalCommandList));

  for (CommandList& commandList : commandLists) {
    auto itAlloc = m_AllocatorByCommandList.find(commandList.CommandListKey);
    GITS_ASSERT(itAlloc != m_AllocatorByCommandList.end());

    {
      ID3D12GraphicsCommandListResetCommand resetCommand;
      resetCommand.Key = GetUniqueCommandKey();
      resetCommand.m_Object.Key = commandList.CommandListKey;
      resetCommand.m_pAllocator.Key = itAlloc->second;
      resetCommand.m_pInitialState.Key = commandList.InitialState;
      m_Recorder.Record(ID3D12GraphicsCommandListResetSerializer(resetCommand));
    }

    for (const auto& command : commandList.Commands) {
      std::unique_ptr<Command> copy = CreateCommandCopy(command.get());
      m_KeyAllocator.RemapCommandKey(copy->Key);
      m_Recorder.Record(*createCommandSerializer(copy.get()));
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
      executeCommandLists.m_Object.Key = commandQueueKey;
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
      commandQueueSignal.m_Object.Key = commandQueueKey;
      commandQueueSignal.m_pFence.Key = fenceKey;
      commandQueueSignal.m_Value.Value = currentFenceValue;
      m_Recorder.Record(ID3D12CommandQueueSignalSerializer(commandQueueSignal));
    }
    {
      ID3D12FenceGetCompletedValueCommand getCompletedValue;
      getCompletedValue.Key = GetUniqueCommandKey();
      getCompletedValue.m_Object.Key = fenceKey;
      getCompletedValue.m_Result.Value = currentFenceValue++;
      m_Recorder.Record(ID3D12FenceGetCompletedValueSerializer(getCompletedValue));
    }
    {
      ID3D12CommandAllocatorResetCommand allocatorReset;
      allocatorReset.Key = GetUniqueCommandKey();
      allocatorReset.m_Object.Key = itAlloc->second;
      m_Recorder.Record(ID3D12CommandAllocatorResetSerializer(allocatorReset));
    }
  }
}

std::vector<CommandListSplitService::CommandList> CommandListSplitService::SplitCommandList(
    CommandList commandList) {
  std::vector<CommandList> result;
  if (!commandList.Split) {
    result.push_back(std::move(commandList));
    return result;
  }

  const auto getRestoreCommands = [this](const CommandListStateService& commandListState) {
    auto commands = commandListState.RestoreState();
    for (auto& command : commands) {
      command->Key = GetUniqueCommandKey();
      command->ThreadId = 0;
    }
    return commands;
  };

  struct CommandListPiece : CommandList {
    bool IsIntervalPiece{};
  };

  std::vector<CommandListPiece> commandListPieces;
  {
    CommandListStateService commandListState(commandList.CommandListKey);
    CommandListPiece* currentPiece = &commandListPieces.emplace_back(
        CommandList{commandList.CommandListKey, commandList.InitialState, false}, false);
    std::optional<std::pair<unsigned, unsigned>> prevInterval;
    for (size_t i = 0; i < commandList.Commands.size(); ++i) {
      auto& command = commandList.Commands[i];
      auto interval = GetInterval(command->Key);
      if (interval && !prevInterval) {
        prevInterval = interval;
        if (i > 0) {
          currentPiece = &commandListPieces.emplace_back(
              CommandList{commandList.CommandListKey, commandList.InitialState, false,
                          getRestoreCommands(commandListState)},
              true);
        } else {
          currentPiece->IsIntervalPiece = true;
        }
      } else if (interval != prevInterval) {
        prevInterval = interval;
        currentPiece = &commandListPieces.emplace_back(
            CommandList{commandList.CommandListKey, commandList.InitialState, false,
                        getRestoreCommands(commandListState)},
            interval.has_value());
      }

      if (IsBeginEndCommand(*command)) {
        static bool logged = false;
        if (!logged) {
          LOG_WARNING << "CommandListSplit - Begin/End commands not recorded in split";
          logged = true;
        }
      } else {
        commandListState.StoreCommand(*command);
        currentPiece->Commands.push_back(std::move(command));
      }
    }
  }

  commandListPieces.erase(
      std::remove_if(commandListPieces.begin(), commandListPieces.end(),
                     [](CommandListPiece& piece) {
                       for (const auto& command : piece.Commands) {
                         if (!CommandListStateService::IsStateCommand(command->GetId())) {
                           return false;
                         }
                       }
                       GITS_ASSERT(!piece.IsIntervalPiece,
                                   "CommandListSplit - Interval contains only state commands");
                       return true;
                     }),
      commandListPieces.end());

  for (auto& piece : commandListPieces) {
    result.push_back(std::move(static_cast<CommandList&>(piece)));
  }
  return result;
}

bool CommandListSplitService::IsBeginEndCommand(const Command& command) {
  switch (command.GetId()) {
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_BEGINQUERY:
    return true;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_ENDQUERY:
    return static_cast<const ID3D12GraphicsCommandListEndQueryCommand&>(command).m_Type.Value !=
           D3D12_QUERY_TYPE_TIMESTAMP;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_BEGINEVENT:
    return true;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_ENDEVENT:
    return true;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_BEGINRENDERPASS:
    return true;
  case CommandId::ID_ID3D12GRAPHICSCOMMANDLIST4_ENDRENDERPASS:
    return true;
  default:
    return false;
  }
}

void CommandListSplitService::AddInterval(unsigned a, unsigned b) {
  GITS_ASSERT(a <= b, "CommandListSplit - Invalid interval");
  GITS_ASSERT(!GetInterval(a).has_value(), "CommandListSplit - Overlapping intervals not handled");
  GITS_ASSERT(!GetInterval(b).has_value(), "CommandListSplit - Overlapping intervals not handled");
  m_SplitIntervals[a] = b;
}

std::optional<std::pair<unsigned, unsigned>> CommandListSplitService::GetInterval(unsigned key) {
  if (m_SplitIntervals.empty()) {
    return std::nullopt;
  }

  auto it = m_SplitIntervals.upper_bound(key);
  if (it == m_SplitIntervals.begin()) {
    return std::nullopt;
  }
  --it;
  if (key > it->second) {
    return std::nullopt;
  }
  return std::pair<unsigned, unsigned>{it->first, it->second};
}

} // namespace DirectX
} // namespace gits
