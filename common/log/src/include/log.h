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
// LOG_VERBOSE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL, LOG_NONE

// The LOG_TRACE and LOG_TRACEV macros are not defined in Plog
// For now we map them to LOG_DEBUG and LOG_VERBOSE
#define LOG_TRACE  LOG_DEBUG
#define LOG_TRACEV LOG_VERBOSE

#include <atomic>
#include <mutex>
namespace plog {
PLOG_LINKAGE util::nstring LogPrefix(plog::Severity severity);
class FormatRawScope {
public:
  static bool IsRaw();
  FormatRawScope();
  ~FormatRawScope();
  // Disallow copying (gits::noncopyable is not available here).
  FormatRawScope(const FormatRawScope&) = delete;
  FormatRawScope& operator=(const FormatRawScope&) = delete;

private:
  static std::recursive_mutex s_mutex;
  static std::atomic<bool> s_isRaw;
  static unsigned s_refCount;
  std::lock_guard<std::recursive_mutex> lock_;
};
} // namespace plog

// These macros are not part of the Plog library and should only be used to help with the transition
// Only intended to be used for LOG_TRACE
#define LOG_FORMAT_RAW plog::FormatRawScope formatRawScopeInstance;
#define LOG_PREFIX     plog::LogPrefix(plog::Severity::debug)

namespace gits {
namespace log {
PLOG_LINKAGE void SetMaxSeverity(gits::LogLevel lvl);
PLOG_LINKAGE plog::Severity GetSeverity(gits::LogLevel lvl);
PLOG_LINKAGE void Initialize(plog::Severity severity);
PLOG_LINKAGE void Initialize(plog::Severity severity, plog::IAppender* appender);
PLOG_LINKAGE void AddConsoleAppender();
PLOG_LINKAGE void RemoveConsoleAppender();
PLOG_LINKAGE void AddFileAppender(const std::filesystem::path& logFilePath);
PLOG_LINKAGE bool ShouldLog(gits::LogLevel lvl);
}; // namespace log
}; // namespace gits
