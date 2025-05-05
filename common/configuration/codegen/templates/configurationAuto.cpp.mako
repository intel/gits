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
<%def name="render_group(data)">\
% for option in data.options:
  % if option.is_group:
${render_group(option)}\
  % else:
    % if not option.is_derived:
      % if option.is_string_type:
  ${".".join(option.instance_namespace[1:])}.${option.instance_name} = "${option.default}";
      % else:
  ${".".join(option.instance_namespace[1:])}.${option.instance_name} = stringTo<${option.type}>("${option.default}");
      % endif
    % endif
  % endif
% endfor
</%def>
Configuration::Configuration() {
  // Initialize all the options to their default values
${render_group(data)}
}

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