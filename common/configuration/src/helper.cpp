// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "helper.h"

#include <cstdlib>
#include <stdexcept>

#ifdef GITS_PLATFORM_WINDOWS
#include <cstring>
#endif

namespace gits {
const char* getEnvVar(const std::string& varName) {
#ifdef GITS_PLATFORM_WINDOWS
  char* buffer = nullptr;
  size_t size = 0;
  if (_dupenv_s(&buffer, &size, varName.c_str()) != 0 || buffer == nullptr) {
    return nullptr;
  }
  return buffer;
#else
  return std::getenv(varName.c_str());
#endif
}
} // namespace gits
