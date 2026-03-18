// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "launcherConfig.h"
#include "log.h"

namespace {
// anonymous namespace for YAML keys to avoid typos and for easier refactoring
static constexpr const char* WINDOW_SIZE_KEY = "windowSize";
static constexpr const char* WINDOW_POS_KEY = "windowPos";
static constexpr const char* UI_SCALE_KEY = "uiScale";
static constexpr const char* THEME_KEY = "theme";
} // namespace

namespace gits::gui {
LauncherConfig::LauncherConfig() {
  WindowPos = ImVec2(::Settings::WINDOW_POS_X, ::Settings::WINDOW_POS_Y);
  WindowSize = ImVec2(::Settings::WINDOW_SIZE_WIDTH, ::Settings::WINDOW_SIZE_HEIGHT);

  FileLocation = GetGITSLauncherConfigPath();
  UIScale = 1.0f;
}

std::filesystem::path LauncherConfig::GetGITSLauncherConfigPath() {
  const std::string& appName = "gitsLauncher";
  std::filesystem::path home;

#ifdef _WIN32
  char* appData = getenv("APPDATA");
  if (appData) {
    home = std::filesystem::path(appData) / std::filesystem::path(appName);
  }
#else
  char* homeDir = getenv("HOME");
  if (homeDir) {
    home = std::filesystem::path(homeDir) / std::filesystem::path(".config") / appName;
  }
#endif

  try {
    std::filesystem::create_directories(home);
  } catch (const std::exception& e) {
    LOG_ERROR << "Couldn't create directory for launcher config: " << home
              << " Error: " << e.what();

    return std::filesystem::path();
  }
  return home / "gitsLauncherConfig.yaml";
}

// Return by value (avoid returning reference to local)
LauncherConfig LauncherConfig::FromFile(const std::filesystem::path& path) {
  LauncherConfig config;

  if (!path.empty()) {
    config.FileLocation = path;
  }

  const auto& launcherConfigPath = config.FileLocation;
  if (!std::filesystem::exists(launcherConfigPath)) {
    return config;
  }

  try {
    std::ifstream file(launcherConfigPath);
    if (!file.is_open()) {
      return config;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    YAML::Node yaml = YAML::Load(content);

    config.Paths.Read(yaml);
    if (yaml[WINDOW_SIZE_KEY]) {
      const auto sizeNode = yaml[WINDOW_SIZE_KEY];
      if (sizeNode.IsSequence() && sizeNode.size() == 2) {
        config.WindowSize = ImVec2(sizeNode[0].as<double>(), sizeNode[1].as<double>());
      }
    }
    if (yaml[WINDOW_POS_KEY]) {
      const auto posNode = yaml[WINDOW_POS_KEY];
      if (posNode.IsSequence() && posNode.size() == 2) {
        config.WindowPos = ImVec2(posNode[0].as<double>(), posNode[1].as<double>());
      }
    }
    if (yaml[UI_SCALE_KEY]) {
      config.UIScale = yaml[UI_SCALE_KEY].as<float>();
    }
    if (yaml[THEME_KEY]) {
      const auto themeID = yaml[THEME_KEY].as<std::string>();
      if (!themeID.empty()) {
        config.Theme.SetThemeByID(themeID);
      }
    }
    return config;
  } catch (const std::exception& e) {
    LOG_ERROR << "Error loading launcher config: " << e.what() << std::endl;
  }

  return config;
}

bool LauncherConfig::ToFile(const std::filesystem::path& path) {
  if (!path.empty()) {
    FileLocation = path;
  }
  const auto& launcherConfigPath = FileLocation;

  try {
    std::filesystem::create_directories(launcherConfigPath.parent_path());

    YAML::Emitter out;
    out << YAML::BeginMap;
    Paths.Write(out);
    out << YAML::Key << WINDOW_SIZE_KEY << YAML::Value;
    out << YAML::BeginSeq << WindowSize.x << WindowSize.y << YAML::EndSeq;
    out << YAML::Key << WINDOW_POS_KEY << YAML::Value;
    out << YAML::BeginSeq << WindowPos.x << WindowPos.y << YAML::EndSeq;
    out << YAML::Key << UI_SCALE_KEY << YAML::Value << UIScale;
    out << YAML::Key << THEME_KEY << YAML::Value << Theme.CurThemeID();
    out << YAML::EndMap;

    std::ofstream file(launcherConfigPath);
    if (!file.is_open()) {
      return false;
    }

    file << out.c_str();
    file.close();

    return true;

  } catch (const std::exception& e) {
    LOG_ERROR << "Error saving launcher config: " << e.what() << std::endl;
    return false;
  }
}

} // namespace gits::gui
