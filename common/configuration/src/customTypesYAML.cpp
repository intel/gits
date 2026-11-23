// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "customTypesYAML.h"

#include <yaml-cpp/yaml.h>

#include "stringFromType.h"
#include "log.h"

namespace YAML {

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
