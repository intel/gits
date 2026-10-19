// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsrPluginConfig.h"

#include <yaml-cpp/yaml.h>

#include <string>
#include <utility>

namespace gits::rtsr {
namespace {

template <typename T>
T ReadScalar(const YAML::Node& node, const char* key, T fallback) {
  try {
    const auto value = node[key];
    if (value && value.IsScalar()) {
      return value.as<T>();
    }
  } catch (const YAML::Exception&) {
  }
  return fallback;
}

} // namespace

PluginConfig ReadPluginConfig(const std::filesystem::path& path, LogSink log) {
  PluginConfig config;
  try {
    const auto yaml = YAML::LoadFile(path.string());
    const auto section = yaml["Config"];
    if (section && section.IsMap()) {
      config.Endpoints.Address =
          ReadScalar<std::string>(section, "ServerAddress", config.Endpoints.Address);
      config.Endpoints.ServerPushPort =
          ReadScalar(section, "ServerPushPort", config.Endpoints.ServerPushPort);
      config.Endpoints.ServerRequestPort =
          ReadScalar(section, "ServerRequestPort", config.Endpoints.ServerRequestPort);
      config.Endpoints.ClientRequestPort =
          ReadScalar(section, "ClientRequestPort", config.Endpoints.ClientRequestPort);
      config.ConnectTimeoutMs = ReadScalar(section, "ConnectTimeoutMs", config.ConnectTimeoutMs);
    }
  } catch (const YAML::Exception& error) {
    if (log) {
      log("Rtsr - could not read " + path.string() + " (" + error.what() + "), using defaults");
    }
  }
  return config;
}

} // namespace gits::rtsr
