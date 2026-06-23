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

#include "configMetadataAuto.h"

#include <string>
#include <optional>
#include <unordered_map>

namespace gits {
<%!
def whitespace(number):
    return ' ' * number * 2
%>
<%def name="render_group(data)">\
% for option in data.options:
  % if option.is_group:
${render_group(option)}\
  % else:
  ${".".join(option.instance_namespace[1:])}.${option.instance_name} = {"${option.short_description}"};
  % endif
% endfor
</%def>
const std::optional<ConfigFieldMetadata> ConfigMetadata::Get(const std::string& configPath) {
  static const std::unordered_map<std::string, ConfigFieldMetadata> configPathToMetadata = []() {
    // We use a lambda here to avoid a large stack impact of an initializer list of a large map
    std::unordered_map<std::string, ConfigFieldMetadata> m;
    m.reserve(${len(all_options)});
% for option in all_options:
    m.emplace("${option.get_config_path()}", ConfigMetadata::${"::".join(option.namespace[1:])}::${option.instance_name});
% endfor

    return m;
  }();

  if (configPathToMetadata.contains(configPath)) {
    return configPathToMetadata.at(configPath);
  }

  return std::nullopt;
}

} // namespace gits
