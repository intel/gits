// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "programBinaryCheckService.h"

#include "glEnumsAuto.h"
#include "log.h"
#include "openglDriversAuto.h"

namespace gits::opengl {

ProgramBinaryCheckService& ProgramBinaryCheckService::Get() {
  static ProgramBinaryCheckService instance;
  return instance;
}

void ProgramBinaryCheckService::OnBinaryLoad(GITSKey program) {
  if (m_Warned.load(std::memory_order_relaxed)) {
    return;
  }
  std::lock_guard lock(m_Mutex);
  m_Loaded.insert(program);
  m_Tracking.store(true, std::memory_order_relaxed);
}

void ProgramBinaryCheckService::Forget(GITSKey program) {
  if (!m_Tracking.load(std::memory_order_relaxed)) {
    return;
  }
  std::lock_guard lock(m_Mutex);
  m_Loaded.erase(program);
}

void ProgramBinaryCheckService::OnUse(GITSKey program, GLuint name) {
  if (!m_Tracking.load(std::memory_order_relaxed) || m_Warned.load(std::memory_order_relaxed)) {
    return;
  }
  std::lock_guard lock(m_Mutex);
  if (m_Loaded.erase(program) == 0) {
    return;
  }
  auto& table = player::g_DispatchTable;
  const auto isProgram = table.glIsProgram.load(std::memory_order_acquire);
  const auto getProgramiv = table.glGetProgramiv.load(std::memory_order_acquire);
  if (isProgram == nullptr || getProgramiv == nullptr || isProgram(name) == GL_FALSE) {
    return;
  }
  GLint linked = GL_FALSE;
  getProgramiv(name, GL_LINK_STATUS, &linked);
  if (linked != GL_FALSE || m_Warned.exchange(true, std::memory_order_relaxed)) {
    return;
  }
  m_Loaded.clear();
  m_Tracking.store(false, std::memory_order_relaxed);
  LOG_WARNING << "A program is used whose recorded binary the driver rejected, so it is not "
                 "linked. Record with OpenGL.Recorder.SuppressProgramBinary to capture its "
                 "shader sources.";
}

} // namespace gits::opengl
