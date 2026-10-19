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

namespace gits::gui {

struct StudioConfig : public BaseConfig {
  StudioConfig();
};
} // namespace gits::gui
