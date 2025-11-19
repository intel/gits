// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "log.h"

// Plog
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/DynamicAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Formatters/MessageOnlyFormatter.h>
#include <plog/Init.h>

#include <chrono>

#ifdef _WIN32
#include <plog/Appenders/DebugOutputAppender.h>
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

namespace plog {
util::nstring LogPrefix(plog::Severity severity) {
  auto toStr = [](plog::Severity severity) {
    switch (severity) {
    case plog::fatal:
      return "FATAL";
    case plog::error:
      return "ERROR";
    case plog::warning:
      return "WARNING";
    case plog::info:
      return "INFO";
    // Debug and verbose will be logged as TRACE
    case plog::debug:
    case plog::verbose:
      return "TRACE";
    default:
      return "NONE";
    }
  };

  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  auto timeT = std::chrono::system_clock::to_time_t(now);

  // Print formatted date and time on a pre-allocated buffer
  char buffer[32];
  size_t offset =
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&timeT));
  std::snprintf(&buffer[offset], sizeof(buffer) - offset, ".%03d", static_cast<int>(ms.count()));

  util::nostringstream ss;
  ss << static_cast<char*>(buffer) << " - " << toStr(severity) << " - ";
  return ss.str();
}

class GitsFormatter {
public:
  static util::nstring header() {
    return util::nstring();
  }

  static util::nstring format(const Record& record) {
    util::nostringstream ss;
    if (record.getInstanceId() == GITS_LOG_INSTANCE_ID_RAW) {
      // RAW trace logging: Do not add prefix or endl
      ss << record.getMessage();
    } else {
      ss << LogPrefix(record.getSeverity()) << record.getMessage() << std::endl;
    }
    return ss.str();
  }
};
} // namespace plog

namespace gits {
namespace log {

namespace {
static plog::ColorConsoleAppender<plog::GitsFormatter> consoleAppender;
static plog::DynamicAppender dynamicAppender;
} // namespace

void SetMaxSeverity(gits::LogLevel lvl) {
  auto severity = GetSeverity(lvl);
  plog::get()->setMaxSeverity(severity);
  plog::get<GITS_LOG_INSTANCE_ID_RAW>()->setMaxSeverity(severity);
}

plog::Severity GetSeverity(gits::LogLevel lvl) {
  switch (lvl) {
  case gits::LogLevel::OFF:
    return plog::Severity::none;
  case gits::LogLevel::ERR:
    return plog::Severity::error;
  case gits::LogLevel::WARN:
    return plog::Severity::warning;
  case gits::LogLevel::INFO:
    return plog::Severity::info;
  case gits::LogLevel::INFOV:
    return plog::Severity::info;
  case gits::LogLevel::TRACE:
    return plog::Severity::debug;
  case gits::LogLevel::TRACEV:
    return plog::Severity::verbose;
  default:
    return plog::Severity::none;
  }
}

void Initialize(gits::LogLevel lvl) {
  auto severity = GetSeverity(lvl);
  plog::init(severity);
  plog::get()->addAppender(&dynamicAppender);

#ifdef _WIN32
  static plog::DebugOutputAppender<plog::MessageOnlyFormatter> debugAppender;
  plog::get()->addAppender(&debugAppender);
#endif

  // Instance used for RAW trace formatting
  plog::init<GITS_LOG_INSTANCE_ID_RAW>(severity, plog::get());
}

void Initialize(gits::LogLevel lvl, plog::IAppender* appender) {
  auto severity = GetSeverity(lvl);
  plog::init(severity, appender);
  plog::init<GITS_LOG_INSTANCE_ID_RAW>(severity, appender);
}

void AddConsoleAppender() {
  dynamicAppender.addAppender(&consoleAppender);
}

void RemoveConsoleAppender() {
  dynamicAppender.removeAppender(&consoleAppender);
}

void AddFileAppender(const std::filesystem::path& logFilePath) {
  static std::unique_ptr<plog::IAppender> fileAppender;
  if (logFilePath.empty()) {
    return;
  }

  // Filename format: gits_<pid>.log
  std::string fileName = "gits_" + std::to_string(getpid()) + ".log";
  std::filesystem::path logFile = logFilePath / fileName;
  fileAppender = std::make_unique<plog::RollingFileAppender<plog::GitsFormatter>>(logFile.c_str());

  plog::get()->addAppender(fileAppender.get());
}

bool ShouldLog(gits::LogLevel lvl) {
  return plog::get()->checkSeverity(GetSeverity(lvl));
}
} // namespace log
} // namespace gits

void GitsAssert(bool condition,
                const char* conditionStr,
                const std::string& msg,
                const std::source_location& loc) {
  if (condition) {
    return;
  }

  LOG_ERROR << "Assertion failed: " << conditionStr;
  if (!msg.empty()) {
    LOG_ERROR << msg;
  }
  LOG_ERROR << "  Function: " << loc.function_name();
  LOG_ERROR << "  File: " << loc.file_name();
  LOG_ERROR << "  Line: " << loc.line();

#ifdef NDEBUG
  // In release mode, use quick_exit to terminate the program immediately
  std::quick_exit(EXIT_FAILURE);
#else
  // In debug mode, use standard assert to trigger debugger break (and optionally ignore)
  assert(condition);
#endif
}
