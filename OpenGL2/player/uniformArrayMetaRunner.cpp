// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "uniformArrayMetaRunner.h"

#include "layerAuto.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"
#include "playerManager.h"
#include "replayContextCaps.h"
#include "threadContextService.h"
#include "uniformLocationMapService.h"

namespace gits::opengl2 {

namespace {
template <typename GetProgramivFn, typename GetActiveUniformFn, typename GetUniformLocationFn>
void ReplayUniformArray(const UniformArrayMetaCommand& command,
                        GetProgramivFn getProgramiv,
                        GetActiveUniformFn getActiveUniform,
                        GetUniformLocationFn getUniformLocation) {
  const GITSKey programKey = command.m_Program.Value;
  const GLint capturedSize = command.m_ArraySize.Value;
  if (programKey == 0 || programKey == UNMAPPED_OBJECT_NAME_KEY || capturedSize <= 1 ||
      !command.m_Name.HasValue || command.m_Name.Data.empty()) {
    return;
  }
  const GLuint program = ObjectNameMapService::Get().GetHandle(GLObjectKind::Program, programKey);
  const GLchar* name = command.m_Name.Data.data();
  if (program == 0 || getUniformLocation == nullptr) {
    return;
  }

  // An element the replay driver does not keep active maps to -1, which glUniform* ignores.
  const GLint replayLocation = getUniformLocation(program, name);
  const UniformArrayElement replay = FindUniformArrayElement(
      getProgramiv, getActiveUniform, getUniformLocation, program, name, replayLocation);
  const GLint replaySize = replay.ArraySize > 0 ? replay.ArraySize : 0;
  const GLint capturedBase = command.m_Location.Value - command.m_ArrayIndex.Value;
  const GLint replayBase = replayLocation - replay.ArrayIndex;
  auto& locations = UniformLocationMapService::Get();
  for (GLint i = 0; i < capturedSize; ++i) {
    // The queried element keeps the mapping its own glGetUniformLocation registered.
    if (i != command.m_ArrayIndex.Value) {
      locations.Register(programKey, capturedBase + i, i < replaySize ? replayBase + i : -1);
    }
  }
}
} // namespace

void UniformArrayMetaRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.m_Skip) {
    // The command does not say whether the core or the GL_ARB_shader_objects lookup recorded it;
    // either family answers for the same program, so the one the replay context has is used.
    // GL_OBJECT_ACTIVE_UNIFORMS_ARB/GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB have the values of
    // the GL_ACTIVE_UNIFORMS/GL_ACTIVE_UNIFORM_MAX_LENGTH FindUniformArrayElement queries.
    const ReplayContextCaps& caps = CurrentReplayContextCaps();
    auto& table = player::g_DispatchTable;
    if (caps.ShaderObjectsCore) {
      ReplayUniformArray(command, table.glGetProgramiv.load(std::memory_order_acquire),
                         table.glGetActiveUniform.load(std::memory_order_acquire),
                         table.glGetUniformLocation.load(std::memory_order_acquire));
    } else if (caps.ShaderObjects) {
      ReplayUniformArray(command, table.glGetObjectParameterivARB.load(std::memory_order_acquire),
                         table.glGetActiveUniformARB.load(std::memory_order_acquire),
                         table.glGetUniformLocationARB.load(std::memory_order_acquire));
    }
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

} // namespace gits::opengl2
