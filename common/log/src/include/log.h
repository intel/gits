// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

#include <source_location>
#include <filesystem>

#define PLOG_ENABLE_WCHAR_INPUT 1
#include <plog/Log.h>

// Use the main Plog macros for logging
// LOG_VERBOSE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL, LOG_NONE

// The LOG_TRACE and LOG_TRACEV macros are not defined in Plog
// For now we map them to LOG_DEBUG and LOG_VERBOSE
#define LOG_TRACE  LOG_DEBUG
#define LOG_TRACEV LOG_VERBOSE

namespace plog {
PLOG_LINKAGE util::nstring LogPrefix(plog::Severity severity);
} // namespace plog

void GitsAssert(bool condition,
                const char* conditionStr,
                const std::string& msg = "",
                const std::source_location& loc = std::source_location::current());

#define GITS_ASSERT(condition, ...) GitsAssert(condition, #condition, ##__VA_ARGS__)

namespace gits {
namespace log {
PLOG_LINKAGE void SetMaxSeverity(gits::LogLevel lvl);
PLOG_LINKAGE plog::Severity GetSeverity(gits::LogLevel lvl);
PLOG_LINKAGE void Initialize(gits::LogLevel lvl);
PLOG_LINKAGE void Initialize(gits::LogLevel lvl, plog::IAppender* appender);
PLOG_LINKAGE void AddConsoleAppender();
PLOG_LINKAGE void RemoveConsoleAppender();
PLOG_LINKAGE void AddFileAppender(const std::filesystem::path& logFilePath);
} // namespace log
} // namespace gits
