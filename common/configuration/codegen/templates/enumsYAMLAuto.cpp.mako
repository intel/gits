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
#include "log.h"

#include <yaml-cpp/yaml.h>

using namespace gits;

namespace YAML{

% for enum in enums:
Node convert<${enum.name}>::encode(const ${enum.name}& rhs) {
  Node node;
  try {
    node = stringFrom<${enum.name}>(rhs);
  } catch (const std::invalid_argument& e) {
    Log(ERR) << "Caught an invalid_argument exception during encoding ${enum.name}: " << e.what() << std::endl;
  }
  return node;
}

bool convert<${enum.name}>::decode(const Node& node, ${enum.name}& rhs) {
  if (!node.IsScalar()) {
    return false;
  }
  try {
    rhs = stringTo<${enum.name}>(node.as<std::string>());
    return true;
  } catch (const std::invalid_argument& e) {
    Log(ERR) << "Caught an invalid_argument exception during decoding ${enum.name}: " << e.what() << std::endl;
    return false;
  }
}
%endfor

}
