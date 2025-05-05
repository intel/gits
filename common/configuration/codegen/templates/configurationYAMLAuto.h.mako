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
};

%endfor
}
