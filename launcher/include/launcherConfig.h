// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <yaml-cpp/yaml.h>

#include <imgui.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace {
struct Settings {
  static constexpr int WINDOW_POS_X = 300;
  static constexpr int WINDOW_POS_Y = 100;

  static constexpr int WINDOW_SIZE_WIDTH = 1920;
  static constexpr int WINDOW_SIZE_HEIGHT = 1280;
};
} // namespace

namespace gits::gui {

struct LauncherConfig {
  std::filesystem::path GITSPlayerPath;
  std::filesystem::path StreamPath;
  std::filesystem::path ConfigPath;
  std::string CustomArguments;
  std::filesystem::path FileLocation;

  ImVec2 WindowSize;
  ImVec2 WindowPos;

  LauncherConfig() {
    WindowPos = ImVec2(::Settings::WINDOW_POS_X, ::Settings::WINDOW_POS_Y);
    WindowSize = ImVec2(::Settings::WINDOW_SIZE_WIDTH, ::Settings::WINDOW_SIZE_HEIGHT);

    StreamPath = "";

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
    auto basePath = std::filesystem::path(exeDir.string()).parent_path();
    GITSPlayerPath = basePath / "Player" / gitsPlayer;
    if (!std::filesystem::exists(GITSPlayerPath)) {
      GITSPlayerPath = "";
    }

    ConfigPath = basePath / "Player" / "gits_config.yml";
    if (!std::filesystem::exists(ConfigPath)) {
      ConfigPath = "";
    }

    CustomArguments = "";
    FileLocation = GetGITSLauncherConfigPath();
  }

  LauncherConfig& operator=(const LauncherConfig& other) {
    if (this != &other) {
      GITSPlayerPath = other.GITSPlayerPath;
      StreamPath = other.StreamPath;
      ConfigPath = other.ConfigPath;
      CustomArguments = other.CustomArguments;
      FileLocation = other.FileLocation;
    }
    return *this;
  }

  static std::filesystem::path GetGITSLauncherConfigPath() {
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
      home = std::string(homeDir) / ".config" / appName;
    }
#endif

    std::filesystem::create_directories(home);
    return home / "gitsLauncherConfig.yaml";
  }

  static LauncherConfig FromFile(const std::filesystem::path& path = "") {
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

      if (yaml["gitsPlayerPath"]) {
        const auto gitsPlayerPath = std::filesystem::path(yaml["gitsPlayerPath"].as<std::string>());
        if (std::filesystem::exists(gitsPlayerPath)) {
          config.GITSPlayerPath = gitsPlayerPath;
        }
      }
      if (yaml["streamPath"]) {
        const auto streamPath = std::filesystem::path(yaml["streamPath"].as<std::string>());
        if (std::filesystem::exists(streamPath)) {
          config.StreamPath = streamPath;
        }
      }
      if (yaml["configPath"]) {
        const auto configPath = std::filesystem::path(yaml["configPath"].as<std::string>());
        if (std::filesystem::exists(configPath)) {
          config.ConfigPath = configPath;
        }
      }
      if (yaml["customArguments"]) {
        const auto customArguments = yaml["customArguments"].as<std::string>();
        if (!customArguments.empty()) {
          config.CustomArguments = customArguments;
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

    } catch (const std::exception& e) {
      std::cerr << "Error loading launcher config: " << e.what() << std::endl;
    }

    return config;
  }

  bool ToFile(const std::filesystem::path& path = "") {
    if (!path.empty()) {
      FileLocation = path;
    }
    const auto& launcherConfigPath = FileLocation;

    try {
      std::filesystem::create_directories(launcherConfigPath.parent_path());

      YAML::Emitter out;
      out << YAML::BeginMap;
      out << YAML::Key << "gitsPlayerPath" << YAML::Value << GITSPlayerPath.string();
      out << YAML::Key << "streamPath" << YAML::Value << StreamPath.string();
      out << YAML::Key << "configPath" << YAML::Value << ConfigPath.string();
      out << YAML::Key << "customArguments" << YAML::Value << CustomArguments;
      out << YAML::Key << "windowSize" << YAML::Value;
      out << YAML::BeginSeq << WindowSize.x << WindowSize.y << YAML::EndSeq;
      out << YAML::Key << "windowPos" << YAML::Value;
      out << YAML::BeginSeq << WindowPos.x << WindowPos.y << YAML::EndSeq;
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
};
} // namespace gits::gui
