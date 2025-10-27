// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <string>
#include <set>
#include <vector>

#include "enumsAuto.h"
#include "customTypes.h"

#include "bit_range.h"

namespace gits {
struct VulkanObjectRange;

// stringFrom should throw as it's used for serialization that catches issues on it's own.
template <>
std::string stringFrom<std::vector<int>>(const std::vector<int>& value);
template <>
std::string stringFrom<std::vector<float>>(const std::vector<float>& value);
template <>
std::string stringFrom<std::vector<std::string>>(const std::vector<std::string>& value);
template <>
std::string stringFrom<BitRange>(const BitRange& value);
template <>
std::string stringFrom<VulkanObjectRange>(const VulkanObjectRange& value);
template <>
std::string stringFrom<std::vector<ApiBool>>(const std::vector<ApiBool>& value);
template <>
std::string stringFrom<float>(const float& value);
template <>
std::string stringFrom<unsigned int>(const unsigned int& value);
template <>
std::string stringFrom<std::string>(const std::string& value);
template <>
std::string stringFrom<std::filesystem::path>(const std::filesystem::path& path);
template <>
std::string stringFrom<std::uint64_t>(const std::uint64_t& value);
template <>
std::string stringFrom<MemorySizeRequirementOverride>(const MemorySizeRequirementOverride& value);
template <>
std::string stringFrom<bool>(const bool& value);
} // namespace gits
