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

#define GITS_LOG_INSTANCE_ID_MAIN PLOG_DEFAULT_INSTANCE_ID
#define GITS_LOG_INSTANCE_ID_RAW  1

// Use the main Plog macros for logging
// LOG_VERBOSE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL, LOG_NONE

// Use the RAW macros for raw logging without prefixes (these macros use the GITS_LOG_INSTANCE_ID_RAW)
#define LOG_NONE_RAW    LOG_NONE_(GITS_LOG_INSTANCE_ID_RAW)
#define LOG_FATAL_RAW   LOG_FATAL_(GITS_LOG_INSTANCE_ID_RAW)
#define LOG_ERROR_RAW   LOG_ERROR_(GITS_LOG_INSTANCE_ID_RAW)
#define LOG_WARNING_RAW LOG_WARNING_(GITS_LOG_INSTANCE_ID_RAW)
#define LOG_INFO_RAW    LOG_INFO_(GITS_LOG_INSTANCE_ID_RAW)
#define LOG_DEBUG_RAW   LOG_DEBUG_(GITS_LOG_INSTANCE_ID_RAW)
#define LOG_VERBOSE_RAW LOG_VERBOSE_(GITS_LOG_INSTANCE_ID_RAW)

// The LOG_TRACE and LOG_TRACEV macros are not defined in Plog
// For now we map them to LOG_DEBUG and LOG_VERBOSE
#define LOG_TRACE      LOG_DEBUG
#define LOG_TRACE_RAW  LOG_DEBUG_RAW
#define LOG_TRACEV     LOG_VERBOSE
#define LOG_TRACEV_RAW LOG_VERBOSE_RAW

namespace plog {
PLOG_LINKAGE util::nstring LogPrefix(plog::Severity severity);
} // namespace plog

// Only intended to be used for LOG_TRACE
#define LOG_PREFIX plog::LogPrefix(plog::Severity::debug)

namespace gits {
namespace log {
PLOG_LINKAGE void SetMaxSeverity(gits::LogLevel lvl);
PLOG_LINKAGE plog::Severity GetSeverity(gits::LogLevel lvl);
PLOG_LINKAGE void Initialize(gits::LogLevel lvl);
PLOG_LINKAGE void Initialize(gits::LogLevel lvl, plog::IAppender* appender);
PLOG_LINKAGE void AddConsoleAppender();
PLOG_LINKAGE void RemoveConsoleAppender();
PLOG_LINKAGE void AddFileAppender(const std::filesystem::path& logFilePath);
PLOG_LINKAGE bool ShouldLog(gits::LogLevel lvl);
} // namespace log
} // namespace gits
