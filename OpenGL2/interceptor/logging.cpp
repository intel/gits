// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "logging.h"

#include "log.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <filesystem>
#include <mutex>

namespace gits::opengl2 {

namespace {

std::filesystem::path GetExecutableDirectory() {
#ifdef _WIN32
  char path[MAX_PATH] = {};
  GetModuleFileNameA(nullptr, path, MAX_PATH);
  return std::filesystem::path(path).parent_path();
#else
  std::error_code error;
  return std::filesystem::canonical("/proc/self/exe", error).parent_path();
#endif
}

} // namespace

void EnsureLoggingInitialized() {
  // TODO: Move logger initialization to the OpenGL recorder once it owns
  // configuration and can apply thresholdLogLevel and logToConsole.
  static std::once_flag initializationFlag;
  std::call_once(initializationFlag, [] {
    log::Initialize(LogLevel::INFO);
    log::AddFileAppender(GetExecutableDirectory());
  });
}

} // namespace gits::opengl2
