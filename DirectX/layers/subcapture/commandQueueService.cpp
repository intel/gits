// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandQueueService.h"
#include "commandSerializersAuto.h"
#include "stateTrackingService.h"

namespace gits {
namespace DirectX {

CommandQueueService::CommandQueueService(StateTrackingService& stateService)
    : stateService_(stateService) {}

CommandQueueService::~CommandQueueService() {
  clearCommands();
}

void CommandQueueService::addExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  CommandQueueCommand* command = new CommandQueueCommand(c.getId(), c.key);
  command->commandSerializer.reset(new ID3D12CommandQueueExecuteCommandListsSerializer(c));
  commands.push_back(command);
}

void CommandQueueService::addUpdateTileMappings(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  CommandQueueCommand* command = new CommandQueueCommand(c.getId(), c.key);
  command->commandSerializer.reset(new ID3D12CommandQueueUpdateTileMappingsSerializer(c));
  commands.push_back(command);
}

void CommandQueueService::addCommandQueueWait(ID3D12CommandQueueWaitCommand& c) {
  CommandQueueCommand* command = new CommandQueueCommand(c.getId(), c.key);
  command->commandSerializer.reset(new ID3D12CommandQueueWaitSerializer(c));
  commands.push_back(command);
}

void CommandQueueService::addCommandQueueSignal(ID3D12CommandQueueSignalCommand& c) {
  CommandQueueCommand* command = new CommandQueueCommand(c.getId(), c.key);
  command->commandSerializer.reset(new ID3D12CommandQueueSignalSerializer(c));
  commands.push_back(command);
}

void CommandQueueService::restoreCommandQueues() {
  for (CommandQueueCommand* command : commands) {
    stateService_.getRecorder().record(command->commandSerializer.release());
  }
  clearCommands();
}

void CommandQueueService::clearCommands() {
  for (CommandQueueCommand* command : commands) {
    delete command;
  }
  commands.clear();
}

} // namespace DirectX
} // namespace gits
