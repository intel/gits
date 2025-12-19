// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imGuiStyle.h"

#include <algorithm>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>

#include "log.h"

namespace gits {
namespace ImGuiHelper {

bool Themes::SetThemeFromFile(const std::filesystem::path filePath) {
  ImGuiStyle& style = ImGui::GetStyle();

  try {
    YAML::Node root = YAML::LoadFile(filePath.string());

    // Scalar style properties
    if (root["Alpha"]) {
      style.Alpha = root["Alpha"].as<float>();
    }
    if (root["WindowPadding"]) {
      auto v = root["WindowPadding"].as<std::vector<float>>();
      if (v.size() == 2) {
        style.WindowPadding = ImVec2(v[0], v[1]);
      }
    }
    if (root["WindowRounding"]) {
      style.WindowRounding = root["WindowRounding"].as<float>();
    }
    if (root["WindowBorderSize"]) {
      style.WindowBorderSize = root["WindowBorderSize"].as<float>();
    }
    if (root["WindowMinSize"]) {
      auto v = root["WindowMinSize"].as<std::vector<float>>();
      if (v.size() == 2) {
        style.WindowMinSize = ImVec2(v[0], v[1]);
      }
    }
    if (root["WindowTitleAlign"]) {
      auto v = root["WindowTitleAlign"].as<std::vector<float>>();
      if (v.size() == 2) {
        style.WindowTitleAlign = ImVec2(v[0], v[1]);
      }
    }
    if (root["ChildRounding"]) {
      style.ChildRounding = root["ChildRounding"].as<float>();
    }
    if (root["ChildBorderSize"]) {
      style.ChildBorderSize = root["ChildBorderSize"].as<float>();
    }
    if (root["PopupRounding"]) {
      style.PopupRounding = root["PopupRounding"].as<float>();
    }
    if (root["PopupBorderSize"]) {
      style.PopupBorderSize = root["PopupBorderSize"].as<float>();
    }
    if (root["FramePadding"]) {
      auto v = root["FramePadding"].as<std::vector<float>>();
      if (v.size() == 2) {
        style.FramePadding = ImVec2(v[0], v[1]);
      }
    }
    if (root["FrameRounding"]) {
      style.FrameRounding = root["FrameRounding"].as<float>();
    }
    if (root["FrameBorderSize"]) {
      style.FrameBorderSize = root["FrameBorderSize"].as<float>();
    }
    if (root["ItemSpacing"]) {
      auto v = root["ItemSpacing"].as<std::vector<float>>();
      if (v.size() == 2) {
        style.ItemSpacing = ImVec2(v[0], v[1]);
      }
    }
    if (root["ItemInnerSpacing"]) {
      auto v = root["ItemInnerSpacing"].as<std::vector<float>>();
      if (v.size() == 2) {
        style.ItemInnerSpacing = ImVec2(v[0], v[1]);
      }
    }
    if (root["IndentSpacing"]) {
      style.IndentSpacing = root["IndentSpacing"].as<float>();
    }
    if (root["ScrollbarSize"]) {
      style.ScrollbarSize = root["ScrollbarSize"].as<float>();
    }
    if (root["ScrollbarRounding"]) {
      style.ScrollbarRounding = root["ScrollbarRounding"].as<float>();
    }
    if (root["GrabMinSize"]) {
      style.GrabMinSize = root["GrabMinSize"].as<float>();
    }
    if (root["GrabRounding"]) {
      style.GrabRounding = root["GrabRounding"].as<float>();
    }
    if (root["TabRounding"]) {
      style.TabRounding = root["TabRounding"].as<float>();
    }
    if (root["TabBorderSize"]) {
      style.TabBorderSize = root["TabBorderSize"].as<float>();
    }

    // Colors
    if (root["Colors"]) {
      YAML::Node colorsNode = root["Colors"];
      for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        char colorName[64];
        snprintf(colorName, sizeof(colorName), "Color_%d", i);
        if (colorsNode[colorName]) {
          auto v = colorsNode[colorName].as<std::vector<float>>();
          if (v.size() == 4) {
            style.Colors[i] = ImVec4(v[0], v[1], v[2], v[3]);
          }
        }
      }
    }

    return true;
  } catch (const std::exception& e) {
    LOG_WARNING << "Error loading style from file '" << filePath.string() << "': " << e.what();
    return false;
  }
}

bool Themes::SaveThemeToFile(const std::filesystem::path filePath) {
  const auto& style = ImGui::GetStyle();

  LOG_DEBUG << "SaveStyleToFile received path: '" << filePath.string();
  LOG_DEBUG << "Path is empty: " << (filePath.empty() ? "YES" : "NO");
  LOG_DEBUG << "Has filename: " << (filePath.has_filename() ? "YES" : "NO");
  LOG_DEBUG << "Filename: '" << filePath.filename().string();
  LOG_DEBUG << "Parent path: '" << filePath.parent_path().string();

  YAML::Node root;

  // Scalar style properties
  root["Alpha"] = style.Alpha;
  root["WindowPadding"] = std::vector<float>{style.WindowPadding.x, style.WindowPadding.y};
  root["WindowRounding"] = style.WindowRounding;
  root["WindowBorderSize"] = style.WindowBorderSize;
  root["WindowMinSize"] = std::vector<float>{style.WindowMinSize.x, style.WindowMinSize.y};
  root["WindowTitleAlign"] = std::vector<float>{style.WindowTitleAlign.x, style.WindowTitleAlign.y};
  root["ChildRounding"] = style.ChildRounding;
  root["ChildBorderSize"] = style.ChildBorderSize;
  root["PopupRounding"] = style.PopupRounding;
  root["PopupBorderSize"] = style.PopupBorderSize;
  root["FramePadding"] = std::vector<float>{style.FramePadding.x, style.FramePadding.y};
  root["FrameRounding"] = style.FrameRounding;
  root["FrameBorderSize"] = style.FrameBorderSize;
  root["ItemSpacing"] = std::vector<float>{style.ItemSpacing.x, style.ItemSpacing.y};
  root["ItemInnerSpacing"] = std::vector<float>{style.ItemInnerSpacing.x, style.ItemInnerSpacing.y};
  root["IndentSpacing"] = style.IndentSpacing;
  root["ScrollbarSize"] = style.ScrollbarSize;
  root["ScrollbarRounding"] = style.ScrollbarRounding;
  root["GrabMinSize"] = style.GrabMinSize;
  root["GrabRounding"] = style.GrabRounding;
  root["TabRounding"] = style.TabRounding;
  root["TabBorderSize"] = style.TabBorderSize;

  // Colors
  YAML::Node colorsNode;
  for (int i = 0; i < ImGuiCol_COUNT; ++i) {
    const ImVec4& col = style.Colors[i];
    char colorName[64];
    snprintf(colorName, sizeof(colorName), "Color_%d", i);
    colorsNode[colorName] = std::vector<float>{col.x, col.y, col.z, col.w};
  }
  root["Colors"] = colorsNode;

  try {
    // Create parent directories if they don't exist
    std::filesystem::create_directories(filePath.parent_path());

    // Open file for writing
    std::ofstream file(filePath);
    if (!file.is_open()) {
      LOG_WARNING << "Failed to open file: " << filePath;
      return false;
    }

    // Use YAML::Emitter to write the node
    YAML::Emitter emitter;
    emitter << root;

    // Write to file
    file << emitter.c_str();

    return true;
  } catch (const std::exception& e) {
    LOG_WARNING << "Error saving YAML to " << filePath.string() << ": " << e.what();
    return false;
  }
}

const std::string Themes::CurThemeID() const {
  if (CurrentThemeIdx < ThemeIDs.size()) {
    return ThemeIDs[CurrentThemeIdx];
  }
  return std::string();
}

const std::string Themes::CurThemeLabel() const {
  if (CurrentThemeIdx < ThemeLabels.size()) {
    return std::string(ThemeLabels[CurrentThemeIdx]);
  }
  return std::string();
}

bool Themes::SetThemeByIdx(size_t idx) {
  if (idx < ThemeIDs.size() && idx < ThemeLabels.size()) {
    CurrentThemeIdx = idx;
    return true;
  }
  return false;
}

bool Themes::SetThemeByLabel(std::string label) {
  for (size_t i = 0; i < ThemeLabels.size(); ++i) {
    if (label == ThemeLabels[i]) {
      CurrentThemeIdx = i;
      return true;
    }
  }
  return false;
}

bool Themes::SetThemeByID(std::string id) {
  for (size_t i = 0; i < ThemeIDs.size(); ++i) {
    if (id == ThemeIDs[i]) {
      CurrentThemeIdx = i;
      return true;
    }
  }
  return false;
}

void Themes::ApplyTheme() {
  const auto id = CurThemeID();
  if (id == ":dark") {
    ImGui::StyleColorsDark();
  } else {
    ImGui::StyleColorsLight();
  }
}
} // namespace ImGuiHelper
} // namespace gits
