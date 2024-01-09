// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "log.h"

namespace gits {
namespace ocloc {
class COclocLog : public gits::CLog {
public:
  using gits::CLog::CLog;
  template <typename T>
  COclocLog& operator<<(const T& t) {
    _buffer << t;
    return *this;
  }
  COclocLog& operator<<(const char c);
  COclocLog& operator<<(const unsigned char c);
  COclocLog& operator<<(const char* c);
  COclocLog& operator<<(char* c);
  COclocLog& operator<<(gits::manip t);
};
// See common/include/log.h for explanations of these macros.
#define OclocLog1(lvl)                                                                             \
  if (gits::ShouldLog(gits::LogLevel::lvl))                                                        \
  gits::ocloc::COclocLog(gits::LogLevel::lvl, gits::LogStyle::NORMAL)
#define OclocLog2(lvl, style)                                                                      \
  if (gits::ShouldLog(gits::LogLevel::lvl))                                                        \
  gits::ocloc::COclocLog(gits::LogLevel::lvl, gits::LogStyle::style)
// Workaround for a MSVC bug, see https://stackoverflow.com/a/5134656/
#define EXPAND(x) x
// Magic to call different variants based on the number of arguments.
#define GET_OVERLOAD(PLACEHOLDER1, PLACEHOLDER2, NAME, ...) NAME
#define OclocLog(...)                                       EXPAND(GET_OVERLOAD(__VA_ARGS__, OclocLog2, OclocLog1)(__VA_ARGS__))

void LogOclocInvokeInput(unsigned int argc,
                         const char** argv,
                         const uint32_t numSources,
                         const uint8_t** sources,
                         const uint64_t* sourceLens,
                         const char** sourcesNames,
                         const uint32_t numInputHeaders,
                         const uint8_t** dataInputHeaders,
                         const uint64_t* lenInputHeaders,
                         const char** nameInputHeaders);
void LogOclocInvokeOutput(int ret,
                          uint32_t* numOutputs,
                          uint8_t*** dataOutputs,
                          uint64_t** lenOutputs,
                          char*** nameOutputs);
} // namespace ocloc
} // namespace gits
