// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "configurator.h"

#include <filesystem>
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <fstream>
#include <regex>

#include "configurationAuto.h"
#include "configurationYAMLAuto.h"
#include "lua_bindings.h"
#include "configUtils.h"
#include "gits.h"
#include "tools.h"
#include "exception.h"
#include "diagnostic.h"
#include "log.h"

namespace {
bool g_configurationValid = false;
}

namespace gits {
Configurator::Configurator() : configuration(g_configurationValid) {}

bool Configurator::ConfigurationValid() {
  return g_configurationValid;
}

#ifdef GITS_PLATFORM_WINDOWS
void Configurator::PrepareSubcapturePath() {
  auto& cfg = GetMutable();
  std::string preparedPath = cfg.common.player.subcapturePath.string();

  //handle special names in subcapture path:
  //  %f% - original filename
  //  %r% - frames range
  const std::string filenamePlaceholder = "%f%";
  std::string::size_type filenamePos = preparedPath.find(filenamePlaceholder);
  if (filenamePos != std::string::npos) {
    std::stringstream str;
    str << cfg.common.player.streamDir.filename().string();
    std::string left = preparedPath.substr(0, filenamePos);
    std::string right = preparedPath.substr(filenamePos + filenamePlaceholder.size());
    preparedPath = left + str.str() + right;
  }

  const std::string rangePlaceholder = "%r%";
  std::string::size_type rangePos = preparedPath.find(rangePlaceholder);
  if (rangePos != std::string::npos) {
    std::stringstream str;
    str << "frames-" << cfg.directx.features.subcapture.frames;
    std::string left = preparedPath.substr(0, rangePos);
    std::string right = preparedPath.substr(rangePos + rangePlaceholder.size());
    preparedPath = left + str.str() + right;
  }

  cfg.common.player.subcapturePath = preparedPath;
}
#endif

Configurator& Configurator::Instance() {
  static Configurator instance;
  return instance;
}

const Configuration& Configurator::Get() {
  return Instance().configuration;
}

Configuration& Configurator::GetMutable() {
  return Instance().configuration;
}

void Configurator::UpdateFromEnvironment() {
  Instance().configuration.updateFromEnvironment();
}

void Configurator::DeriveData() {
  configuration.DeriveData(configuration);
}

bool Configurator::LoadInto(const std::filesystem::path& filepath, Configuration* config) {
  std::ifstream fin(filepath);
  if (!fin) {
    LOG_ERROR << "Failed to open file: " << filepath << std::endl;
    return false;
  }

  try {
    YAML::Node node = YAML::Load(fin);
    if (YAML::convert<Configuration>::decode(node, *config)) {
      return true;
    } else {
      LOG_ERROR << "Failed to decode YAML to Configuration" << std::endl;
      return false;
    }
  } catch (const YAML::ParserException& e) {
    LOG_ERROR << "YAML Parser Exception: " << e.what() << std::endl;
    LOG_ERROR << "Structural inconsistency: please check the syntax and indentation at the "
                 "specified location."
              << std::endl;
    return false;
  } catch (const YAML::BadConversion& e) {
    LOG_ERROR << "YAML Bad Conversion Exception: " << e.what() << std::endl;
    LOG_ERROR << "Ensure the data types in the YAML file match the expected types in the "
                 "Configuration class."
              << std::endl;
    return false;
  } catch (const YAML::Exception& e) {
    LOG_ERROR << "YAML Exception: " << e.what() << std::endl;
    return false;
  }
}

bool Configurator::Load(const std::filesystem::path& filepath) {
  return LoadInto(filepath, &configuration);
}

bool Configurator::Save(const std::filesystem::path& filepath, const Configuration& config) {
  try {
    YAML::Node node = YAML::convert<Configuration>::encode(config);
    std::ofstream fout(filepath);
    if (!fout) {
      LOG_ERROR << "Failed to open file for writing: " << filepath << std::endl;
      return false;
    }
    fout << node;
    fout.close();
    return true;
  } catch (const std::exception& e) {
    LOG_ERROR << "Exception during save: " << e.what() << std::endl;
    return false;
  }
}

bool Configurator::ApplyOverrides(const std::filesystem::path& filepath,
                                  const std::string& processName) {
  std::ifstream fin(filepath);
  if (!fin) {
    LOG_ERROR << "Failed to open file: " << filepath << std::endl;
    return false;
  }

  try {
    YAML::Node node = YAML::Load(fin);
    if (!node["Overrides"]) {
      return false;
    }
    if (node["Overrides"][processName]) {
      if (YAML::convert<Configuration>::decode(node["Overrides"][processName], configuration)) {
        return true;
      } else {
        LOG_ERROR << "Failed to decode YAML to Configuration" << std::endl;
        return false;
      }
    }
  } catch (const YAML::Exception& e) {
    LOG_ERROR << "YAML Exception reading overrides: " << e.what() << std::endl;
    return false;
  }
  return false;
}

void Configurator::ClearChangedFieldsVector() {
  // delete all entries in changedFields map
  // create an empty map with vectors
  changedFields.clear();
  for (auto source : ConfigEntry::SOURCES) {
    changedFields[source] = std::vector<ConfigEntry>();
  }
}

void Configurator::AddChangedField(const std::string& path,
                                   const std::string& value,
                                   const std::string& defaultValue,
                                   const ConfigEntry::Source source) {
  changedFields[source].emplace_back(ConfigEntry{path, value, defaultValue, source});
}

void Configurator::LogChangedFields() {
  auto isAnyEmpty =
      std::any_of(ConfigEntry::SOURCES, std::end(ConfigEntry::SOURCES),
                  [this](const auto& source) { return !changedFields[source].empty(); });
  if (!isAnyEmpty) {
    return;
  }
  LOG_INFO << "The following config values are changed from default:";
  for (const auto& source : ConfigEntry::SOURCES) {
    if (changedFields[source].empty()) {
      continue;
    }
    LOG_INFO << "-- Set via " << ConfigEntry::toString(source) << ":";
    for (const auto& entry : changedFields[source]) {
      LOG_INFO << "  --" << entry.Path << "=\"" << entry.Value << "\"";
    }
  }
}

} // namespace gits
