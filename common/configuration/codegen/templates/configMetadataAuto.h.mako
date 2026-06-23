// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
// This file is auto-generated, manual changes will be lost on next run.
//
// generated @ ${time}

#pragma once

#include <string>
#include <optional>

namespace gits {
struct ConfigFieldMetadata {
  const std::string ConfigPath;
  const std::string ShortDescription;
  const std::string Description;
  const std::string Type;
  // These are only present for options, not groups
  const std::optional<std::string> DefaultValue;
  const std::optional<std::string> TargetAudience;
};
<%!
def whitespace(number):
    return ' ' * number * 2
%>
<%def name="render_group(data, indentation)">
% if indentation == 0 and data.name == "Configuration":
${whitespace(indentation)}class ConfigMetadata {
public:
${whitespace(indentation+ 1 )}static const std::optional<ConfigFieldMetadata> Get(const std::string& configPath);
% else:
${whitespace(indentation)}struct ${data.name} {
${whitespace(indentation + 1)}inline static const ConfigFieldMetadata GroupMetadata =
${whitespace(indentation + 2)}{"${data.get_config_path()}",
${whitespace(indentation + 2)}R"(${data.short_description})",
${whitespace(indentation + 2)}R"(${data.description})",
${whitespace(indentation + 2)}"${data.type}",
${whitespace(indentation + 2)}std::nullopt,
${whitespace(indentation + 2)}std::nullopt};

% endif
% for option in data.options:
    % if option.is_group:
${render_group(option, indentation+1)}
    % else:
${whitespace(indentation + 1)}inline static const ConfigFieldMetadata ${option.instance_name} =
${whitespace(indentation + 2)}{"${option.get_config_path()}",
${whitespace(indentation + 2)}"${option.short_description}",
${whitespace(indentation + 2)}R"(${option.description})",
${whitespace(indentation + 2)}"${option.type}",
${whitespace(indentation + 2)}"${option.default}"
${whitespace(indentation + 2)}"${option.access_level}"};
    % endif
% endfor

% if indentation == 0:
private:
${whitespace(indentation + 1)}ConfigMetadata() = delete;
};
% else:
${whitespace(indentation)}};
% endif
</%def>
${render_group(data, 0)}

} // namespace gits
