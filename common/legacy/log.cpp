// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   log.cpp
 *
 * @brief Definition of log class.
 *
 */

#include "log.h"
#include "exception.h"
#ifdef GITS_COMMON_PROJ
#include "gits.h"
#endif
#include <iostream>
#include <vector>
#include <fstream>
#include <mutex>

#ifdef GITS_PLATFORM_WINDOWS
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif
#include "tools_lite.h"

// Workaround; interceptors don't have normal config so we
// store config values in global variables instead.
namespace {
#ifndef BUILD_FOR_CCODE
std::unique_ptr<std::ofstream> _file;
#endif
gits::CLog::FPrintf* _func = nullptr;
std::ostream* _log = &std::cout;
std::ostream* _logPlayerErr = &std::cerr;
bool _logToFile = false;
gits::LogLevel _thresholdLogLevel = gits::LogLevel::INFO;
bool _logToConsole = false;
#ifndef BUILD_FOR_CCODE
std::unique_ptr<std::mutex> _mutex;
#endif
} // namespace

static std::string getCurrentDateTimestamp() {
  std::stringstream currentDate;
  const auto time = std::chrono::system_clock::now();
  const auto tTime = std::chrono::system_clock::to_time_t(time);
  const auto localTime = std::localtime(&tTime);
  const auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()) % 1000;
  // TODO: When C++20 becomes available, use std::formatter instead.
  currentDate << std::put_time(localTime, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0')
              << std::setw(3) << ms.count();
  return currentDate.str();
}

bool gits::ShouldLog(LogLevel lvl) {
  // We can't use config directly because e.g. interceptors don't have normal
  // config.
  return lvl >= _thresholdLogLevel;
}

gits::CLog::CLog(LogLevel lvl, LogStyle style)
    : _logLevel(lvl), _style(style), _localPrintFunc(nullptr) {
#ifndef BUILD_FOR_CCODE
  if (!_mutex.get()) {
    _mutex.reset(new std::mutex());
  }
  if (!_file.get()) {
    _file.reset(new std::ofstream);
  }
#endif
  if (_style != NO_PREFIX && _style != RAW) {
    const auto currentDateTimestamp = getCurrentDateTimestamp();
    _buffer << currentDateTimestamp << " - ";
    switch (_logLevel) {
    case LogLevel::TRACEV:
    case LogLevel::TRACE:
      _buffer << "Trace - ";
      break;
    case LogLevel::INFOV:
      _buffer << "InfoV - ";
      break;
    case LogLevel::INFO:
      _buffer << "Info - ";
      break;
    case LogLevel::WARN:
      _buffer << "Warn - ";
      break;
    case LogLevel::ERR:
      _buffer << "Err - ";
      break;
    case LogLevel::OFF:
      _buffer << "Important - ";
      break;
    default:
      throw std::invalid_argument((std::string)EXCEPTION_MESSAGE +
                                  " Created a CLog with invalid LogLevel.");
      break;
    }
  }
}

gits::CLog::CLog(const CLog& rhs)
    : _buffer(rhs._buffer.str()),
      _logLevel(rhs._logLevel),
      _style(rhs._style),
      _localPrintFunc(rhs._localPrintFunc) {
#ifndef BUILD_FOR_CCODE
  if (!_mutex.get()) {
    _mutex.reset(new std::mutex());
  }
  if (!_file.get()) {
    _file.reset(new std::ofstream);
  }
#endif
}

gits::CLog& gits::CLog::operator=(const CLog& rhs) {
  if (this != &rhs) {
    _buffer.str(rhs._buffer.str());
    _logLevel = rhs._logLevel;
    _style = rhs._style;
    _localPrintFunc = rhs._localPrintFunc;
#ifndef BUILD_FOR_CCODE
    if (!_mutex.get()) {
      _mutex.reset(new std::mutex());
    }
    if (!_file.get()) {
      _file.reset(new std::ofstream);
    }
#endif
  }
  return *this;
}

void gits::CLog::SetLogLevel(LogLevel lvl) {
  _thresholdLogLevel = lvl;
}

void gits::CLog::SetLogToConsole(bool logToConsole) {
  _logToConsole = logToConsole;
}

#ifndef BUILD_FOR_CCODE
void gits::CLog::LogFile(const std::filesystem::path& dir) {
  if (_file.get() == nullptr) {
    _file.reset(new std::ofstream);
  }

  _logToFile = true;

  if (!_file->is_open()) {
    _log = _file.get();
    std::stringstream str;
    str << "gits_";
    str << getpid();
    std::filesystem::path logPath(dir);
    const auto logPathEnv = getenv("GITS_CUSTOM_LOG_PATH");
    if (logPathEnv != nullptr) {
      logPath = logPathEnv;
    }
    _file->open(logPath / (str.str() + ".legacy.log"), std::ios_base::app);
    _func = nullptr;
  }
}

void gits::CLog::LogFunction(FPrintf* func) {
  _func = func;
  if (_file.get() != nullptr) {
    _file->close();
  }
  _logToFile = false;
  _log = nullptr;
  _logPlayerErr = nullptr;
}

void gits::CLog::LogFilePlayer(const std::filesystem::path& dir) {
  if (_file.get() == nullptr) {
    _file.reset(new std::ofstream);
  }

  _logToFile = true;

  if (!_file->is_open()) {
    _log = _file.get();
    _file->open(dir, std::ios_base::out);
    _func = nullptr;
  }
}

#endif

gits::CLog::~CLog() {
  try {
#ifndef BUILD_FOR_CCODE
    std::unique_lock<std::mutex> lock(*_mutex);
#endif
    if (_style == NORMAL || _style == NO_PREFIX) {
      _buffer << std::endl;
    }
#ifdef GITS_COMMON_PROJ
    // We check for validity of configuration here in case we are logging errors
    // before configuration could be properly initialized
    if (Configurator::ConfigurationValid() && Configurator::Get().common.shared.useEvents &&
        gits::CGits::InstancePtr() != nullptr) {
      gits::CGits::Instance().PlaybackEvents().logging(_buffer.str().c_str());
    }
#endif

    if (_localPrintFunc) {
      _localPrintFunc(_buffer.str().c_str());
    } else if (_func) {
      _func(_buffer.str().c_str());
    } else {
      // At first these point to stdout and stderr respectively; we print
      // warnings to the latter. If logging to file is requested, we point _log
      // to that file and print everything there.
      if (_logToFile || _logLevel < LogLevel::WARN) {
        *_log << _buffer.str();
        _log->flush();
        if (_logToConsole) {
          std::cout << _buffer.str();
          std::cout.flush();
        }
      } else {
        *_logPlayerErr << _buffer.str();
        _logPlayerErr->flush();
      }
    }
  } catch (...) {
    topmost_exception_handler("CLog::~CLog");
  }
}

gits::CLog& gits::CLog::operator<<(manip t) {
  _buffer << t;
  return *this;
}
