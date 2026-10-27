// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// Per-command CommandRunner subclass for every enabled command; see
// modern-opengl-backend-architecture.md's player skeleton decision record. DecodeCommand()
// reuses the existing commandCodersAuto.h Decode() overload - no new coder work needed. Run()
// is defined in commandRunnersAuto.cpp.mako. Mirrors
// Vulkan/codegen/templates/commandRunnersAuto.h.mako's identical shape, scoped to OpenGL2's
// enabled command set.

#pragma once

#include "commandRunner.h"
#include "commandsAuto.h"
#include "commandCodersAuto.h"

namespace gits::opengl2 {

% for cmd in commands:
% if cmd.enabled:
<% guard = platform_guards.get(cmd.api) %>\
% if guard:
#if ${guard}
% endif
class ${cmd.name}Runner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  ${cmd.name}Command command;
};
% if guard:
#endif
% endif

% endif
% endfor
} // namespace gits::opengl2
