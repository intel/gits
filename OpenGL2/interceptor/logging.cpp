// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "logging.h"

#include "log.h"

#include <windows.h>

#include <filesystem>
#include <mutex>

namespace gits::opengl2 {

namespace {

std::filesystem::path GetExecutableDirectory() {
  char path[MAX_PATH] = {};
  GetModuleFileNameA(nullptr, path, MAX_PATH);
  return std::filesystem::path(path).parent_path();
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

void LogFunctionName(const char* name) {
  // TODO: Remove name-only call logging when layers or the recorder consume
  // intercepted calls.
  EnsureLoggingInitialized();
  LOG_INFO << name;
}

} // namespace gits::opengl2
