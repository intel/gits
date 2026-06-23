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

#pragma once

#include <yaml-cpp/yaml.h>

#include "configurationAuto.h"
#include "customTypesYAML.h"
#include "enumsYAMLAuto.h"

namespace YAML {

% for group in groups:
template <>
struct convert<gits::${group.namespace_str}> {
  static Node encode(const gits::${group.namespace_str}& rhs);
  static bool decode(const Node& node, gits::${group.namespace_str}& rhs);
% if group.namespace_str == "Configuration":
  static void emit(YAML::Emitter& out, const gits::${group.namespace_str}& rhs, bool annotate = true, std::optional<YAML::Node> overrides = std::nullopt);
% else:
  static void emit(YAML::Emitter& out, const gits::${group.namespace_str}& rhs, bool annotate = true);
% endif
};

%endfor
}

namespace gits {

void CheckLegacyPaths(const YAML::Node& rootNode, Configuration& rhs);

}
