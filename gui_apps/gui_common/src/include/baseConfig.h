// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

#include "imGuiStyle.h"
#include "baseConfig.h"

namespace {
struct Settings {
  static constexpr int WINDOW_POS_X = 300;
  static constexpr int WINDOW_POS_Y = 100;

  static constexpr int WINDOW_SIZE_WIDTH = 1920;
  static constexpr int WINDOW_SIZE_HEIGHT = 1280;
};
} // namespace

namespace gits::gui {

struct BaseConfig {
  const static size_t MAX_RECENT_ENTRIES = 5;

  std::filesystem::path FileLocation;

  ImVec2 WindowSize;
  ImVec2 WindowPos;

  std::string AppName;

  float UIScale;
  gits::ImGuiHelper::Themes Theme;

  BaseConfig(const std::string appName);

  std::filesystem::path GetGITSBaseConfigPath();

  virtual void WriteCustom(YAML::Emitter& out) const {};
  virtual void ReadCustom(const YAML::Node& yaml){};

  static BaseConfig FromFile(const std::string appName, const std::filesystem::path& path = "");

  void FillFromFile(const std::filesystem::path& path = "");
  bool ToFile(const std::filesystem::path& path = "");

  void AddRecentStream(const std::filesystem::path& streamPath);
  const std::vector<std::filesystem::path>& RecentStreams();
  void AddRecentTarget(const std::filesystem::path& targetPath);
  const std::vector<std::filesystem::path>& RecentTargets();

private:
  std::vector<std::filesystem::path> m_RecentStreams;
  std::vector<std::filesystem::path> m_RecentTargets;
};

} // namespace gits::gui
