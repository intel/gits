//
// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "yamlUtils.h"

#include <functional>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace gits::gui::yaml_utils {

std::string YamlDeltaGenerator::BuildPath(const std::vector<std::string>& keyPath) {
  std::string path;
  for (size_t i = 0; i < keyPath.size(); ++i) {
    if (i > 0) {
      path += ".";
    }
    path += keyPath[i];
  }
  return path;
}

bool YamlDeltaGenerator::NodesEqual(const YAML::Node& a, const YAML::Node& b) {
  if (a.Type() != b.Type()) {
    return false;
  }

  switch (a.Type()) {
  case YAML::NodeType::Scalar:
    return a.as<std::string>() == b.as<std::string>();
  case YAML::NodeType::Sequence:
    if (a.size() != b.size()) {
      return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
      if (!NodesEqual(a[i], b[i])) {
        return false;
      }
    }
    return true;
  case YAML::NodeType::Map:
    if (a.size() != b.size()) {
      return false;
    }
    for (const auto& pair : a) {
      std::string key = pair.first.as<std::string>();
      if (!b[key] || !NodesEqual(pair.second, b[key])) {
        return false;
      }
    }
    return true;
  default:
    return false;
  }
}

void YamlDeltaGenerator::GenerateDeltaRecursive(const YAML::Node& current,
                                                const YAML::Node& base,
                                                YAML::Node& delta,
                                                std::vector<std::string>& keyPath) {
  if (current.IsMap()) {
    for (const auto& pair : current) {
      std::string key = pair.first.as<std::string>();
      const YAML::Node& currentValue = pair.second;

      keyPath.push_back(key);

      if (!base[key]) {
        // Key doesn't exist in base - this is a new addition
        delta[key] = currentValue;
      } else {
        const YAML::Node& baseValue = base[key];

        if (currentValue.IsMap() && baseValue.IsMap()) {
          // Both are maps - recurse deeper
          YAML::Node subDelta;
          GenerateDeltaRecursive(currentValue, baseValue, subDelta, keyPath);

          // Only add to delta if there are differences
          if (subDelta.size() > 0) {
            delta[key] = subDelta;
          }
        } else if (!NodesEqual(currentValue, baseValue)) {
          // Values are different (including sequences) - add to delta
          delta[key] = currentValue;
        }
      }

      keyPath.pop_back();
    }
  }
}

