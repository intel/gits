// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "configUtils.h"
#include "log.h"
#include <exception.h>

#include <algorithm>

namespace gits {
void parse_vector(const std::string& s, std::vector<std::string>& vector) {
  vector.clear();
  std::stringstream sstream(s);
  std::string elem;
  while (std::getline(sstream, elem, ',')) {
    vector.push_back(elem);
  }
}

std::vector<uint32_t> parseKeys(const std::vector<std::string>& input) {
  std::vector<uint32_t> vec;
  std::transform(input.begin(), input.end(), std::back_inserter(vec), GetKeyVal);
  if (std::find(vec.begin(), vec.end(), 0U) != vec.end()) {
    Log(ERR) << "Invalid exit key combination given.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return vec;
}
} // namespace gits
