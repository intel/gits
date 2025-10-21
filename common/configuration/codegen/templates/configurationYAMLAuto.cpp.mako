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
#include "configurator.h"
#include "log.h"

#include <yaml-cpp/yaml.h>

#include <string>
#include <set>
#include <unordered_set>

using namespace gits;
namespace {
template <typename T>
bool isContained(const std::vector<T>& v1, const std::vector<T>& v2) {
    std::unordered_set<T> setV2(v2.begin(), v2.end());
    for (const auto& entryV1 : v1) {
        if (setV2.find(entryV1) == setV2.end()) {            
            return false;
        }
    }
    return true;
}

template <typename T>
bool sameEntries(const std::vector<T>& v1, const std::vector<T>& v2) {
    return isContained(v1, v2) && isContained(v2, v1);
}
}

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
  std::set<std::string> expectedEntries = {
% for option in group.options:
%   if not option.is_derived:
    "${option.config_name}",
%   endif
% endfor
  };

  auto shouldQuit = false;
  auto encounteredUnrecognized = false;

  for (YAML::const_iterator it=node.begin(); it!=node.end(); ++it) {
    if (it->first.as<std::string>() != "Overrides" && expectedEntries.find(it->first.as<std::string>()) == expectedEntries.end()) {
      LOG_ERROR << "Unrecognized entry: ${group.namespace_str.replace("Configuration::", '').replace("::", '.')}." << it->first.as<std::string>() << " was found in the config file, execution will be stopped";
      encounteredUnrecognized = true;
      shouldQuit = true;
    }
  }

  if (encounteredUnrecognized) {
    std::string msg = "";
    if (expectedEntries.size()){
      msg = "Expected entries: ";
      size_t i = 0;
      for (const auto& entry : expectedEntries) {
        msg += entry + ((++i < expectedEntries.size()) ? ", " : "");
      }
    } else {
      msg = "No entries were expected";
    }
    LOG_ERROR << msg;
  }

% for option in group.options:
%   if not option.is_derived:
%     if option.is_group:
  if (node["${option.config_name}"]) {
    if (!convert<${"::".join(option.namespace)}>::decode(node["${option.config_name}"], rhs.${option.instance_name})) {
      shouldQuit = true;
    }
  }
%     else:
  if (node["${option.config_name}"]) {
    const auto& defaultValue = "${option.get_default(platform)}";
%       if not option.is_vector_type:
    const auto& configValue = node["${option.config_name}"].Scalar();
    if (configValue != defaultValue) {
%       else:
    const auto& vecYAML = node["${option.config_name}"].as<${option.type}>();
    const auto& vecConfig = stringTo<${option.type}>(defaultValue);
    const auto& configValue = stringFrom<${option.type}>(vecYAML);
    if (!sameEntries(vecYAML, vecConfig)) {
%       endif
        Configurator::Instance().AddChangedField("${option.get_path()}", configValue, defaultValue);
    }
    rhs.${option.instance_name} = node["${option.config_name}"].as<${option.type}>();
  }
%     endif
%   endif
% endfor

  return !shouldQuit;
}

%endfor

}
