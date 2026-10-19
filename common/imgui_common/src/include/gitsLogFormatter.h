// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <chrono>
#include <ctime>

#include "log.h"
#include <plog/Record.h>
#include <plog/Util.h>
#include <cstdio>

namespace plog {
class GITSLogFormatter {
public:
  static util::nstring header() {
    return util::nstring();
  }

  static util::nstring format(const Record& record) {
    auto toStr = [](plog::Severity severity) {
      switch (severity) {
      case plog::fatal:
        return "(F)";
      case plog::error:
        return "(E)";
      case plog::warning:
        return "(W)";
      case plog::info:
        return "(I)";
      case plog::debug:
        return "(D)";
      case plog::verbose:
        return "(V)";
      default:
        return "(-)";
      }
    };

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timeT = std::chrono::system_clock::to_time_t(now);

    // Print formatted date and time on a pre-allocated buffer
    char buffer[32];
    size_t offset = std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&timeT));
    std::snprintf(&buffer[offset], sizeof(buffer) - offset, ".%03d", static_cast<int>(ms.count()));

    util::nostringstream prefixSs;
    prefixSs << static_cast<char*>(buffer) << " " << toStr(record.getSeverity()) << " ";

    util::nostringstream ss;
    ss << prefixSs.str() << record.getMessage() << std::endl;
    return ss.str();
  }
};
} // namespace plog
