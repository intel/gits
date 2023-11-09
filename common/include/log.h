// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   log.h
 *
 * @brief Declaration of Log class.
 *
 */

#pragma once

#include "platform.h"
#include "pragmas.h"
#include <sstream>
#include <type_traits>
#include <filesystem>

#ifdef GITS_PLATFORM_WINDOWS

#define HIDDEN

#else // _WIN32

#define HIDDEN __attribute__((visibility("hidden")))

#endif // _WIN32

namespace gits {

/**
  * @brief Class responsible for handling project logs
  *
  * gits::Log is a class that is responsible for handling
  * project logs. It stores information about which module
  * is currently used (player or recorder) and redirect messages
  * to appropriate target
  */

typedef std::ostream& (*manip)(std::ostream&); //support for manipulators like "endl"

// TODO: It would be nice to make these two enums "enum classes", which
// aren't really classes, but improved enums (mainly better typing and no
// implicit int conversions).
enum LogStyle {
  NORMAL,     // Trace: example log\n
  NO_NEWLINE, // Trace: example log
  NO_PREFIX,  // example log\n
  RAW,        // example log
};

enum LogLevel {
  TRACEV = 0, // Trace verbose. Like trace, but even more detailed.
  TRACE = 1,  // Trace. Detailed output of what program is doing, for advanced users.
  INFOV = 2,  // Info verbose. Like info but provides some additional information.
  INFO = 3,   // Info. Normal output, usually tells user what the program is doing.
  WARN = 4,   // Warning. Something potentially bad has happened, user might want to know.
  ERR = 5,    // Error. Something bad has happened and user needs to know.
  OFF = 6,    // Important. Special level that shows messages even when logging is off.
};

enum class TraceData { VK_STRUCTS, FRAME_NUMBER };

const TraceData TraceDataAll[] = {TraceData::VK_STRUCTS, TraceData::FRAME_NUMBER};

class CLog {
public:
  typedef void FPrintf(const char*);

  CLog(LogLevel lvl, LogStyle style) HIDDEN;
  CLog(const CLog& rhs) HIDDEN;
  CLog& operator=(const CLog& rhs) HIDDEN;
  ~CLog() HIDDEN;

  template <class T>
  CLog& operator<<(const T& t) HIDDEN;
  CLog& operator<<(const char c) {
    return operator<< <int>(c);
  }
  CLog& operator<<(const unsigned char c) {
    return operator<< <unsigned>(c);
  }

  CLog& operator<<(manip t) HIDDEN;

  static void SetLogLevel(LogLevel lvl) HIDDEN;
#ifndef BUILD_FOR_CCODE
  static void LogFile(const std::filesystem::path& dir) HIDDEN;
  static void LogFunction(FPrintf* func) HIDDEN;
  static void LogFilePlayer(const std::filesystem::path& dir) HIDDEN;
#endif

protected:
  std::stringstream _buffer;
  LogLevel _logLevel;
  LogStyle _style;

  FPrintf* _localPrintFunc;
};

bool ShouldLog(LogLevel lvl);

// CLog is stream-based and used like this: Log(ERR) << "something" << some_variable << "stuff";
// This means avoiding logging too low-level info is hard. We could check the level each time << is
// used, but this is suboptimal. We could wrap the whole statement into an if, which is optimal
// but user has to manually add ifs to every log call. Fortunately we can insert these ifs with
// macros.
//
// [MSVC WA] TODO: Remove this workaround when no longer needed.
// These macros used to look like this:
//   #define Log1(lvl) if (gits::ShouldLog(lvl)) gits::CLog(lvl, gits::NORMAL)
//   #define Log2(lvl, style) if (gits::ShouldLog(lvl)) gits::CLog(lvl, style)
// Unfortunately MSVC has no warning for dangling else (look it up if you don't know what that is)
// and missing braces might result in an unexpected control flow. That's why we replaced ifs with
// one-iteration for loops which is terribly ugly but it works.
#define Log1(lvl)                                                                                  \
  for (bool _done_ = false; !_done_ && gits::ShouldLog(gits::LogLevel::lvl); _done_ = true)        \
  gits::CLog(gits::LogLevel::lvl, gits::LogStyle::NORMAL)
#define Log2(lvl, style)                                                                           \
  for (bool _done_ = false; !_done_ && gits::ShouldLog(gits::LogLevel::lvl); _done_ = true)        \
  gits::CLog(gits::LogLevel::lvl, gits::LogStyle::style)
//
// Workaround for a MSVC bug, see https://stackoverflow.com/a/5134656/
#define EXPAND(x) x
//
// Magic to call different variants based on the number of arguments.
// Notice how the increasing number of arguments coming from __VA_ARGS__ pushes LogN arguments
// right. It ensures the correct LogN argument will be passed as NAME to the GET_OVERLOAD macro.
#define GET_OVERLOAD(PLACEHOLDER1, PLACEHOLDER2, NAME, ...) NAME
#define Log(...)                                            EXPAND(GET_OVERLOAD(__VA_ARGS__, Log2, Log1)(__VA_ARGS__))
} // namespace gits

// Scoped enums (enum class) unlike traditional enums are not implicitly cast onto their underlying types
template <class E, std::enable_if_t<std::is_enum<E>{}>* = nullptr>
std::ostream& operator<<(std::ostream& stm, E e) {
  return stm << static_cast<typename std::underlying_type<E>::type>(e);
}

template <class T>
gits::CLog& gits::CLog::operator<<(const T& t) {
  _buffer << t;
  return *this;
}
