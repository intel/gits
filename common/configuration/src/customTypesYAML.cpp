// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "customTypesYAML.h"

#include <yaml-cpp/yaml.h>

#include "stringFromType.h"
#include "log2.h"

namespace YAML {

bool convert<gits::VulkanObjectRange>::decode(const Node& node, gits::VulkanObjectRange& rhs) {
  if (node.IsMap()) {
    return false;
  }
  try {
    rhs = gits::stringTo<gits::VulkanObjectRange>(node.as<std::string>());
    return true;
  } catch (const std::exception& e) {
    LOG_ERROR << "Error decoding VulkanObjectRange: " << e.what() << std::endl;
    return false;
  }
}

Node convert<gits::VulkanObjectRange>::encode(const gits::VulkanObjectRange& rhs) {
  Node node;
  node = gits::stringFrom<gits::VulkanObjectRange>(rhs);
  return node;
}

bool convert<std::filesystem::path>::decode(const Node& node, std::filesystem::path& rhs) {
  if (!node.IsScalar()) {
    return false;
  }
  rhs = node.as<std::string>();
  return true;
}

Node convert<std::filesystem::path>::encode(const std::filesystem::path& rhs) {
  return Node(rhs.string());
}

bool convert<gits::MemorySizeRequirementOverride>::decode(
    const Node& node, gits::MemorySizeRequirementOverride& rhs) {
  if (!node.IsMap()) {
    return false;
  }

  rhs.fixedAmount = node["fixedAmount"].as<uint32_t>(0);
  rhs.percent = node["percent"].as<uint32_t>(0);

  return true;
}

Node convert<gits::MemorySizeRequirementOverride>::encode(
    const gits::MemorySizeRequirementOverride& rhs) {
  Node node;
  node["fixedAmount"] = rhs.fixedAmount;
  node["percent"] = rhs.percent;
  return node;
}

bool convert<BitRange>::decode(const Node& node, BitRange& rhs) {
  if (!node.IsScalar()) {
    return false;
  }
  try {
    rhs = gits::stringTo<BitRange>(node.as<std::string>());
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

Node convert<BitRange>::encode(const BitRange& rhs) {
  Node node;
  node = gits::stringFrom<BitRange>(rhs);
  return node;
}

} // namespace YAML
