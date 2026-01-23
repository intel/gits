// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "launcherConfig.h"

namespace gits::gui {
LauncherConfig::LauncherConfig() {
  WindowPos = ImVec2(::Settings::WINDOW_POS_X, ::Settings::WINDOW_POS_Y);
  WindowSize = ImVec2(::Settings::WINDOW_SIZE_WIDTH, ::Settings::WINDOW_SIZE_HEIGHT);

  StreamPath = "";
  TargetPath = "";

  GITSPlayerPath = "";
  ConfigPath = "";

  CustomArguments = "";
  FileLocation = GetGITSLauncherConfigPath();
  UIScale = 1.0f;
}

void LauncherConfig::DetectBasePaths() {
  // the parent folder of the folder in which the executable being run is:
  // Get the parent folder of the folder in which the executable is being run
#ifdef _WIN32
  char exePath[MAX_PATH];
  GetModuleFileNameA(nullptr, exePath, MAX_PATH);
  std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
#else
  char exePath[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", exePath, PATH_MAX);
  std::filesystem::path exeDir =
      std::filesystem::path(std::string(exePath, (count > 0) ? count : 0)).parent_path();
#endif
  const std::string gitsPlayer = "gitsPlayer.exe";

  // Case 1: installation: Player is relative to Launcher
  GITSPlayerPath = exeDir / "Player" / gitsPlayer;
  if (std::filesystem::exists(GITSPlayerPath)) {
    GITSBasePath = exeDir;
  } else {
    // Case 2: VS development: Player is in ../GITS/Player relative to Launcher
    GITSPlayerPath = exeDir.parent_path().parent_path() / "player" / "Debug" / gitsPlayer;
    if (std::filesystem::exists(GITSPlayerPath)) {
      GITSBasePath = exeDir.parent_path().parent_path() / "dist";
    } else {
      GITSPlayerPath = "";
    }
  }
  ConfigPath = "";
  if (!GITSPlayerPath.empty()) {
    ConfigPath = GITSPlayerPath.parent_path() / "gits_config.yml";
    if (!std::filesystem::exists(ConfigPath)) {
      ConfigPath = "";
    }
  }
  if (!GITSBasePath.empty() && ConfigPath.empty()) {
    ConfigPath = GITSBasePath / "Player" / "gits_config.yml";
    if (!std::filesystem::exists(ConfigPath)) {
      ConfigPath = "";
    }
  }
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

  std::filesystem::create_directories(home);
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
    config.DetectBasePaths();
    return config;
  }

  try {
    std::ifstream file(launcherConfigPath);
    if (!file.is_open()) {
      config.DetectBasePaths();
      return config;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    YAML::Node yaml = YAML::Load(content);

    if (yaml["gitsPlayerPath"]) {
      const auto gitsPlayerPath = std::filesystem::path(yaml["gitsPlayerPath"].as<std::string>());
      if (std::filesystem::exists(gitsPlayerPath)) {
        config.GITSPlayerPath = gitsPlayerPath;
      }
    }
    if (yaml["gitsBasePath"]) {
      const auto gitsBasePath = std::filesystem::path(yaml["gitsBasePath"].as<std::string>());
      if (std::filesystem::exists(gitsBasePath)) {
        config.GITSBasePath = gitsBasePath;
      }
    }
    if (yaml["streamPath"]) {
      const auto streamPath = std::filesystem::path(yaml["streamPath"].as<std::string>());
      if (std::filesystem::exists(streamPath)) {
        config.StreamPath = streamPath;
      }
    }
    if (yaml["targetPath"]) {
      const auto targetPath = std::filesystem::path(yaml["targetPath"].as<std::string>());
      if (std::filesystem::exists(targetPath)) {
        config.TargetPath = targetPath;
      }
    }
    if (yaml["configPath"]) {
      const auto configPath = std::filesystem::path(yaml["configPath"].as<std::string>());
      if (std::filesystem::exists(configPath)) {
        config.ConfigPath = configPath;
      }
    }
    if (yaml["captureConfigPath"]) {
      const auto captureConfigPath =
          std::filesystem::path(yaml["captureConfigPath"].as<std::string>());
      if (std::filesystem::exists(captureConfigPath)) {
        config.CaptureConfigPath = captureConfigPath;
      }
    }
    if (yaml["customArguments"]) {
      const auto customArguments = yaml["customArguments"].as<std::string>();
      if (!customArguments.empty()) {
        config.CustomArguments = customArguments;
      }
    }
    if (yaml["captureCustomArguments"]) {
      const auto captureCustomArguments = yaml["captureCustomArguments"].as<std::string>();
      if (!captureCustomArguments.empty()) {
        config.CaptureCustomArguments = captureCustomArguments;
      }
    }
    if (yaml["captureOutputPath"]) {
      const auto captureOutputPath = yaml["captureOutputPath"].as<std::string>();
      if (!captureOutputPath.empty()) {
        config.CaptureOutputPath = captureOutputPath;
      }
    }
    if (yaml["windowSize"]) {
      const auto sizeNode = yaml["windowSize"];
      if (sizeNode.IsSequence() && sizeNode.size() == 2) {
        config.WindowSize = ImVec2(sizeNode[0].as<double>(), sizeNode[1].as<double>());
      }
    }
    if (yaml["windowPos"]) {
      const auto posNode = yaml["windowPos"];
      if (posNode.IsSequence() && posNode.size() == 2) {
        config.WindowPos = ImVec2(posNode[0].as<double>(), posNode[1].as<double>());
      }
    }
    if (yaml["uiScale"]) {
      config.UIScale = yaml["uiScale"].as<float>();
    }
    if (yaml["theme"]) {
      const auto themeID = yaml["theme"].as<std::string>();
      if (!themeID.empty()) {
        config.Theme.SetThemeByID(themeID);
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error loading launcher config: " << e.what() << std::endl;
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
    out << YAML::Key << "gitsPlayerPath" << YAML::Value << GITSPlayerPath.string();
    out << YAML::Key << "gitsBasePath" << YAML::Value << GITSBasePath.string();
    out << YAML::Key << "streamPath" << YAML::Value << StreamPath.string();
    out << YAML::Key << "targetPath" << YAML::Value << TargetPath.string();
    out << YAML::Key << "configPath" << YAML::Value << ConfigPath.string();
    out << YAML::Key << "captureConfigPath" << YAML::Value << CaptureConfigPath.string();
    out << YAML::Key << "customArguments" << YAML::Value << CustomArguments;
    out << YAML::Key << "captureCustomArguments" << YAML::Value << CaptureCustomArguments;
    out << YAML::Key << "captureOutputPath" << YAML::Value << CaptureOutputPath.string();
    out << YAML::Key << "windowSize" << YAML::Value;
    out << YAML::BeginSeq << WindowSize.x << WindowSize.y << YAML::EndSeq;
    out << YAML::Key << "windowPos" << YAML::Value;
    out << YAML::BeginSeq << WindowPos.x << WindowPos.y << YAML::EndSeq;
    out << YAML::Key << "uiScale" << YAML::Value << UIScale;
    out << YAML::Key << "theme" << YAML::Value << Theme.CurThemeID();
    out << YAML::EndMap;

    std::ofstream file(launcherConfigPath);
    if (!file.is_open()) {
      return false;
    }

    file << out.c_str();
    file.close();

    return true;

  } catch (const std::exception& e) {
    std::cerr << "Error saving launcher config: " << e.what() << std::endl;
    return false;
  }
}

} // namespace gits::gui
