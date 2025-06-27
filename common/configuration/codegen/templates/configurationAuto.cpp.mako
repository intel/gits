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

#include "configurationAuto.h"

#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "enumsAuto.h"
#include "helper.h"
#include "stringFromType.h"
#include "stringToType.h"
#include "bit_range.h"
#ifndef BUILD_FOR_CCODE
#include "deriveData.h"
#endif

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
    % if not option.is_derived:
      % if option.is_string_type:
  ${".".join(option.instance_namespace[1:])}.${option.instance_name} = "${option.get_default(platform)}";
      % else:
  ${".".join(option.instance_namespace[1:])}.${option.instance_name} = stringTo<${option.type}>("${option.get_default(platform)}");
      % endif
    % endif
  % endif
% endfor
</%def>
Configuration::Configuration() {
  // Initialize all the options to their default values
${render_group(data)}
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
${whitespace(4)}${option.instance_name} = stringTo<${option.type}>(env_${option.name});
${whitespace(3)}} catch (const std::exception& e) {
${whitespace(4)}Log(ERR) << "Error parsing environment variable ${option.get_environment_string()}: " << e.what() << std::endl;
${whitespace(3)}}
${whitespace(2)}}

% endif
% endfor
}
% endfor

#ifndef BUILD_FOR_CCODE
% for group in groups:
void ${group.namespace_str}::DeriveData(Configuration &config) {
  DeriveConfigData(*this, config);
};

%endfor

template <typename T>
void DeriveConfigData(T& obj, Configuration& config) {
  return;
}
#endif
} // namespace gits