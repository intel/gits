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

void UniformArrayCaptureLayer::Post(glGetUniformLocationCommand& command) {
  const GLint location = command.m_Return.Value;
  if (location != -1 && command.m_name.HasValue && !command.m_name.Data.empty()) {
    const GLchar* name = command.m_name.Data.data();
    const UniformArrayElement element = FindUniformArrayElement(
        g_DispatchTable.glGetProgramiv.load(std::memory_order_acquire),
        g_DispatchTable.glGetActiveUniform.load(std::memory_order_acquire),
        g_DispatchTable.glGetUniformLocation.load(std::memory_order_acquire),
        command.m_program.Value, name, location);
    if (element.ArraySize > 1) {
      UniformArrayMetaCommand meta(command.m_ThreadId);
      meta.m_Key = CaptureManager::Get().CreateCommandKey();
      meta.m_Program.Value = command.m_program.Key;
      meta.m_Name = command.m_name;
      meta.m_Location.Value = location;
      meta.m_ArraySize.Value = element.ArraySize;
      meta.m_ArrayIndex.Value = element.ArrayIndex;
      CaptureManager::Get().RecordCommand(meta, new UniformArrayMetaSerializer(meta));
    }
  }
  Post(static_cast<Command&>(command));
}

} // namespace gits::opengl2
