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

<%def name="generate_known_legacy_paths(legacy_path)">
<%
    all_paths = []
    for i in range(1, len(legacy_path) + 1):
        all_paths.append('.'.join(legacy_path[:i]))
    return all_paths
%>
</%def>

<%
    # We track already added paths in a set to not duplicate the sub-paths
    known_legacy_paths = set()
    for option in all_options:
      for path in option.get_legacy_paths():
        known_legacy_paths.update(generate_known_legacy_paths(path[0]))
%>

#include "configurationYAMLAuto.h"
#include "configurator.h"
#include "log.h"

#include <yaml-cpp/yaml.h>

#include <string>
#include <set>
#include <unordered_set>

using namespace gits;
namespace {

std::unordered_set<std::string> g_KnownLegacyPaths = {
% for path in known_legacy_paths:
  "${path}",
% endfor
};

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
%   if not option.is_derived and not (not option.is_group and option.is_deprecated):
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
    if (it->first.as<std::string>() != "Overrides" && expectedEntries.find(it->first.as<std::string>()) == expectedEntries.end() && 
      g_KnownLegacyPaths.count("${group.namespace_str.replace("Configuration::", '').replace("::", '.')}." + it->first.as<std::string>()) == 0) {
      LOG_ERROR << "Unrecognized entry: ${group.namespace_str.replace("Configuration::", '').replace("::", '.')}." << it->first.as<std::string>() << " was found in the config file. Execution will be stopped.";
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
%       if not option.is_deprecated:
    const auto& defaultValue = "${option.get_default(platform)}";
%       else:
    LOG_WARNING << "Encountered deprecated option: ${option.get_path().replace("Configuration.", "")}, please update your config file";
    const auto& defaultValue = "";
%       endif
%       if not option.is_vector_type:
    const auto& configValue = node["${option.config_name}"].Scalar();
    if (configValue != defaultValue) {
%       else:
    const auto& vecYAML = node["${option.config_name}"].as<${option.type}>();
    const auto& vecConfig = stringTo<${option.type}>(defaultValue);
    const auto& configValue = stringFrom<${option.type}>(vecYAML);
    if (!sameEntries(vecYAML, vecConfig)) {
%       endif
        Configurator::Instance().AddChangedField("${option.get_path()}", configValue, defaultValue, Configurator::ConfigEntry::Source::CONFIG_FILE);
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

namespace {

// Checking for a nested node key existence requires us to check each "level" separately
bool YamlPathExists(const YAML::Node& node, const std::vector<std::string>& path, size_t index = 0) {
    if (index >= path.size()) {
        return true;  // Node exists if we managed to get to the end
    }

    if (!node[path[index]]) {
        return false;
    }

    return YamlPathExists(node[path[index]], path, index + 1);
}

}

namespace gits {

void CheckLegacyPaths(const YAML::Node& rootNode, Configuration& rhs) {
  % for option in all_options:
  %   if len(option.get_legacy_paths()) > 0:
  if (!YamlPathExists(rootNode, {${', '.join(f"\"{e}\"" for e in option.get_yaml_path()[0])}})){
%     for access_path in option.get_legacy_paths():
    if (YamlPathExists(rootNode, {${', '.join(f"\"{e}\"" for e in access_path[0])}})) {
      LOG_WARNING << "Deprecated config path found: " << "${'.'.join(e for e in access_path[0])}, " << "please update it to: " << "${option.get_path().replace("Configuration.", "")}";
      const auto& defaultValue = "${option.get_default(platform)}";
%       if not option.is_vector_type:
      const auto& configValue = rootNode${access_path[1]}.Scalar();
      if (configValue != defaultValue) {
%       else:
      const auto& vecYAML = rootNode${access_path[1]}.as<${option.type}>();
      const auto& vecConfig = stringTo<${option.type}>(defaultValue);
      const auto& configValue = stringFrom<${option.type}>(vecYAML);
      if (!sameEntries(vecYAML, vecConfig)) {
%       endif
        Configurator::Instance().AddChangedField("${option.get_path()}", configValue, defaultValue, Configurator::ConfigEntry::Source::CONFIG_FILE, "${'.'.join(e for e in access_path[0])}");
      }
      rhs.${option.instance_path}.${option.instance_name} = rootNode${access_path[1]}.as<${option.type}>();
    }
%     endfor
  }

%   endif
% endfor
}

}
