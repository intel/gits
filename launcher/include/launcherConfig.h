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
#include "launcherPaths.h"

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

  LauncherPaths Paths;

  std::filesystem::path FileLocation;

  ImVec2 WindowSize;
  ImVec2 WindowPos;

  float UIScale;
  gits::ImGuiHelper::Themes Theme;

  LauncherConfig();

  static std::filesystem::path GetGITSLauncherConfigPath();

  static LauncherConfig FromFile(const std::filesystem::path& path = "");
  bool ToFile(const std::filesystem::path& path = "");
};
} // namespace gits::gui
