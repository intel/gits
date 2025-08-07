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

#pragma once

#include "enumsAuto.h"
#include "stringToType.h"
#include "args.hxx"

#include "configurationAuto.h"
#include "argumentValidators.h"

namespace gits {
<%!
def whitespace(number):
    return ' ' * number * 2

def initialize_argument(option, namespace_str, indentation):
    if option.is_group:
        return f'{option.instance_name}(group)'
    if option.type == 'bool':
       return f'{option.instance_name}(group, "{option.instance_name}", "{option.short_description}", {{{option.get_shorthands()}}}),\r\n' + \
              whitespace(indentation + 1) + f'  {option.get_negative_instance_name()}(group, "{option.get_negative_instance_name()}", "Sets {option.instance_name} option to false", {{{option.get_negative_shorthands()}}})'
    return f'{option.instance_name}(group, "{option.instance_name}", "{option.short_description}", {{{option.get_shorthands()}}})'

def initialize_arguments(options, namespace_str, indentation):
    prefix = ",\r\n  " + whitespace(indentation + 1)
    return "  " + prefix.join([initialize_argument(option, namespace_str, indentation) for option in options if (not option.is_derived or option.argument_only) and option.has_leafs()])

%>

<%def name="render_group(group, indentation)">
% if not group.is_derived and group.has_leafs():
${whitespace(indentation)}struct ${group.argument_name} {
${whitespace(indentation + 1)}args::Group group;
% for option in group.options:
%  if not option.is_derived and option.has_leafs():
    % if option.is_group: 
  ${render_group(option, indentation+1)}
    % else:
${whitespace(indentation + 1)}${f"{option.get_argument_type()} {option.instance_name};"}
    % endif
%    if option.type == 'bool' and not option.is_derived:
${whitespace(indentation + 1)}${f"{option.get_argument_type()} {option.get_negative_instance_name()};"}
%    endif
%  else:
%    if option.argument_only:
${whitespace(indentation + 1)}${f"{option.get_argument_type()} {option.instance_name};"}
%    if option.type == 'bool':
${whitespace(indentation + 1)}${f"{option.get_argument_type()} {option.get_negative_instance_name()};"}
%    endif
%    endif
%  endif
% endfor

${whitespace(indentation + 1)}${group.argument_name}(args::Group& parentGroup)
${whitespace(indentation + 1)}: group(parentGroup, "${group.name}"),
%   if len(group.options) > 0:    
${whitespace(indentation + 1)}${initialize_arguments(group.options, group.namespace_str, indentation)}
%   endif
${whitespace(indentation + 1)}{
${whitespace(indentation + 2)}group.SetTags(${option.get_tags_escaped()});
% for option in group.options:
%  if not option.is_derived and not option.is_group:
${whitespace(indentation + 2)}${option.instance_name}.SetTags(${option.get_tags_escaped()});
%    if option.type == 'bool':
${whitespace(indentation + 2)}${option.get_negative_instance_name()}.SetTags(${option.get_tags_escaped()});
%    endif
%  endif
% endfor
${whitespace(indentation + 1)}}
${whitespace(indentation + 1)}bool Validate();
${whitespace(indentation + 1)}void UpdateConfiguration(${group.namespace_str} *config);
% if indentation == 0:
};
% else:
${whitespace(indentation)}} ${group.instance_name};
% endif
%endif
</%def>
${render_group(data, 0)}
} // namespace gits
