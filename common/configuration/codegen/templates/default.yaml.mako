# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

<%!
def whitespace(number):
    return ' ' * number * 2
%>
<%def name="render_group(data, indentation)">\
## This creates the pretty section separators
% if indentation == 0:
<%
  box_width = 59
  text = data.config_name + ' Settings'
  total_padding = box_width - len(text)
  left_padding = total_padding // 2
  right_padding = total_padding - left_padding - 2
%>
${'#' * box_width}
#${' ' * (box_width - 2)}#
#${' ' * left_padding}${text}${' ' * right_padding}#
#${' ' * (box_width - 2)}#
${'#' * box_width}

% endif
${whitespace(indentation)}${data.config_name}:
% for option in data.options:
    % if option.is_os_visible(platform):
    % if option.has_leafs():
        % if option.is_group: 
            % if not (option.is_derived or option.argument_only):
${render_group(option, indentation+1)}\
            % endif
        % else:
            % if not (option.is_deprecated or option.is_derived or option.argument_only):
                % if option.needs_quotes_in_yml:
${whitespace(indentation + 1)}${option.config_name}: '${option.get_default(platform, installpath, conditions)}'${" # " + option.short_description if option.short_description else ''}
                % else:
${whitespace(indentation + 1)}${option.config_name}: ${option.get_default(platform, installpath, conditions)}${" # " + option.short_description if option.short_description else ''}
                % endif
            % endif
        % endif
    % endif
    % endif
% endfor
</%def>

% for option in data.options:
    % if option.is_os_visible(platform):
      % if option.leaf_count - option.derived_count > 0:
${render_group(option, 0)}\
      % endif
    % endif
% endfor

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                         #
#                       OVERRIDES                         #
#                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

# Override gits_config.yml with settings based on the executable name
# GITS will override the base configuration with the YAML under each entry

# Example entry (disable recording for MyApplication.exe):
#   MyApplication:
#     Common:
#       Recorder:
#         Enabled: false

Overrides:
