// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandSerializer.h"
#include "commandsAuto.h"
#include "commandCodersAuto.h"

namespace gits {
namespace vulkan {

// Constructors are defined in commandSerializersAuto.cpp: their GetSize/Encode calls resolve
// against overload sets spanning every command and structure, which is too slow to repeat in
// every includer.
% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
class ${command.name}Serializer : public stream::CommandSerializer {
public:
  ${command.name}Serializer(const ${command.name}Command& command);

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
