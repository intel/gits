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
% for option in data.options:
    % if option.leaf_count - option.derived_count > 0:
${render_group(option, 0)}\
    % endif
% endfor
<%!
def whitespace(number):
    return ' ' * number * 2
%>
<%def name="render_group(data, indentation)">\
${whitespace(indentation)}${data.config_name}:
% for option in data.options:
    % if option.has_leafs():
        % if option.is_group: 
            % if not option.is_derived or not option.argument_only:
${render_group(option, indentation+1)}\
            % endif
        % else:
            % if not option.is_derived or not option.argument_only:
                % if option.needs_quotes_in_yml:
${whitespace(indentation + 1)}${option.config_name}: '${option.default}'
                % else:
${whitespace(indentation + 1)}${option.config_name}: ${option.default}
                % endif
            % endif
        % endif
    % endif
% endfor
</%def>
