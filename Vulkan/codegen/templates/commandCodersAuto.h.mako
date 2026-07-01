// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandsAuto.h"

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
uint32_t GetSize(const ${command.name}Command& command);
void Encode(const ${command.name}Command& command, char* dest);
void Decode(char* src, ${command.name}Command& command);
% if define:
#endif
% endif
% endfor

} // namespace vulkan
} // namespace gits
