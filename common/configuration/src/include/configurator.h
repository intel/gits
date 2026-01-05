// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <optional>
#include <map>
#include <string>
#include <vector>

#include "configurationAuto.h"
#include "platform.h"
#include "tools_lite.h"

namespace gits {

class Configurator : public gits::noncopyable {
public:
  struct ConfigEntry {
    enum class Source {
      DEFAULT,
      CONFIG_FILE,
      ARGUMENT,
      ENVIRONMENT_VARIABLE
    };

    std::string Path;
    std::string Value;
    std::string Default;
    Source source;

    static std::string toString(Source source) {
      switch (source) {
      case Source::DEFAULT:
        return "default value";
      case Source::CONFIG_FILE:
        return "config file";
      case Source::ARGUMENT:
        return "argument";
      case Source::ENVIRONMENT_VARIABLE:
        return "environment variable";
      default:
        return "<?UNKNOWN?>";
      }
    }

    static constexpr Source SOURCES[3] = {Source::CONFIG_FILE, Source::ARGUMENT,
                                          Source::ENVIRONMENT_VARIABLE};
  };

  static bool ConfigurationValid();

  static bool IsRecorder() {
    return Get().common.mode == GITSMode::MODE_RECORDER;
  }

  static bool IsPlayer() {
    return Get().common.mode == GITSMode::MODE_PLAYER;
  }

#ifdef GITS_PLATFORM_WINDOWS
  // If more options start to use the ApiBool then we can come up with a generic helper
  static bool IsHudEnabledForApi(gits::ApiBool api) {
    if (std::find(Get().common.shared.hud.enabled.begin(), Get().common.shared.hud.enabled.end(),
                  api) != Get().common.shared.hud.enabled.end()) {
      return true;
    } else {
      return false;
    }
  }
#endif

#ifdef GITS_PLATFORM_WINDOWS
  static void PrepareSubcapturePath();
#endif

  static std::string ConfigFileName() {
    return "gits_config.yml";
  }

public: // Singleton
  static Configurator& Instance();
  static const Configuration& Get();
  static Configuration& GetMutable();

  static bool LoadInto(const std::filesystem::path& filepath, Configuration* config);
  bool Load(const std::filesystem::path& filepath);

  static bool Save(const std::filesystem::path& filepath, const Configuration& config);

  bool ApplyOverrides(const std::filesystem::path& filepath, const std::string& processName);
  void DeriveData();

  void ClearChangedFieldsVector();
  void AddChangedField(const std::string& path,
                       const std::string& value,
                       const std::string& defaultValue,
                       const ConfigEntry::Source source);
  void LogChangedFields();

public: // Configuration
  void UpdateFromEnvironment();

private:
  Configurator();

  Configuration configuration;
  std::map<ConfigEntry::Source, std::vector<ConfigEntry>> changedFields;
};
} // namespace gits
