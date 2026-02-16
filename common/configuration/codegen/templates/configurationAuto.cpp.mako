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

#include "configurationAuto.h"

#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "configurator.h"
#include "enumsAuto.h"
#include "helper.h"
#include "stringFromType.h"
#include "stringToType.h"
#include "bit_range.h"
#include "deriveData.h"

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
    % if not option.is_derived and not option.is_deprecated:
      % if option.is_string_type:
  ${".".join(option.instance_namespace[1:])}.${option.instance_name} = "${option.get_default(platform)}";
      % else:
  ${".".join(option.instance_namespace[1:])}.${option.instance_name} = stringTo<${option.type}>("${option.get_default(platform)}");
      % endif
    % endif
  % endif
% endfor
</%def>
Configuration::Configuration(bool& validityFlag) {
  // Initialize all the options to their default values
${render_group(data)}

  validityFlag = true;
}

% for group in groups:
void ${group.namespace_str}::updateFromEnvironment() {
% for option in group.options:
% if option.is_group:
${whitespace(2)}${option.instance_name}.updateFromEnvironment();
% else:
${whitespace(2)}const char* env_${option.name} = getEnvVar("${option.get_environment_string()}");
${whitespace(2)}if (env_${option.name}) {
${whitespace(3)}try {
%   if not option.is_deprecated:
${whitespace(4)}const auto& old_${option.instance_name} = stringFrom<${option.type}>(${option.instance_name});
%   else:
${whitespace(4)}LOG_WARNING << "Encountered deprecated option: ${option.get_environment_string()}, please update your environment variables";
${whitespace(4)}const auto& old_${option.instance_name} = ${option.instance_name}.has_value() ? stringFrom<${option.type}>(${option.instance_name}.value()) : "";
%   endif
${whitespace(4)}${option.instance_name} = stringTo<${option.type}>(env_${option.name});
${whitespace(4)}Configurator::Instance().AddChangedField("${option.get_path()}", env_${option.name},
                                            old_${option.instance_name},
                                            Configurator::ConfigEntry::Source::ENVIRONMENT_VARIABLE);
${whitespace(3)}} catch (const std::exception& e) {
${whitespace(4)}LOG_ERROR << "Error parsing environment variable ${option.get_environment_string()}: " << e.what() << std::endl;
${whitespace(3)}}
${whitespace(2)}}

% endif
% endfor
}
% endfor

void Configuration::CheckLegacyEnvironmentPaths() {
% for option in all_options:
% if len(option.get_legacy_paths()) > 0:
${whitespace(2)}if (!getEnvVar("${option.get_environment_string()}")){
% for access_path in option.get_legacy_paths():
${whitespace(2)}const char* env_${option.name} = getEnvVar("${access_path[2]}");
${whitespace(2)}if (env_${option.name}) {
${whitespace(3)}try {
${whitespace(4)}${option.instance_path}.${option.instance_name} = stringTo<${option.type}>(env_${option.name});
${whitespace(3)}} catch (const std::exception& e) {
${whitespace(4)}LOG_ERROR << "Error parsing environment variable ${access_path[2]}: " << e.what() << std::endl;
${whitespace(3)}}
${whitespace(3)}LOG_WARNING << "Deprecated environment variable found: ${access_path[2]}, please update it to: ${option.get_environment_string()}";
${whitespace(2)}}
% endfor
${whitespace(2)}}
// ${access_path[2]}
% endif
% endfor
}

% for group in groups:
void ${group.namespace_str}::DeriveData(Configuration &config) {
  DeriveConfigData(*this, config);
};

%endfor

template <typename T>
void DeriveConfigData(T& obj, Configuration& config) {
  return;
}
} // namespace gits
