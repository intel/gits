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

#include "configurationYAMLAuto.h"

#include <yaml-cpp/yaml.h>

using namespace gits;

namespace YAML{

% for group in groups:
Node convert<${group.namespace_str}>::encode(const ${group.namespace_str}& rhs) {
  Node node;
% for option in group.options:
%   if not option.is_derived:
  node["${option.config_name}"] = rhs.${option.instance_name};
%   endif
% endfor
  return node;
}

bool convert<${group.namespace_str}>::decode(const Node& node, ${group.namespace_str}& rhs) {
% for option in group.options:
%   if not option.is_derived:
%   if option.is_group:
  if(node["${option.config_name}"]) {
    convert<${"::".join(option.namespace)}>::decode(node["${option.config_name}"], rhs.${option.instance_name});
  }
%   else:
  if(node["${option.config_name}"]) {
    rhs.${option.instance_name} = node["${option.config_name}"].as<${option.type}>();
  }
%   endif
%   endif
% endfor
  return true;
}
%endfor

}
