// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "unsupportedCommand.h"

#include "log.h"

#include <mutex>
#include <string>
#include <unordered_set>

namespace gits::opengl2 {

void ReportUnsupportedCommand(const char* name) {
  static std::mutex warningMutex;
  static std::unordered_set<std::string> alreadyWarnedNames;
  const std::lock_guard lock(warningMutex);
  if (alreadyWarnedNames.insert(name).second) {
    LOG_WARNING << "Unsupported OpenGL command: " << name;
  }
}

} // namespace gits::opengl2
