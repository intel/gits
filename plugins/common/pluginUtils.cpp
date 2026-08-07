// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pluginUtils.h"

#include "log.h"

#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace gits {

YAML::Node LoadPluginConfig(const std::filesystem::path& configYamlPath,
                            const std::string& pluginName) {
  // Uppercase plugin name or config key for GITS_PLUGIN_<name>_<key> env var names
  const auto toSegment = [](std::string_view value) {
    std::string segment(value);
    for (char& c : segment) {
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return segment;
  };

  YAML::Node root = YAML::LoadFile(configYamlPath.string());
  YAML::Node config = root["Config"];
  if (!config || !config.IsMap()) {
    return root;
  }

  const std::string pluginSegment = toSegment(pluginName);
  if (pluginSegment.empty()) {
    return root;
  }

  for (auto it = config.begin(); it != config.end(); ++it) {
    if (!it->second || !it->second.IsScalar()) {
      continue;
    }

    const std::string key = it->first.Scalar();
    const std::string keySegment = toSegment(key);
    if (keySegment.empty()) {
      continue;
    }

    const std::string envVar = "GITS_PLUGIN_" + pluginSegment + "_" + keySegment;
    const char* raw = std::getenv(envVar.c_str());
    if (raw == nullptr) {
      continue;
    }

    LOG_INFO << pluginName << " - Config." << key << " overridden to \"" << raw << "\" via "
             << envVar;
    it->second = raw;
  }

  return root;
}

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

} // namespace gits
