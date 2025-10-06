// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <optional>

#include "configurationAuto.h"
#include "platform.h"
#include "tools_lite.h"

namespace gits {

class Configurator : public gits::noncopyable {
public:
  struct ConfigEntry {
    std::string Path;
    std::string Value;
    std::string Default;
  };

  static bool ConfigurationValid();

  static bool IsRecorder() {
    return Get().common.mode == GITSMode::MODE_RECORDER;
  }

  static bool IsPlayer() {
    return Get().common.mode == GITSMode::MODE_PLAYER;
  }

  static bool DumpBinary() {
    return Get().common.recorder.recordingMode == RecordingMode::BINARY;
  }

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

#ifndef BUILD_FOR_CCODE
  static bool LoadInto(const std::filesystem::path& filepath, Configuration* config);
  bool Load(const std::filesystem::path& filepath);

  static bool Save(const std::filesystem::path& filepath, const Configuration& config);

  bool ApplyOverrides(const std::filesystem::path& filepath, const std::string& processName);
  void DeriveData();

  void ClearChangedFieldsVector();
  const std::vector<ConfigEntry>& GetChangedFields() const;
  void AddChangedField(const std::string& path,
                       const std::string& value,
                       const std::string& defaultValue);
  void LogChangedFields();
#endif

public: // Configuration
  void UpdateFromEnvironment();

private:
  Configurator();

  Configuration configuration;
  std::vector<ConfigEntry> changedFields;
};
} // namespace gits
