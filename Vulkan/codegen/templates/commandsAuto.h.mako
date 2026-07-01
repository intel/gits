// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "vulkanHeader2.h"
#include "command.h"
#include "commandIdsAuto.h"

namespace gits {
namespace vulkan {

% for command in commands:
<%
initializer_list = generate_initializer_list(command)
params_for_function = generate_params_for_function(command)
define = get_define(command.platform)
%>\
% if define:
#ifdef ${define}
% endif
class ${command.name}Command : public Command {
public:
  ${command.name}Command(uint32_t threadId,
      % for param in command.params:
      ${param.full_type}${',' if not loop.last else ''}
      % endfor
    )
    : Command{CommandId::ID_${command.name.upper()}, threadId},
      % for param in initializer_list:
      ${param}${',' if not loop.last else ''}
      % endfor
  {}
  ${command.name}Command() : Command(CommandId::ID_${command.name.upper()}) {}

public:
  % if command.return_type != 'void':
  Argument<${command.return_type}> m_Return{};
  % endif
  % for param in params_for_function:
  ${param}{};
  %endfor
};
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
