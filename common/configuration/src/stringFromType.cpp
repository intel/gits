// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stringFromType.h"
#include "customTypes.h"

#include <filesystem>
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
std::string stringFrom<std::vector<float>>(const std::vector<float>& value) {
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
std::string stringFrom(const std::vector<ApiBool>& value) {
  std::ostringstream oss;
  for (size_t i = 0; i < value.size(); ++i) {
    if (i > 0) {
      oss << ",";
    }
    oss << stringFrom<ApiBool>(value[i]);
  }
  return oss.str();
}

template <>
std::string stringFrom<float>(const float& value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

template <>
std::string stringFrom<unsigned int>(const unsigned int& value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

template <>
std::string stringFrom<std::string>(const std::string& value) {
  return value;
}

template <>
std::string stringFrom<std::filesystem::path>(const std::filesystem::path& value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}
template <>
std::string stringFrom<std::uint64_t>(const std::uint64_t& value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}
template <>
std::string stringFrom<MemorySizeRequirementOverride>(const MemorySizeRequirementOverride& value) {
  std::ostringstream oss;
  if (value.fixedAmount == 0 && value.percent == 0) {
    return std::string();
  }

  oss << value.fixedAmount << "," << value.percent;
  return oss.str();
}

template <>
std::string stringFrom<bool>(const bool& value) {
  return value ? "true" : "false";
}

} // namespace gits
