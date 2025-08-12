// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "commandIdsAuto.h"
#include "commandWriters.h"

namespace gits {
namespace DirectX {

class StateTrackingService;

class CommandQueueService {
public:
  CommandQueueService(StateTrackingService& stateService);
  CommandQueueService(const CommandQueueService&) = delete;
  CommandQueueService& operator=(const CommandQueueService&) = delete;
  ~CommandQueueService();
  void addExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c);
  void addUpdateTileMappings(ID3D12CommandQueueUpdateTileMappingsCommand& c);
  void addCommandQueueWait(ID3D12CommandQueueWaitCommand& c);
  void addCommandQueueSignal(ID3D12CommandQueueSignalCommand& c);
  void restoreCommandQueues();
  void clearCommands();

private:
  StateTrackingService& stateService_;

  struct CommandQueueCommand {
    CommandQueueCommand(CommandId id_, unsigned key) : id(id_), commandKey(key) {}
    CommandId id{};
    unsigned commandKey{};
    std::unique_ptr<CommandWriter> commandWriter;
  };
  std::vector<CommandQueueCommand*> commands;
};

} // namespace DirectX
} // namespace gits