void YamlDeltaGenerator::GenerateFormattedYaml(
    const YAML::Node& delta,
    std::vector<std::string>& keyPath,
    std::string& output,
    const std::function<std::string(const std::string&)>& commentLookup,
    int indent) {
  std::string indentStr(indent, ' ');

  for (const auto& pair : delta) {
    std::string key = pair.first.as<std::string>();
    const YAML::Node& value = pair.second;

    keyPath.push_back(key);
    std::string path = BuildPath(keyPath);

    if (value.IsMap()) {
      // Handle empty maps
      if (value.size() == 0) {
        output += indentStr + key + ": {}";
        if (commentLookup) {
          std::string comment = commentLookup(path);
          if (!comment.empty()) {
            output += " # " + comment;
          }
        }
        output += "\n";
      } else {
        // Add the key
        output += indentStr + key + ":\n";

        // Add comment if available
        if (commentLookup) {
          std::string comment = commentLookup(path);
          if (!comment.empty()) {
            output += indentStr + "  # " + comment + "\n";
          }
        }

        // Recurse for nested maps
        GenerateFormattedYaml(value, keyPath, output, commentLookup, indent + 2);
      }
    } else if (value.IsSequence()) {
      // Handle empty sequences
      if (value.size() == 0) {
        output += indentStr + key + ": []";
        if (commentLookup) {
          std::string comment = commentLookup(path);
          if (!comment.empty()) {
            output += " # " + comment;
          }
        }
        output += "\n";
      } else {
        // Handle sequences (arrays/lists)
        output += indentStr + key + ":";

        // Add comment if available
        if (commentLookup) {
          std::string comment = commentLookup(path);
          if (!comment.empty()) {
            output += " # " + comment;
          }
        }
        output += "\n";

        // Add each sequence item
        for (const auto& item : value) {
          if (item.IsMap()) {
            // For maps in sequences, we need special handling
            bool firstKey = true;
            for (const auto& itemPair : item) {
              std::string itemKey = itemPair.first.as<std::string>();
              const YAML::Node& itemValue = itemPair.second;

              if (firstKey) {
                // First key gets the "- " prefix
                output += indentStr + "  - " + itemKey + ":";
                firstKey = false;
              } else {
                // Subsequent keys are indented to align with the first key
                output += indentStr + "    " + itemKey + ":";
              }

              keyPath.push_back(itemKey);
              std::string itemPath = BuildPath(keyPath);

              // Handle empty nodes first
              if (!itemValue.IsDefined() || (itemValue.IsSequence() && itemValue.size() == 0)) {
                output += " []";
                if (commentLookup) {
                  std::string comment = commentLookup(itemPath);
                  if (!comment.empty()) {
                    output += " # " + comment;
                  }
                }
                output += "\n";
              } else if (itemValue.IsMap() && itemValue.size() == 0) {
                output += " {}";
                if (commentLookup) {
                  std::string comment = commentLookup(itemPath);
                  if (!comment.empty()) {
                    output += " # " + comment;
                  }
                }
                output += "\n";
              } else if (itemValue.IsMap()) {
                output += "\n";
                // Recurse with proper indentation (4 more spaces from the "- ")
                GenerateFormattedYaml(itemValue, keyPath, output, commentLookup, indent + 6);
              } else if (itemValue.IsSequence()) {
                output += "\n";
                // Handle nested sequences - create a temporary node to recurse
                YAML::Node tempNode;
                tempNode[itemKey] = itemValue;
                GenerateFormattedYaml(tempNode, keyPath, output, commentLookup, indent + 6);
              } else {
                // Scalar value
                output += " ";
                std::string valueStr = YAML::Dump(itemValue);
                if (!valueStr.empty() && valueStr.back() == '\n') {
                  valueStr.pop_back();
                }
                output += valueStr;

                if (commentLookup) {
                  std::string comment = commentLookup(itemPath);
                  if (!comment.empty()) {
                    output += " # " + comment;
                  }
                }
                output += "\n";
              }

              keyPath.pop_back();
            }
          } else {
            // Simple scalar in sequence
            output += indentStr + "  - ";
            std::string itemStr = YAML::Dump(item);
            if (!itemStr.empty() && itemStr.back() == '\n') {
              itemStr.pop_back();
            }
            output += itemStr + "\n";
          }
        }
      }
    } else {
      // Leaf node - add key: value with comment
      output += indentStr + key + ": ";

      std::string valueStr = YAML::Dump(value);
      if (!valueStr.empty() && valueStr.back() == '\n') {
        valueStr.pop_back(); // Remove trailing newline from YAML::Dump
      }

      output += valueStr;

      if (commentLookup) {
        std::string comment = commentLookup(path);
        if (!comment.empty()) {
          output += " # " + comment;
        }
      }
      output += "\n";
    }

    keyPath.pop_back();
  }
}

YAML::Node YamlDeltaGenerator::GenerateDeltaNode(const YAML::Node& current,
                                                 const YAML::Node& base) {
  YAML::Node delta;
  std::vector<std::string> keyPath;
  GenerateDeltaRecursive(current, base, delta, keyPath);
  return delta;
}

std::string YamlDeltaGenerator::GenerateDelta(
    const YAML::Node& current,
    const YAML::Node& base,
    const std::function<std::string(const std::string&)>& commentLookup) {
  try {
    YAML::Node delta = GenerateDeltaNode(current, base);

    if (delta.size() == 0) {
      return "# No differences found\n";
    }

    std::string output;
    std::vector<std::string> keyPath;
    GenerateFormattedYaml(delta, keyPath, output, commentLookup, 0);

    return output;

  } catch (const YAML::Exception& e) {
    return "# Error processing YAML: " + std::string(e.what()) + "\n";
  }
}

std::string YamlDeltaGenerator::GenerateDelta(
    const std::string& currentYaml,
    const std::string& baseYaml,
    const std::function<std::string(const std::string&)>& commentLookup) {
  try {
    YAML::Node current = YAML::Load(currentYaml);
    YAML::Node base = YAML::Load(baseYaml);
    return GenerateDelta(current, base, commentLookup);
  } catch (const YAML::Exception& e) {
    return "# Error parsing YAML: " + std::string(e.what()) + "\n";
  }
}

} // namespace gits::gui::yaml_utils
