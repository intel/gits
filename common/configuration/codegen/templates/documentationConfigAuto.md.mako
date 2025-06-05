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

def get_defaults(option):
    r = f'{option.default}'
    for k,v in option.defaults_per_platform.items():
        r += f', {v} {":material-microsoft-windows:" if k == "win32" else ":simple-linux:"}'
    return r

def format_description(description):
   return "  " + description.replace('\n', '  \n  ').replace('\n    \n', '\n  \n')
%>
<%def name="render_group(group, indentation)">
${header(indentation)} ${group.get_config_path()} { : data-toc-label='${group.name}' }

% if group.description:
${format_description(group.description)}
% endif

% if len([option for option in group.get_config_options() if not option.is_derived]) > 0:
| Key | Type | Default |
|-|-|-|
% for option in group.get_config_options():
% if not option.is_derived:
| [${option.name}](#${option.name.lower()})  <button class="btn" title="Copy full path to clipboard" data-clipboard-target="#copy-id-${option.name}">:material-clipboard-text-outline:</button> | ${f"[{option.type}](EnumsAuto.md#{option.type.lower()})" if get_enum_values(option.type, enums) else option.type} | ${get_defaults(option)} |
% endif
% endfor
% endif

% for option in group.get_config_options():
% if not option.is_derived:
- <span id="copy-id-${option.name}" style="font-weight: bold;">${option.argument_path}</span> <button class="btn" title="Copy to clipboard" data-clipboard-target="#copy-id-${option.name}">:material-clipboard-text-outline:</button> 
% if option.has_custom_shorthands:
(${option.get_custom_shorthands()})
% endif
{: #${option.name.lower()}}
% if option.is_os_limited():

  _Only available on:_
% if option.is_os_visible('win32'):
:material-microsoft-windows:
% endif
% if option.is_os_visible('lnx_32'):
:simple-linux:
% endif

% endif



${format_description(option.description)}  

% endif
% endfor

% for subgroup in group.get_config_groups():
${render_group(subgroup, indentation+1)}
% endfor

</%def>
${render_group(get_group_by_name(data, meta_data[0]), 1)}
