// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "log.h"

// Plog
#include <plog/Appenders/ConsoleAppender.h>
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

std::atomic<bool> FormatRawScope::s_isRaw = false;
std::recursive_mutex FormatRawScope::s_mutex = {};
unsigned FormatRawScope::s_refCount = 0;

FormatRawScope::FormatRawScope() : lock_(s_mutex) {
  if (s_refCount == 0) {
    s_isRaw = true;
  }
  ++s_refCount;
}

FormatRawScope::~FormatRawScope() {
  --s_refCount;
  if (s_refCount == 0) {
    s_isRaw = false;
  }
}

bool FormatRawScope::IsRaw() {
  return s_isRaw;
}

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
  auto timeT = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

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
    if (FormatRawScope::IsRaw()) {
      ss << record.getMessage();
    } else {
      ss << LogPrefix(record.getSeverity()) << record.getMessage() << "\n";
    }
    return ss.str();
  }
};
} // namespace plog

namespace gits {
namespace log {

namespace {
static plog::ConsoleAppender<plog::GitsFormatter> consoleAppender;
static plog::DynamicAppender dynamicAppender;
} // namespace

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

void Initialize(plog::Severity severity) {
  plog::init(severity);
  plog::get()->addAppender(&dynamicAppender);

#ifdef _WIN32
  static plog::DebugOutputAppender<plog::MessageOnlyFormatter> debugAppender;
  plog::get()->addAppender(&debugAppender);
#endif
}

void AddConsoleAppender() {
  dynamicAppender.addAppender(&consoleAppender);
}

void RemoveConsoleAppender() {
  dynamicAppender.removeAppender(&consoleAppender);
}

void SetLogFile(const std::filesystem::path& logFilePath) {
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
