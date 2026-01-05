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
#include "enumsAuto.h"

namespace YAML
{

%for enum in enums:
template<>
struct convert<gits::${enum.name}> {
  static Node encode(const gits::${enum.name}& rhs);
  static bool decode(const Node& node, gits::${enum.name}& rhs);
};

%endfor
}
