// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   tools_lite.cpp
 *
 * @brief Common dependencyless tools for GITS project.
 *
 * This file contains various tools and utilities that do not require including
 * headers other than the standard library. This is in contrast to the regular
 * tools.cpp, which does not have this constraint.
 */

#include "tools_lite.h"

#include <algorithm>
#include <string>

namespace gits {

std::string bytesToHex(const uint8_t* const bytes, const size_t length) {
  std::string ret(2 * length, ' ');
  const char digits[] = "0123456789abcdef";
  auto iter = ret.begin();
  for (size_t i = 0; i < length; ++i) {
    *iter++ = digits[bytes[i] >> 4];
    *iter++ = digits[bytes[i] & 0x0f];
  }
  return ret;
}

void ReverseByPairs(std::string& str) {
  if (str.size() & 1) {
    return;
  }
  std::reverse(str.begin(), str.end());
  for (size_t i = 0; i + 1 < str.size(); i += 2) {
    std::swap(str[i], str[i + 1]);
  }
}

std::string to_string(const std::u8string& u8str) {
  return std::string(u8str.begin(), u8str.end());
}

} // namespace gits
