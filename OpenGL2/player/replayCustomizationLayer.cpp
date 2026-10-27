// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "replayCustomizationLayer.h"

#include "glEnumsAuto.h"
#include "glxDrawableService.h"
#include "handleArgumentUpdaters.h"
#include "hdcVirtualizationService.h"
#include "openglDriversAuto.h"
#include "playerManager.h"
#include "programBinaryCheckService.h"
#include "uniformLocationMapService.h"

#include <atomic>

namespace gits::opengl2 {

namespace {

thread_local GLenum tl_RecordedWaitResult{};
thread_local GLint tl_RecordedLocation{};

bool Executed(const Command& command) {
  return PlayerManager::Get().ExecuteCommands() && !command.m_Skip;
}

// The other elements of a uniform array the location belongs to are registered by the
// UniformArrayMetaCommand the recorder records right after the command, which knows the array's
// capture-time size.
template <typename CommandT>
void RegisterLocation(const CommandT& command, const HandleArgument<GLuint>& program) {
  if (Executed(command)) {
    UniformLocationMapService::Get().Register(program.Key, tl_RecordedLocation,
                                              command.m_Return.Value);
  }
}

template <typename QueryCurrentProgramT>
void TrackCurrentProgram(const Command& command,
                         const HandleArgument<GLuint>& program,
                         QueryCurrentProgramT queryCurrentProgram) {
  if (Executed(command) && queryCurrentProgram() == program.Value) {
    UniformLocationMapService::Get().SetCurrentProgram(CurrentContext(), program.Key);
  }
}

template <typename CommandT>
void TrackProgramPipeline(const CommandT& command) {
  if (!Executed(command)) {
    return;
  }
  GLint pipeline = 0;
  player::g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(GL_PROGRAM_PIPELINE_BINDING,
                                                                        &pipeline);
  if (static_cast<GLuint>(pipeline) == command.m_pipeline.Value) {
    UniformLocationMapService::Get().BindProgramPipeline(CurrentContext(), command.m_pipeline.Key);
  }
}

template <typename CommandT, typename QueryFunctionT>
void TrackActiveProgram(const CommandT& command, const std::atomic<QueryFunctionT>& query) {
  if (!Executed(command)) {
    return;
  }
  GLint program = 0;
  query.load(std::memory_order_acquire)(command.m_pipeline.Value, GL_ACTIVE_PROGRAM, &program);
  if (static_cast<GLuint>(program) == command.m_program.Value) {
    UniformLocationMapService::Get().SetActiveProgram(command.m_pipeline.Key,
                                                      command.m_program.Key);
  }
}

} // namespace

void ReplayCustomizationLayer::Pre(glGetUniformLocationCommand& command) {
  tl_RecordedLocation = command.m_Return.Value;
}

void ReplayCustomizationLayer::Post(glGetUniformLocationCommand& command) {
  RegisterLocation(command, command.m_program);
}

void ReplayCustomizationLayer::Pre(glGetUniformLocationARBCommand& command) {
  tl_RecordedLocation = command.m_Return.Value;
}

void ReplayCustomizationLayer::Post(glGetUniformLocationARBCommand& command) {
  RegisterLocation(command, command.m_programObj);
}

void ReplayCustomizationLayer::Post(glBindProgramPipelineCommand& command) {
  TrackProgramPipeline(command);
}

void ReplayCustomizationLayer::Post(glBindProgramPipelineEXTCommand& command) {
  TrackProgramPipeline(command);
}

void ReplayCustomizationLayer::Post(glActiveShaderProgramCommand& command) {
  TrackActiveProgram(command, player::g_DispatchTable.glGetProgramPipelineiv);
}

void ReplayCustomizationLayer::Post(glActiveShaderProgramEXTCommand& command) {
  TrackActiveProgram(command, player::g_DispatchTable.glGetProgramPipelineivEXT);
}

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
  TrackCurrentProgram(command, command.m_program, [] {
    GLint program = 0;
    player::g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(GL_CURRENT_PROGRAM,
                                                                          &program);
    return static_cast<GLuint>(program);
  });
  ProgramBinaryCheckService::Get().OnUse(command.m_program.Key, command.m_program.Value);
}

void ReplayCustomizationLayer::Post(glUseProgramObjectARBCommand& command) {
  // GL_CURRENT_PROGRAM is a GL 2.0 query; a GL_ARB_shader_objects-only driver would reject it.
  TrackCurrentProgram(command, command.m_programObj, [] {
    return player::g_DispatchTable.glGetHandleARB.load(std::memory_order_acquire)(
        GL_PROGRAM_OBJECT_ARB);
  });
  ProgramBinaryCheckService::Get().OnUse(command.m_programObj.Key, command.m_programObj.Value);
}

void ReplayCustomizationLayer::Post(glUseProgramStagesCommand& command) {
  ProgramBinaryCheckService::Get().OnUse(command.m_program.Key, command.m_program.Value);
}

void ReplayCustomizationLayer::Post(glViewportCommand& command) {
  // GrowToFit() no-ops unless the fallback window is the current draw window. The framebuffer
  // binding is not checked: growing the window for a framebuffer object's viewport only leaves
  // the default framebuffer larger than the application draws into.
  if (Executed(command) && command.m_x.Value == 0 && command.m_y.Value == 0) {
#if defined(_WIN32)
    HdcVirtualizationService::Get().GrowToFit(
        player::g_DispatchTable.wglGetCurrentDC.load(std::memory_order_acquire)(),
        command.m_width.Value, command.m_height.Value);
#else
    GlxDrawableService::Get().FitViewport(
        player::g_DispatchTable.glXGetCurrentDrawable.load(std::memory_order_acquire)(),
        command.m_width.Value, command.m_height.Value);
#endif
  }
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
