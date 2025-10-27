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

#include "configurator.h"
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
    const auto newValue = ${option.instance_name}.Get();
    Configurator::Instance().AddChangedField("${option.get_path()}",  stringFrom<${option.type}>(newValue),
                                            stringFrom<${option.type}>(config->${option.instance_name}),
                                            Configurator::ConfigEntry::Source::ARGUMENT);
    config->${option.instance_name} = newValue;
  }
%   else:
  ${option.instance_name}.UpdateConfiguration(&(config->${option.instance_name}));
%   endif
%    if option.type == 'bool':
  if (${option.get_bool_value_instance_name()}) {
    const auto newValue = ${option.instance_name}.Get();
    Configurator::Instance().AddChangedField("${option.get_path()}",  stringFrom<${option.type}>(config->${option.instance_name}),
                                            stringFrom<${option.type}>(newValue),
                                            Configurator::ConfigEntry::Source::ARGUMENT);
    config->${option.instance_name} = ${option.get_bool_value_instance_name()}.Get();
  }
%    endif
%  endif
% endfor
}
% endif
% endfor
} // namespace gits