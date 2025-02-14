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
