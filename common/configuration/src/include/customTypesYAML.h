// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <vector>
#include <set>
#include <yaml-cpp/yaml.h>

#include "stringFromType.h"
#include "stringToType.h"

namespace YAML {

template <>
struct convert<std::filesystem::path> {
  static bool decode(const Node& node, std::filesystem::path& rhs);
  static Node encode(const std::filesystem::path& rhs);
};

template <>
struct convert<BitRange> {
  static bool decode(const Node& node, BitRange& rhs);
  static Node encode(const BitRange& rhs);
};

} // namespace YAML
