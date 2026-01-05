// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stringToType.h"

namespace gits {
template <>
bool stringTo<bool>(const std::string& str) {
  bool result;
  if (str == "true" || str == "1" || str == "True" || str == "TRUE") {
    result = true;
  } else if (str == "false" || str == "0" || str == "False" || str == "FALSE") {
    result = false;
  } else {
    throw std::runtime_error("Invalid boolean string: " + str);
  }
  return result;
}
} // namespace gits
