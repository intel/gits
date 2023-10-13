// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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

namespace {
#ifndef BUILD_FOR_CCODE
std::unique_ptr<std::ofstream> _file;
#endif
gits::CLog::FPrintf* _func = nullptr;
std::ostream* _log = &std::cout;
std::ostream* _log_player_err = &std::cerr;
bool _log_to_file = false;
// Workaround; interceptors don't have normal config so we
// store the level in a global variable instead.
gits::LogLevel _thresholdLogLevel = gits::LogLevel::INFO;
#ifndef BUILD_FOR_CCODE
std::unique_ptr<std::mutex> _mutex;
#endif
} // namespace

bool gits::ShouldLog(gits::LogLevel lvl) {
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
    switch (_logLevel) {
    case TRACEV:
    case TRACE:
      _buffer << "Trace: ";
      break;
    case INFOV:
      _buffer << "InfoV: ";
      break;
    case INFO:
      _buffer << "Info: ";
      break;
    case WARN:
      _buffer << "Warn: ";
      break;
    case ERR:
      _buffer << "Err: ";
      break;
    case OFF: // Messages that should be shown even with logging turned off.
      _buffer << "Important: ";
      break;
    default:
      throw std::invalid_argument((std::string)EXCEPTION_MESSAGE +
                                  " Created a CLog with invalid LogLevel.");
      break;
    }
  }
}

gits::CLog::CLog(const CLog& rhs)
    : _buffer(rhs._buffer.str()), _style(rhs._style), _localPrintFunc(rhs._localPrintFunc) {
#ifndef BUILD_FOR_CCODE
  if (!_mutex.get()) {
    _mutex.reset(new std::mutex());
  }
  if (!_file.get()) {
    _file.reset(new std::ofstream);
  }
#endif
}

void gits::CLog::SetLogLevel(LogLevel lvl) {
  _thresholdLogLevel = lvl;
}

#ifndef BUILD_FOR_CCODE
void gits::CLog::LogFile(const std::filesystem::path& dir) {
  if (_file.get() == nullptr) {
    _file.reset(new std::ofstream);
  }

  _log_to_file = true;

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
    _file->open(logPath / (str.str() + ".log"), std::ios_base::app);
    _func = nullptr;
  }
}

void gits::CLog::LogFunction(FPrintf* func) {
  _func = func;
  if (_file.get() != nullptr) {
    _file->close();
  }
  _log_to_file = false;
  _log = nullptr;
  _log_player_err = nullptr;
}

void gits::CLog::LogFilePlayer(const std::filesystem::path& dir) {
  if (_file.get() == nullptr) {
    _file.reset(new std::ofstream);
  }

  _log_to_file = true;

  if (!_file->is_open()) {
    _log = _file.get();
    _file->open(dir, std::ios_base::out);
    _func = nullptr;
  }
}

#endif

gits::CLog::~CLog() {
#ifndef BUILD_FOR_CCODE
  std::unique_lock<std::mutex> lock(*_mutex);
#endif
  if (_style == NORMAL || _style == NO_PREFIX) {
    _buffer << std::endl;
  }
#ifdef GITS_COMMON_PROJ
  if (gits::Config::Get().common.useEvents && gits::CGits::InstancePtr() != nullptr) {
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
    if (_log_to_file || _logLevel < LogLevel::WARN) {
      *_log << _buffer.str();
      _log->flush();
    } else {
      *_log_player_err << _buffer.str();
      _log_player_err->flush();
    }
  }
}

gits::CLog& gits::CLog::operator<<(manip t) {
  _buffer << t;
  return *this;
}
