// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "replayCustomizationLayer.h"

#include "programBinaryCheckService.h"

namespace gits::opengl2 {

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

} // namespace gits::opengl2
