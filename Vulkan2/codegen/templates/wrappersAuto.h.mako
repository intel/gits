// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "vulkanHeader.h"

#include <unordered_map>
#include <string>

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
${command.return_type} ${command.name}Wrapper(
    % for param in command.params:
    ${param.full_type}${',' if not loop.last else ''}
    % endfor
    );
% if define:
#endif
% endif

% endfor

const std::unordered_map<std::string, PFN_vkVoidFunction> g_FunctionWrappers = {
% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
  {"${command.name}", reinterpret_cast<PFN_vkVoidFunction>(${command.name}Wrapper)},
% if define:
#endif
% endif
% endfor
};

} // namespace vulkan
} // namespace gits
