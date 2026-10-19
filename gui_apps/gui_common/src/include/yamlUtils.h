// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <functional>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace gits::gui::yaml_utils {

class YamlDeltaGenerator {
private:
  // Helper to build YAML path from key sequence
  static std::string BuildPath(const std::vector<std::string>& keyPath);

  // Check if two YAML nodes are equal
  static bool NodesEqual(const YAML::Node& a, const YAML::Node& b);

  // Recursively generate delta
  static void GenerateDeltaRecursive(const YAML::Node& current,
                                     const YAML::Node& base,
                                     YAML::Node& delta,
                                     std::vector<std::string>& keyPath);

  // Generate formatted YAML string with comments
  static void GenerateFormattedYaml(
      const YAML::Node& delta,
      std::vector<std::string>& keyPath,
      std::string& output,
      const std::function<std::string(const std::string&)>& commentLookup,
      int indent = 0);

public:
  // Generate delta YAML::Node
  static YAML::Node GenerateDeltaNode(const YAML::Node& current, const YAML::Node& base);

  // Generate delta YAML string from YAML::Node objects with comment lookup
  static std::string GenerateDelta(
      const YAML::Node& current,
      const YAML::Node& base,
      const std::function<std::string(const std::string&)>& commentLookup = nullptr);

  // Convenience method that accepts strings (loads them as YAML::Node)
  static std::string GenerateDelta(
      const std::string& currentYaml,
      const std::string& baseYaml,
      const std::function<std::string(const std::string&)>& commentLookup = nullptr);
};

} // namespace gits::gui::yaml_utils
