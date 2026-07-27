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
#include "enumsYAMLAuto.h"
#include "configurator.h"
#include "log.h"

#include <yaml-cpp/yaml.h>

#include <string>
#include <set>
#include <unordered_set>

using namespace gits;
namespace {

bool safeFloatCompare(const std::string& a, const std::string& b) {
    try {
        return std::fabs(std::stof(a) - std::stof(b)) < 1e-6f;
    } catch (const std::invalid_argument&) {
        return a == b; // fallback to string compare
    } catch (const std::out_of_range&) {
        return a == b;
    }
}

std::optional<uint32_t> parseUint32(const std::string& str) {
    if (str.empty()) return std::nullopt;

    try {
        size_t pos = 0;
        uint32_t result = 0;

        // Binary: "0b..." or "0B..."
        if (str.size() > 2 && str[0] == '0' && (str[1] == 'b' || str[1] == 'B')) {
            result = static_cast<uint32_t>(std::stoull(str.substr(2), &pos, 2));
            if (pos != str.size() - 2) return std::nullopt; // trailing garbage
        }
        // Hex: "0x..." or "0X..."
        else if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            result = static_cast<uint32_t>(std::stoull(str.substr(2), &pos, 16));
            if (pos != str.size() - 2) return std::nullopt;
        }
        // Decimal (default)
        else {
            uint64_t val = std::stoull(str, &pos, 10);
            if (pos != str.size()) return std::nullopt; // trailing garbage
            if (val > UINT32_MAX) return std::nullopt;  // overflow check
            result = static_cast<uint32_t>(val);
        }

        return result;

    } catch (const std::invalid_argument&) {
        return std::nullopt;
    } catch (const std::out_of_range&) {
        return std::nullopt;
    }
}

bool compareUint32Strings(const std::string& a, const std::string& b) {
    auto va = parseUint32(a);
    auto vb = parseUint32(b);

    if (!va.has_value() || !vb.has_value()) {
        // Fallback: if either fails to parse, do raw string compare
        return a == b;
    }

    return va.value() == vb.value();
}

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

% if group.namespace_str == "Configuration":
void convert<${group.namespace_str}>::emit(YAML::Emitter& out, const ${group.namespace_str}& rhs, bool annotate, std::optional<YAML::Node> overrides) {
% else:
void convert<${group.namespace_str}>::emit(YAML::Emitter& out, const ${group.namespace_str}& rhs, bool annotate) {
% endif
  out << YAML::BeginMap;
% for option in group.options:
%   if not option.is_derived and not (hasattr(option, 'is_deprecated') and option.is_deprecated):
  out << YAML::Key << "${option.config_name}";
%     if not option.is_group and option.is_vector_type and option.short_description:
  if (annotate) {
    out << YAML::Comment(R"(${option.short_description})");
  }
%     endif
  out << YAML::Value;
%     if option.is_group:
  convert<${"::".join(option.namespace)}>::emit(out, rhs.${option.instance_name});
%     else:
  out << YAML::convert<${option.type}>::encode(rhs.${option.instance_name});
%       if not option.is_vector_type and option.short_description :
  if (annotate) {
    out << YAML::Comment(R"(${option.short_description})");
  }
%       endif
%     endif
%   endif
% endfor

% if group.namespace_str == "Configuration":
  if (overrides.has_value()) {
    out << YAML::Key << "Overrides" << YAML::Value << overrides.value();
  }
% endif

  out << YAML::EndMap;
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
%         if option.numeric_format != None: # this must be uint32_t in non decimal format
    if (!compareUint32Strings(configValue, defaultValue)) {
%         elif option.type == "float":
    if (!safeFloatCompare(configValue, defaultValue)) {
%         else:
    if (configValue != defaultValue) {
%         endif
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
