// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandWriter.h"
#include "commandsAuto.h"
#include "commandCodersAuto.h"

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
class ${command.name}Writer : public CommandWriter {
public:
  ${command.name}Writer(${command.name}Command& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[m_DataSize]);
    Encode(command, m_Data.get());
  }

  uint32_t Id() const override {
    return static_cast<uint32_t>(CommandId::ID_${command.name.upper()});
  }
};
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
