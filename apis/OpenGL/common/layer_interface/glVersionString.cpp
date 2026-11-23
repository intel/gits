// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "glVersionString.h"

#include <cstdio>
#include <string>

namespace gits::opengl {

std::pair<int, int> ParseGlVersionMajorMinor(const char* versionString) {
  if (versionString == nullptr) {
    return {0, 0};
  }
  int major = 0;
  int minor = 0;
  if (std::sscanf(versionString, "%d.%d", &major, &minor) != 2) {
    return {0, 0};
  }
  return {major, minor};
}

bool ExtensionsStringContainsWholeWord(const char* extensions, const char* extensionName) {
  if (extensions == nullptr) {
    return false;
  }
  const std::string haystack = extensions;
  const std::string needle = extensionName;
  size_t pos = haystack.find(needle);
  while (pos != std::string::npos) {
    const bool leftOk = (pos == 0 || haystack[pos - 1] == ' ');
    const size_t rightIdx = pos + needle.size();
    const bool rightOk = (rightIdx == haystack.size() || haystack[rightIdx] == ' ');
    if (leftOk && rightOk) {
      return true;
    }
    pos = haystack.find(needle, pos + 1);
  }
  return false;
}

} // namespace gits::opengl
