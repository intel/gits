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
#include "launcherCommon.h"

namespace gits::gui {

struct LauncherConfig : public BaseConfig {
  static constexpr const char* MODE_KEY = "mode";

  Mode LastMode = Mode::SIMPLE;

  LauncherConfig() : BaseConfig("gitsLauncher"){};

  void WriteCustom(YAML::Emitter& out) const {
    out << YAML::Key << MODE_KEY << YAML::Value << static_cast<int>(LastMode);
  };

  void ReadCustom(const YAML::Node& yaml) {
    if (!yaml[MODE_KEY]) {
      return;
    }

    const auto storedValue = yaml[MODE_KEY].as<int>();
    LastMode = storedValue == static_cast<int>(Mode::ADVANCED) ? Mode::ADVANCED : Mode::SIMPLE;
  }
};

} // namespace gits::gui
