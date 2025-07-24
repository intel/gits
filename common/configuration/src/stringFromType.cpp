// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stringFromType.h"
#include "customTypes.h"

#include <sstream>

namespace gits {
template <>
std::string stringFrom<std::vector<int>>(const std::vector<int>& value) {
  std::ostringstream oss;
  for (size_t i = 0; i < value.size(); ++i) {
    if (i > 0) {
      oss << ",";
    }
    oss << value[i];
  }
  return oss.str();
}

template <>
std::string stringFrom<std::vector<uint32_t>>(const std::vector<uint32_t>& value) {
  std::ostringstream oss;
  for (size_t i = 0; i < value.size(); ++i) {
    if (i > 0) {
      oss << ",";
    }
    oss << value[i];
  }
  return oss.str();
}

template <>
std::string stringFrom<std::vector<std::string>>(const std::vector<std::string>& value) {
  std::ostringstream oss;
  for (size_t i = 0; i < value.size(); ++i) {
    if (i > 0) {
      oss << ",";
    }
    oss << value[i];
  }
  return oss.str();
}

template <>
std::string stringFrom<BitRange>(const BitRange& value) {
  std::ostringstream oss;
  if (value.full()) {
    return "all";
  }
  if (value.empty()) {
    return "-";
  }
  return value.StrValue();
}

template <>
std::string stringFrom(const VulkanObjectRange& value) {
  std::ostringstream oss;
  oss << stringFrom<std::vector<uint32_t>>(value.objVector) << "/"
      << stringFrom<BitRange>(value.range) << "/" << stringFrom<VulkanObjectMode>(value.objMode);
  return oss.str();
}

template <>
std::string stringFrom<std::set<TraceData>>(const std::set<TraceData>& value) {
  std::ostringstream oss;
  int i = 0;
  for (const auto& elem : value) {
    if (i > 0) {
      oss << ",";
    }
    oss << elem;
    i++;
  }
  return oss.str();
}
} // namespace gits
