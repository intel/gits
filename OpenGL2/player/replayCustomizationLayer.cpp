// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "replayCustomizationLayer.h"

#include "glEnumsAuto.h"
#include "openglDriversAuto.h"
#include "playerManager.h"
#include "programBinaryCheckService.h"

namespace gits::opengl2 {

namespace {

thread_local GLenum tl_RecordedWaitResult{};

bool Executed(const Command& command) {
  return PlayerManager::Get().ExecuteCommands() && !command.m_Skip;
}

} // namespace

void ReplayCustomizationLayer::Post(glProgramBinaryCommand& command) {
  ProgramBinaryCheckService::Get().OnBinaryLoad(command.m_program.Key);
}

void ReplayCustomizationLayer::Post(glProgramBinaryOESCommand& command) {
  ProgramBinaryCheckService::Get().OnBinaryLoad(command.m_program.Key);
}

void ReplayCustomizationLayer::Post(glLinkProgramCommand& command) {
  ProgramBinaryCheckService::Get().Forget(command.m_program.Key);
}

void ReplayCustomizationLayer::Post(glLinkProgramARBCommand& command) {
  ProgramBinaryCheckService::Get().Forget(command.m_programObj.Key);
}

void ReplayCustomizationLayer::Post(glDeleteProgramCommand& command) {
  ProgramBinaryCheckService::Get().Forget(command.m_program.Key);
}

void ReplayCustomizationLayer::Post(glDeleteObjectARBCommand& command) {
  ProgramBinaryCheckService::Get().Forget(command.m_obj.Key);
}

void ReplayCustomizationLayer::Post(glUseProgramCommand& command) {
  ProgramBinaryCheckService::Get().OnUse(command.m_program.Key, command.m_program.Value);
}

void ReplayCustomizationLayer::Post(glUseProgramObjectARBCommand& command) {
  ProgramBinaryCheckService::Get().OnUse(command.m_programObj.Key, command.m_programObj.Value);
}

void ReplayCustomizationLayer::Post(glUseProgramStagesCommand& command) {
  ProgramBinaryCheckService::Get().OnUse(command.m_program.Key, command.m_program.Value);
}

void ReplayCustomizationLayer::Pre(glClientWaitSyncCommand& command) {
  tl_RecordedWaitResult = command.m_Return.Value;
}

void ReplayCustomizationLayer::Post(glClientWaitSyncCommand& command) {
  // The application went on as if the GPU had reached the sync object, so its next commands
  // may overwrite data the GPU is still reading; a slower replay GPU must catch up first. The
  // retries flush, so they cannot wait on commands the driver has not submitted.
  if (!Executed(command) || (tl_RecordedWaitResult != GL_ALREADY_SIGNALED &&
                             tl_RecordedWaitResult != GL_CONDITION_SATISFIED)) {
    return;
  }
  constexpr GLuint64 retryTimeoutNs = 1000000000;
  const auto clientWaitSync =
      player::g_DispatchTable.glClientWaitSync.load(std::memory_order_acquire);
  while (command.m_Return.Value == GL_TIMEOUT_EXPIRED) {
    command.m_Return.Value =
        clientWaitSync(command.m_sync.Value, GL_SYNC_FLUSH_COMMANDS_BIT, retryTimeoutNs);
  }
}

} // namespace gits::opengl2
