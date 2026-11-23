// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "keyEvents.h"

#include <string>
#include <sstream>
#include <vector>

namespace gits {
template <class T>
bool parse_pair(const std::string& s, T& pair) {
  std::stringstream str(s);
  str >> pair.first;
  str.ignore();
  str >> pair.second;
  return !(!str.eof() || str.fail() || str.bad());
}

std::vector<uint32_t> parseKeys(const std::vector<std::string>& input);
} // namespace gits
