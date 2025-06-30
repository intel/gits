// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "configurator.h"

#include <filesystem>
#ifndef BUILD_FOR_CCODE
#include <yaml-cpp/yaml.h>
#endif
#include <iostream>
#include <fstream>
#include <regex>

#include "configurationAuto.h"

#ifndef BUILD_FOR_CCODE
#include "configurationYAMLAuto.h"
#include "lua_bindings.h"
#include "configUtils.h"
#endif

#include "gits.h"
#include "tools.h"
#include "exception.h"

#ifndef BUILD_FOR_CCODE
#include "diagnostic.h"
#endif

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

#ifndef BUILD_FOR_CCODE
void Configurator::DeriveData() {
  configuration.DeriveData(configuration);
}

bool Configurator::LoadInto(const std::filesystem::path filepath, Configuration* config) {
  std::ifstream fin(filepath);
  if (!fin) {
    Log(ERR) << "Failed to open file: " << filepath << std::endl;
    return false;
  }

  try {
    YAML::Node node = YAML::Load(fin);
    if (YAML::convert<Configuration>::decode(node, *config)) {
      return true;
    } else {
      Log(ERR) << "Failed to decode YAML to Configuration" << std::endl;
      return false;
    }
  } catch (const YAML::ParserException& e) {
    Log(ERR) << "YAML Parser Exception: " << e.what() << std::endl;
    Log(ERR) << "Structural inconsistency: please check the syntax and indentation at the "
                "specified location."
             << std::endl;
    return false;
  } catch (const YAML::BadConversion& e) {
    Log(ERR) << "YAML Bad Conversion Exception: " << e.what() << std::endl;
    Log(ERR) << "Ensure the data types in the YAML file match the expected types in the "
                "Configuration class."
             << std::endl;
    return false;
  } catch (const YAML::Exception& e) {
    Log(ERR) << "YAML Exception: " << e.what() << std::endl;
    return false;
  }
}

bool Configurator::Load(const std::filesystem::path& filepath) {
  return LoadInto(filepath, &configuration);
}

bool Configurator::Save(const std::filesystem::path filepath, const Configuration& config) {
  try {
    YAML::Node node = YAML::convert<Configuration>::encode(config);
    std::ofstream fout(filepath);
    if (!fout) {
      Log(ERR) << "Failed to open file for writing: " << filepath << std::endl;
      return false;
    }
    fout << node;
    fout.close();
    return true;
  } catch (const std::exception& e) {
    Log(ERR) << "Exception during save: " << e.what() << std::endl;
    return false;
  }
}

bool Configurator::ApplyOverrides(const std::filesystem::path filepath, std::string processName) {
  std::ifstream fin(filepath);
  if (!fin) {
    Log(ERR) << "Failed to open file: " << filepath << std::endl;
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
        Log(ERR) << "Failed to decode YAML to Configuration" << std::endl;
        return false;
      }
    }
  } catch (const YAML::Exception& e) {
    Log(ERR) << "YAML Exception reading overrides: " << e.what() << std::endl;
    return false;
  }
  return false;
}
#endif
} // namespace gits
