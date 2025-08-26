// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"

namespace gits {

// Log message with LogLevel::INFO
template <typename... Args>
void log(CGits& gits, Args&&... args) {
  gits.GetMessageBus().publish(
      {PUBLISHER_PLUGIN, TOPIC_LOG},
      std::make_shared<LogMessage>(LogLevel::INFO, std::forward<Args>(args)...));
}

// Log message with LogLevel::WARN
template <typename... Args>
void logW(CGits& gits, Args&&... args) {
  gits.GetMessageBus().publish(
      {PUBLISHER_PLUGIN, TOPIC_LOG},
      std::make_shared<LogMessage>(LogLevel::WARN, std::forward<Args>(args)...));
}

// Log message with LogLevel::ERR
template <typename... Args>
void logE(CGits& gits, Args&&... args) {
  gits.GetMessageBus().publish(
      {PUBLISHER_PLUGIN, TOPIC_LOG},
      std::make_shared<LogMessage>(LogLevel::ERR, std::forward<Args>(args)...));
}

// Log message with LogLevel::TRACE
// The message to be also logged in the trace files if DirectX.Features.Trace is enabled
template <typename... Args>
void logT(CGits& gits, Args&&... args) {
  gits.GetMessageBus().publish(
      {PUBLISHER_PLUGIN, TOPIC_LOG},
      std::make_shared<LogMessage>(LogLevel::TRACE, std::forward<Args>(args)...));
}

} // namespace gits

namespace {
std::string FormatMemorySize(size_t bytes) {
  constexpr size_t kKiB = 1024;
  constexpr size_t kMiB = 1024 * kKiB;
  constexpr size_t kGiB = 1024 * kMiB;
  constexpr double slack = 0.05; // 5% slack

  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2);

  if (bytes < kMiB) {
    oss << bytes << " bytes";
  } else if (bytes < kGiB) {
    double mib = static_cast<double>(bytes) / kMiB;
    if (mib > (kGiB / static_cast<double>(kMiB)) - slack * (kGiB / static_cast<double>(kMiB))) {
      // If within 5% of 1 GiB, show as GiB
      double gib = static_cast<double>(bytes) / kGiB;
      oss << gib << " GiB";
    } else {
      oss << mib << " MiB";
    }
  } else {
    double gib = static_cast<double>(bytes) / kGiB;
    oss << gib << " GiB";
  }
  return oss.str();
}
} // namespace
