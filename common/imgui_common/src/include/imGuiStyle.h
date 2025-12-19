// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "imgui.h"
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace gits {

enum class HUDAnchor;

namespace ImGuiHelper {

class Themes {
public:
  std::vector<const char*> ThemeLabels = {"Dark", "Light"};
  std::vector<std::string> ThemeIDs = {":dark", ":light"};

  bool SetThemeFromFile(const std::filesystem::path filePath);
  bool SaveThemeToFile(const std::filesystem::path filePath);

public:
  size_t CurrentThemeIdx = 0;

  const std::string CurThemeID() const;
  const std::string CurThemeLabel() const;
  bool SetThemeByIdx(size_t idx);
  bool SetThemeByLabel(std::string label);
  bool SetThemeByID(std::string id);

  void ApplyTheme();
};
}; // namespace ImGuiHelper
}; // namespace gits
