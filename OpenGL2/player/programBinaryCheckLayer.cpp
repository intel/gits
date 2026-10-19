// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "programBinaryCheckLayer.h"

#include "glEnumsAuto.h"
#include "log.h"
#include "openglDriversAuto.h"

namespace gits::opengl2 {

void ProgramBinaryCheckLayer::Post(glProgramBinaryCommand& command) {
  OnBinaryLoad(command.m_program.Key);
}

void ProgramBinaryCheckLayer::Post(glProgramBinaryOESCommand& command) {
  OnBinaryLoad(command.m_program.Key);
}

void ProgramBinaryCheckLayer::Post(glLinkProgramCommand& command) {
  Forget(command.m_program.Key);
}

void ProgramBinaryCheckLayer::Post(glLinkProgramARBCommand& command) {
  Forget(command.m_programObj.Key);
}

void ProgramBinaryCheckLayer::Post(glDeleteProgramCommand& command) {
  Forget(command.m_program.Key);
}

void ProgramBinaryCheckLayer::Post(glDeleteObjectARBCommand& command) {
  Forget(command.m_obj.Key);
}

void ProgramBinaryCheckLayer::Post(glUseProgramCommand& command) {
  OnUse(command.m_program.Key, command.m_program.Value);
}

void ProgramBinaryCheckLayer::Post(glUseProgramObjectARBCommand& command) {
  OnUse(command.m_programObj.Key, command.m_programObj.Value);
}

void ProgramBinaryCheckLayer::Post(glUseProgramStagesCommand& command) {
  OnUse(command.m_program.Key, command.m_program.Value);
}

void ProgramBinaryCheckLayer::OnBinaryLoad(GITSKey key) {
  if (m_Warned.load(std::memory_order_relaxed)) {
    return;
  }
  std::lock_guard lock(m_Mutex);
  m_Loaded.insert(key);
  m_Tracking.store(true, std::memory_order_relaxed);
}

void ProgramBinaryCheckLayer::Forget(GITSKey key) {
  if (!m_Tracking.load(std::memory_order_relaxed)) {
    return;
  }
  std::lock_guard lock(m_Mutex);
  m_Loaded.erase(key);
}

void ProgramBinaryCheckLayer::OnUse(GITSKey key, GLuint program) {
  if (!m_Tracking.load(std::memory_order_relaxed) || m_Warned.load(std::memory_order_relaxed)) {
    return;
  }
  std::lock_guard lock(m_Mutex);
  if (m_Loaded.erase(key) == 0) {
    return;
  }
  auto& table = player::g_DispatchTable;
  const auto isProgram = table.glIsProgram.load(std::memory_order_acquire);
  const auto getProgramiv = table.glGetProgramiv.load(std::memory_order_acquire);
  if (isProgram == nullptr || getProgramiv == nullptr || isProgram(program) == GL_FALSE) {
    return;
  }
  GLint linked = GL_FALSE;
  getProgramiv(program, GL_LINK_STATUS, &linked);
  if (linked != GL_FALSE || m_Warned.exchange(true, std::memory_order_relaxed)) {
    return;
  }
  m_Loaded.clear();
  m_Tracking.store(false, std::memory_order_relaxed);
  LOG_WARNING << "A program is used whose recorded binary the driver rejected, so it is not "
                 "linked. Record with OpenGL.Recorder.SuppressProgramBinary to capture its "
                 "shader sources.";
}

} // namespace gits::opengl2
