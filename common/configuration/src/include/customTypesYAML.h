// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <vector>
#include <set>
#include <yaml-cpp/yaml.h>

#include "customTypes.h"

#include "stringFromType.h"
#include "stringToType.h"

namespace YAML {

template <>
struct convert<std::filesystem::path> {
  static bool decode(const Node& node, std::filesystem::path& rhs);
  static Node encode(const std::filesystem::path& rhs);
};

template <>
struct convert<gits::VulkanObjectRange> {
  static bool decode(const Node& node, gits::VulkanObjectRange& rhs);
  static Node encode(const gits::VulkanObjectRange& rhs);
};

template <>
struct convert<gits::MemorySizeRequirementOverride> {
  static bool decode(const Node& node, gits::MemorySizeRequirementOverride& rhs);
  static Node encode(const gits::MemorySizeRequirementOverride& rhs);
};

template <>
struct convert<BitRange> {
  static bool decode(const Node& node, BitRange& rhs);
  static Node encode(const BitRange& rhs);
};

template <>
struct convert<std::set<gits::TraceData>> {
  static bool decode(const Node& node, std::set<gits::TraceData>& rhs);
  static Node encode(const std::set<gits::TraceData>& rhs);
};

} // namespace YAML
