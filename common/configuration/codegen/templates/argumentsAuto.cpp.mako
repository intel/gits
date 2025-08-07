// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
// This file is auto-generated, manual changes will be lost on next run.
//
// generated @ ${time}

#include "argumentsAuto.h"

#include "configurationAuto.h" 
#include "argumentParser.h"
#include "argumentValidators.h"

namespace gits{
% for group in groups:
% if not group.is_derived and group.has_leafs():
bool ${group.argument_namespace_str}::Validate() {
  return ValidateConfiguration(*this);
}

void ${group.argument_namespace_str}::UpdateConfiguration(${group.namespace_str} *config) {
% for option in group.options:
%  if (option.argument_only or not option.is_derived) and option.has_leafs():
%   if option.type != 'Group':
  if (${option.instance_name}) {
    config->${option.instance_name} = ${option.instance_name}.Get();
  }
%   else:
  ${option.instance_name}.UpdateConfiguration(&(config->${option.instance_name}));
%   endif
%    if option.type == 'bool':
  if (${option.get_negative_instance_name()}) {
    config->${option.instance_name} = false;
  }
%    endif
%  endif
% endfor
}
% endif
% endfor
} // namespace gits