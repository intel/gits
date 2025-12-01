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

#include <filesystem>
#include <set>
#include <string>
#include <vector>
#include <optional>

#include "enumsAuto.h"
#include "helper.h"
#include "stringFromType.h"
#include "stringToType.h"
#include "bit_range.h"
#include "log.h"

namespace gits {

<%!
def whitespace(number):
    return ' ' * number * 2
%>
<%def name="render_group(data, indentation)">
${whitespace(indentation)}struct ${data.name} {
${whitespace(indentation + 1)}void DeriveData(Configuration& config);
% for option in data.options:
    % if option.is_group:
${render_group(option, indentation+1)}
    % else:
      % if not option.is_deprecated:
${whitespace(indentation + 1)}${option.type} ${option.instance_name};
      % else:
${whitespace(indentation + 1)}std::optional<${option.type}> ${option.instance_name};
      % endif
    % endif
% endfor

${whitespace(indentation + 1)}void updateFromEnvironment();
% if indentation == 0:
${whitespace(indentation + 1)}Configuration(bool& validityFlag);
};
% else:
${whitespace(indentation)}} ${data.instance_name};
% endif
</%def>
${render_group(data, 0)}

template <typename T>
void DeriveConfigData(T& obj, Configuration& config);
} // namespace gits
