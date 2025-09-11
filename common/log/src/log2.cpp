// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "log2.h"

// Plog
#include <plog/Formatters/MessageOnlyFormatter.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
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
std::mutex FormatRawScope::s_mutex = {};

FormatRawScope::FormatRawScope() : lock_(s_mutex) {
  s_isRaw = true;
}

FormatRawScope::~FormatRawScope() {
  s_isRaw = false;
}

bool FormatRawScope::IsRaw() {
  return s_isRaw;
}

class GitsFormatter {
public:
  static util::nstring header() {
    return util::nstring();
  }

  static util::nstring format(const Record& record) {
    auto toStr = [](Severity severity) {
      switch (severity) {
      case fatal:
        return "FATAL";
      case error:
        return "ERROR";
      case warning:
        return "WARN";
      case info:
        return "INFO";
      case debug:
        return "DEBUG";
      case verbose:
        return "VERB";
      default:
        return "NONE";
      }
    };

    util::nostringstream ss;
    if (FormatRawScope::IsRaw()) {
      ss << record.getMessage();
    } else {
      auto now = std::chrono::system_clock::now();
      auto timeT = std::chrono::system_clock::to_time_t(now);
      auto ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

      // Print formatted date and time on a pre-allocated buffer
      char buffer[32];
      size_t offset =
          std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&timeT));
      std::snprintf(&buffer[offset], sizeof(buffer) - offset, ".%03d",
                    static_cast<int>(ms.count()));

      ss << static_cast<char*>(buffer) << " - " << toStr(record.getSeverity()) << " - "
         << record.getMessage() << "\n";
    }

    return ss.str();
  }
};
} // namespace plog

namespace gits {
namespace log {
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
  static plog::ConsoleAppender<plog::GitsFormatter> consoleAppender;
  plog::init(severity).addAppender(&consoleAppender);

#ifdef _WIN32
  static plog::DebugOutputAppender<plog::MessageOnlyFormatter> debugAppender;
  plog::get()->addAppender(&debugAppender);
#endif
}

void SetLogFile(const std::filesystem::path& logFilePath) {
  static std::unique_ptr<plog::IAppender> fileAppender;
  if (logFilePath.empty()) {
    return;
  }

  // Filename format: gits_<pid>.0.log
  // In the future, the "0" may be replaced by the plog instance id
  std::string fileName = "gits_" + std::to_string(getpid()) + ".0.log";
  std::filesystem::path logFile = logFilePath / fileName;
  fileAppender = std::make_unique<plog::RollingFileAppender<plog::GitsFormatter>>(logFile.c_str());

  plog::get()->addAppender(fileAppender.get());
}
}; // namespace log
}; // namespace gits
