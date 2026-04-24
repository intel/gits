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
    : m_StateService(stateService) {}

CommandQueueService::~CommandQueueService() {
  ClearCommands();
}

void CommandQueueService::AddExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  CommandQueueCommand* command = new CommandQueueCommand(c.GetId(), c.Key);
  command->CommandSerializer.reset(new ID3D12CommandQueueExecuteCommandListsSerializer(c));
  m_Commands.push_back(command);
}

void CommandQueueService::AddUpdateTileMappings(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  CommandQueueCommand* command = new CommandQueueCommand(c.GetId(), c.Key);
  command->CommandSerializer.reset(new ID3D12CommandQueueUpdateTileMappingsSerializer(c));
  m_Commands.push_back(command);
}

void CommandQueueService::AddCommandQueueWait(ID3D12CommandQueueWaitCommand& c) {
  CommandQueueCommand* command = new CommandQueueCommand(c.GetId(), c.Key);
  command->CommandSerializer.reset(new ID3D12CommandQueueWaitSerializer(c));
  m_Commands.push_back(command);
}

void CommandQueueService::AddCommandQueueSignal(ID3D12CommandQueueSignalCommand& c) {
  CommandQueueCommand* command = new CommandQueueCommand(c.GetId(), c.Key);
  command->CommandSerializer.reset(new ID3D12CommandQueueSignalSerializer(c));
  m_Commands.push_back(command);
}

void CommandQueueService::RestoreCommandQueues() {
  for (CommandQueueCommand* command : m_Commands) {
    m_StateService.GetRecorder().Record(*command->CommandSerializer);
  }
  ClearCommands();
}

void CommandQueueService::ClearCommands() {
  for (CommandQueueCommand* command : m_Commands) {
    delete command;
  }
  m_Commands.clear();
}

} // namespace DirectX
} // namespace gits
