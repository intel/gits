// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "uniformArrayCaptureLayer.h"

#include "captureManager.h"
#include "openglDriversAuto.h"
#include "uniformArrayMetaTypes.h"

namespace gits::opengl2 {

namespace {
template <typename CommandT,
          typename GetProgramivFn,
          typename GetActiveUniformFn,
          typename GetUniformLocationFn>
void RecordUniformArrayElement(CommandT& command,
                               const HandleArgument<GLuint>& program,
                               GetProgramivFn getProgramiv,
                               GetActiveUniformFn getActiveUniform,
                               GetUniformLocationFn getUniformLocation) {
  const GLint location = command.m_Return.Value;
  if (location == -1 || !command.m_name.HasValue || command.m_name.Data.empty()) {
    return;
  }
  const UniformArrayElement element =
      FindUniformArrayElement(getProgramiv, getActiveUniform, getUniformLocation, program.Value,
                              command.m_name.Data.data(), location);
  if (element.ArraySize > 1) {
    UniformArrayMetaCommand meta(command.m_ThreadId);
    meta.m_Key = CaptureManager::Get().TakePendingPostHookKey();
    meta.m_Program.Value = program.Key;
    meta.m_Name = command.m_name;
    meta.m_Location.Value = location;
    meta.m_ArraySize.Value = element.ArraySize;
    meta.m_ArrayIndex.Value = element.ArrayIndex;
    CaptureManager::Get().RecordCommand(meta, new UniformArrayMetaSerializer(meta));
  }
}
} // namespace

void UniformArrayCaptureLayer::Post(glGetUniformLocationCommand& command) {
  RecordUniformArrayElement(command, command.m_program,
                            g_DispatchTable.glGetProgramiv.load(std::memory_order_acquire),
                            g_DispatchTable.glGetActiveUniform.load(std::memory_order_acquire),
                            g_DispatchTable.glGetUniformLocation.load(std::memory_order_acquire));
  Post(static_cast<Command&>(command));
}

void UniformArrayCaptureLayer::Post(glGetUniformLocationARBCommand& command) {
  // GL_OBJECT_ACTIVE_UNIFORMS_ARB/GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB have the values of
  // the GL_ACTIVE_UNIFORMS/GL_ACTIVE_UNIFORM_MAX_LENGTH FindUniformArrayElement queries.
  RecordUniformArrayElement(
      command, command.m_programObj,
      g_DispatchTable.glGetObjectParameterivARB.load(std::memory_order_acquire),
      g_DispatchTable.glGetActiveUniformARB.load(std::memory_order_acquire),
      g_DispatchTable.glGetUniformLocationARB.load(std::memory_order_acquire));
  Post(static_cast<Command&>(command));
}

} // namespace gits::opengl2
