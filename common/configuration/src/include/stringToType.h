// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <set>
#include <string>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <vector>
#include <cstdlib>

#include "enumsAuto.h"
#include "customTypes.h"

namespace {
std::vector<std::string> splitString(const std::string& str, char delimiter) {
  std::vector<std::string> elements;
  std::stringstream ss(str);
  std::string item;
  while (std::getline(ss, item, delimiter)) {
    // Trim single quotes from the beginning and end of the item if present
    if (!item.empty() && item.front() == '\'' && item.back() == '\'') {
      item = item.substr(1, item.size() - 2);
    }
    // Trim whitespace from the beginning and end of the item
    item.erase(0, item.find_first_not_of(" \t"));
    item.erase(item.find_last_not_of(" \t") + 1);
    elements.push_back(item);
  }
  return elements;
}

std::vector<std::string> processSimpleArray(std::istream& in, char delimiter) {
  std::string trimmedStr;
  std::getline(in, trimmedStr);

  if (!trimmedStr.empty() && trimmedStr.front() == '[' && trimmedStr.back() == ']') {
    trimmedStr = trimmedStr.substr(1, trimmedStr.size() - 2);
  }

  return splitString(trimmedStr, delimiter);
}
} // namespace

inline std::istream& operator>>(std::istream& in, std::vector<std::string>& sequence) {
  sequence = processSimpleArray(in, ',');
  return in;
}

inline std::istream& operator>>(std::istream& in, std::vector<int>& sequence) {
  auto elements = processSimpleArray(in, ',');
  for (const auto& elem : elements) {
    if (elem.empty()) {
      sequence.push_back(0);
    } else {
      sequence.push_back(std::stoi(elem));
    }
  }
  return in;
}

inline std::istream& operator>>(std::istream& in, std::vector<float>& sequence) {
  auto elements = processSimpleArray(in, ',');
  for (const auto& elem : elements) {
    if (elem.empty()) {
      sequence.push_back(0);
    } else {
      sequence.push_back(std::stof(elem));
    }
  }
  return in;
}

inline std::istream& operator>>(std::istream& in, std::vector<uint32_t>& sequence) {
  auto elements = processSimpleArray(in, ',');
  for (const auto& elem : elements) {
    if (elem.empty()) {
      sequence.push_back(0);
    } else {
      sequence.push_back(std::stoul(elem));
    }
  }
  return in;
}

inline std::istream& operator>>(std::istream& in, gits::MemorySizeRequirementOverride& data) {
  std::string str;
  std::getline(in, str);

  data = {0, 0};

  if (str.empty()) {
    return in;
  }

  std::vector<std::string> parts = splitString(str, ',');
  if (parts.size() != 2) {
    throw std::runtime_error("Invalid MemorySizeRequirementOverride string");
  }

  data.fixedAmount = gits::stringTo<uint32_t>(parts[0]);
  data.percent = gits::stringTo<uint32_t>(parts[1]);

  return in;
}

inline std::istream& operator>>(std::istream& lhs, BitRange& rhs) {
  std::string str;
  lhs >> str;
  rhs = BitRange(str);
  return lhs;
}

inline std::istream& operator>>(std::istream& in, gits::VulkanObjectRange& data) {
  std::string input;
  if (std::getline(in, input)) {
    std::istringstream iss(input);
    std::string vectorStr, rangeStr, modeStr;
    if (std::getline(iss, vectorStr, '/') && std::getline(iss, rangeStr, '/') &&
        std::getline(iss, modeStr)) {
      data.objVector = gits::stringTo<std::vector<uint32_t>>(vectorStr);
      data.range = gits::stringTo<BitRange>(rangeStr);
      data.objMode = gits::stringTo<gits::VulkanObjectMode>(modeStr);
    } else {
      in.setstate(std::ios::failbit);
    }
  } else {
    in.setstate(std::ios::failbit);
  }
  return in;
}

inline std::istream& operator>>(std::istream& in, std::set<gits::TraceData>& sequence) {
  auto elements = processSimpleArray(in, ',');
  for (const auto& elem : elements) {
    if (!elem.empty()) {
      sequence.insert(gits::stringTo<gits::TraceData>(elem));
    }
  }
  return in;
}

namespace {
// Needed to jump namespaces for the above operators
template <typename T>
void readFromStream(std::istringstream& iss, T& result) {
  iss >> result;
}
} // namespace

namespace gits {
// stringTo should throw as it's used for serialization that catches issues on it's own.
template <typename T>
T stringTo(const std::string& str) {
  T result;
  std::istringstream iss(str);
  readFromStream(iss, result);
  return result;
}

template <>
bool stringTo<bool>(const std::string& str);
} // namespace gits
