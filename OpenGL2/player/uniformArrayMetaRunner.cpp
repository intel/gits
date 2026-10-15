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
#include "uniformLocationMapService.h"

namespace gits::opengl2 {

namespace {
void ReplayUniformArray(const UniformArrayMetaCommand& command) {
  const GITSKey programKey = command.m_Program.Value;
  const GLint capturedSize = command.m_ArraySize.Value;
  if (programKey == 0 || programKey == UNMAPPED_OBJECT_NAME_KEY || capturedSize <= 1 ||
      !command.m_Name.HasValue || command.m_Name.Data.empty()) {
    return;
  }
  const GLuint program = ObjectNameMapService::Get().GetHandle(GLObjectKind::Program, programKey);
  const GLchar* name = command.m_Name.Data.data();
  auto& table = player::g_DispatchTable;
  auto getUniformLocation = table.glGetUniformLocation.load(std::memory_order_acquire);
  if (program == 0 || getUniformLocation == nullptr) {
    return;
  }

  // An element the replay driver does not keep active maps to -1, which glUniform* ignores.
  const GLint replayLocation = getUniformLocation(program, name);
  const UniformArrayElement replay =
      FindUniformArrayElement(table.glGetProgramiv.load(std::memory_order_acquire),
                              table.glGetActiveUniform.load(std::memory_order_acquire),
                              getUniformLocation, program, name, replayLocation);
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
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.m_Skip) {
    ReplayUniformArray(command);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

} // namespace gits::opengl2
