<%!
# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================
# This file is auto-generated, manual changes will be lost on next run.
%>---
icon: ${meta_data[1]}
---

<%!
def whitespace(number):
    return ' ' * number * 2

def header(number):
    return '#' * number

def get_group_by_name(group, name):
    if group.type != 'Group':
        return None
    if group.name == name:
        return group
    else:
        for option in group.options:
            res = get_group_by_name(option, name)
            if res:
                return res
    return None

def get_enum_values(type, enums):
    enum = [enum for enum in enums if enum.name == type]
    if len(enum) != 1:
        return ""
    return "`" + (', ').join([value.value for value in enum[0].values]) + "`"
%>
<%def name="render_group(group, indentation)">
${header(indentation)} ${group.name}

% if group.description:
${group.description}
% endif

% if len(group.get_config_options()) > 0:
| Key | Type | Default | Enumvalues |
|-|-|-|-|
% for option in group.get_config_options():
| ${option.name} | ${option.type} | ${option.default} | ${get_enum_values(option.type, enums)} |
% endfor
% endif

% for option in group.get_config_options():
- ${option.name}

  ${option.description}
% endfor

% for subgroup in group.get_config_groups():
${render_group(subgroup, indentation+1)}
% endfor

</%def>
${render_group(get_group_by_name(data, meta_data[0]), 1)}
