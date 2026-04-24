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
  CommandQueueCommand* Command = new CommandQueueCommand(c.GetId(), c.Key);
  Command->CommandSerializer.reset(new ID3D12CommandQueueExecuteCommandListsSerializer(c));
  Commands.push_back(Command);
}

void CommandQueueService::AddUpdateTileMappings(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  CommandQueueCommand* Command = new CommandQueueCommand(c.GetId(), c.Key);
  Command->CommandSerializer.reset(new ID3D12CommandQueueUpdateTileMappingsSerializer(c));
  Commands.push_back(Command);
}

void CommandQueueService::AddCommandQueueWait(ID3D12CommandQueueWaitCommand& c) {
  CommandQueueCommand* Command = new CommandQueueCommand(c.GetId(), c.Key);
  Command->CommandSerializer.reset(new ID3D12CommandQueueWaitSerializer(c));
  Commands.push_back(Command);
}

void CommandQueueService::AddCommandQueueSignal(ID3D12CommandQueueSignalCommand& c) {
  CommandQueueCommand* Command = new CommandQueueCommand(c.GetId(), c.Key);
  Command->CommandSerializer.reset(new ID3D12CommandQueueSignalSerializer(c));
  Commands.push_back(Command);
}

void CommandQueueService::RestoreCommandQueues() {
  for (CommandQueueCommand* Command : Commands) {
    m_StateService.GetRecorder().Record(*Command->CommandSerializer);
  }
  ClearCommands();
}

void CommandQueueService::ClearCommands() {
  for (CommandQueueCommand* Command : Commands) {
    delete Command;
  }
  Commands.clear();
}

} // namespace DirectX
} // namespace gits
