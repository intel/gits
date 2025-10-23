// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <set>

#include "enumsAuto.h"

#include "bit_range.h"

namespace gits {
struct VulkanObjectRange;

// stringFrom should throw as it's used for serialization that catches issues on it's own.
template <>
std::string stringFrom<std::vector<int>>(const std::vector<int>& value);

template <>
std::string stringFrom<std::vector<std::string>>(const std::vector<std::string>& value);

template <>
std::string stringFrom<BitRange>(const BitRange& value);

template <>
std::string stringFrom<VulkanObjectRange>(const VulkanObjectRange& value);

template <>
std::string stringFrom<std::vector<ApiBool>>(const std::vector<ApiBool>& value);
} // namespace gits
