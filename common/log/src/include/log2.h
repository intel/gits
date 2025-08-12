// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "enumsAuto.h"

/*
Mapping between plog::Severity and gits::LogLevel:

  plog::Severity::none    -> gits::LogLevel::OFF
  plog::Severity::fatal   -> N/A
  plog::Severity::error   -> gits::LogLevel::ERROR
  plog::Severity::warning -> gits::LogLevel::WARN
  N/A                     -> gits::LogLevel::INFO
  plog::Severity::info    -> gits::LogLevel::INFOV
  plog::Severity::debug   -> gits::LogLevel::TRACE
  plog::Severity::verbose -> gits::LogLevel::TRACEV

This mapping is temporary and will be removed in the future (leaving only the plog::Severity).
*/

#include <filesystem>

#define PLOG_ENABLE_WCHAR_INPUT 1
#include <plog/Log.h>

// Use the main Plog macros for logging
// LOG_VERBOSE, LOG_DEBUG, LOG_INFO, LOG_WARNING PLOG_WARNING, LOG_ERROR, LOG_FATAL, LOG_NONE

namespace gits {
namespace log {
plog::Severity GetSeverity(gits::LogLevel lvl);
void Initialize(plog::Severity severity);
void SetLogFile(const std::filesystem::path& logFilePath);
}; // namespace log
}; // namespace gits
