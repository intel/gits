// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandRunner.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "commandCodersAuto.h"
#include "commandCodersCustom.h"

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
class ${command.name}Runner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  ${command.name}Command command;
};
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
