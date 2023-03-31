// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "bit_range.h"
#include <string>
DISABLE_WARNINGS
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/optional.hpp>
ENABLE_WARNINGS
#include <vector>
#include <set>
#include <algorithm>

namespace gits {

template <typename T>
inline void ConfigValueSet(const boost::property_tree::ptree& prop_tree,
                           const std::string& name,
                           T& value) {
  value = prop_tree.get<T>(name);
}

template <>
inline void ConfigValueSet(const boost::property_tree::ptree& prop_tree,
                           const std::string& name,
                           bool& value) {
  std::string str = prop_tree.get<std::string>(name);
  if (str == "1" || str == "Yes" || str == "True") {
    value = true;
  } else {
    value = false;
  }
}

template <>
inline void ConfigValueSet(const boost::property_tree::ptree& prop_tree,
                           const std::string& name,
                           std::vector<std::string>& values) {
  std::string value = prop_tree.get<std::string>(name);
  size_t pos;
  std::string delimeter = ",";
  char charsToRemove[] = "()\"";
  size_t len = strlen(charsToRemove);
  for (size_t i = 0; i < len; i++) {
    value.erase(std::remove(value.begin(), value.end(), charsToRemove[i]), value.end());
  }

  if (value.length() == 0) {
    return;
  }

  while ((pos = value.find(delimeter)) != std::string::npos) {
    values.push_back(value.substr(0, pos));
    value.erase(0, pos + delimeter.length());
  }
  values.push_back(value);
}
} // namespace gits
