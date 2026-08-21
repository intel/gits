// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "commandsAuto.h"
#include "commandIdsAuto.h"
#include "commandSerializer.h"

namespace gits {
namespace DirectX {

class StateTrackingService;

class CommandQueueService {
public:
  CommandQueueService(StateTrackingService& stateService);
  CommandQueueService(const CommandQueueService&) = delete;
  CommandQueueService& operator=(const CommandQueueService&) = delete;
  ~CommandQueueService();
  void AddExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c);
  void AddUpdateTileMappings(ID3D12CommandQueueUpdateTileMappingsCommand& c);
  void AddCommandQueueWait(ID3D12CommandQueueWaitCommand& c);
  void AddCommandQueueSignal(ID3D12CommandQueueSignalCommand& c);
  void RestoreCommandQueues();
  void ClearCommands();

private:
  StateTrackingService& m_StateService;

  struct CommandQueueCommand {
    CommandQueueCommand(CommandId id_, CommandKey key) : Id(id_), Key(key) {}
    CommandId Id{};
    CommandKey Key{};
    std::unique_ptr<stream::CommandSerializer> CommandSerializer;
  };
  std::vector<CommandQueueCommand*> m_Commands;
};

} // namespace DirectX
} // namespace gits
