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

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#include "imGuiStyle.h"

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
  std::filesystem::path GITSBasePath;
  std::filesystem::path StreamPath;
  std::filesystem::path TargetPath;
  std::filesystem::path ConfigPath;
  std::filesystem::path CaptureConfigPath;
  std::string CustomArguments;
  std::string CaptureCustomArguments;
  std::filesystem::path CaptureOutputPath;
  std::filesystem::path FileLocation;

  ImVec2 WindowSize;
  ImVec2 WindowPos;

  float UIScale;
  gits::ImGuiHelper::Themes Theme;

  LauncherConfig();

  void DetectBasePaths();
  static std::filesystem::path GetGITSLauncherConfigPath();

  static LauncherConfig FromFile(const std::filesystem::path& path = "");
  bool ToFile(const std::filesystem::path& path = "");
};
} // namespace gits::gui
