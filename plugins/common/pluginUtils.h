// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "messageBus.h"
#include "yaml-cpp/yaml.h"

#include <cstddef>
#include <filesystem>
#include <string>

namespace gits {
// Log message with LogLevel::TRACE
// The message to be also logged in the trace files if Common.Shared.Trace is enabled
template <typename... Args>
void logT(MessageBus* msgBus, Args&&... args) {
  GITS_ASSERT(msgBus != nullptr);
  msgBus->publish({PUBLISHER_PLUGIN, TOPIC_LOG},
                  std::make_shared<LogMessage>(LogLevel::TRACE, std::forward<Args>(args)...));
}

// Loads plugin config.yml and applies GITS_PLUGIN_<PLUGIN>_<KEY> environment overrides
// for scalar keys directly under Config.
YAML::Node LoadPluginConfig(const std::filesystem::path& configYamlPath,
                            const std::string& pluginName);

std::string FormatMemorySize(size_t bytes);
} // namespace gits
