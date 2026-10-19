// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include "imGuiHelper.h"

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

void Themes::CycleTheme() {
  CurrentThemeIdx = (CurrentThemeIdx + 1) % ThemeIDs.size();
}

void Themes::ApplyTheme() {
  const auto id = CurThemeID();
  if (id == ":dark") {
    ApplyDarkTheme();
  } else {
    ApplyLightTheme();
  }
}

void Themes::ApplyCommonThemeSettings() {
  ImGuiStyle& style = ImGui::GetStyle();

  // -------------------------------------------------------
  // Rounding, Spacing, Sizes & Alpha
  // -------------------------------------------------------
  style.WindowRounding = 4.0f;
  style.FrameRounding = 4.0f;
  style.ScrollbarRounding = 4.0f;
  style.GrabRounding = 4.0f;
  style.TabRounding = 4.0f;
  style.ChildRounding = 4.0f;
  style.PopupRounding = 4.0f;

  style.WindowPadding = ImVec2(10.0f, 10.0f);
  style.FramePadding = ImVec2(6.0f, 4.0f);

  style.ItemSpacing = ImVec2(8.0f, 8.0f);
  style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);

  style.ScrollbarSize = 12.0f;
  style.GrabMinSize = 8.0f;
  style.WindowBorderSize = 1.0f;
  style.ChildBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
  style.TabBorderSize = 1.0f;

  style.DisabledAlpha = 0.6f;
}

void Themes::ApplyDarkTheme() {
  ImGui::StyleColorsDark();

  ApplyCommonThemeSettings();

  ImGuiStyle& style = ImGui::GetStyle();
  // -------------------------------------------------------
  // Colors
  // -------------------------------------------------------
  ImVec4* colors = style.Colors;

  // -- Backgrounds --
  colors[ImGuiCol_WindowBg] = ImVec4(0.071f, 0.106f, 0.176f, 1.000f);  // Shade 3: #141F45
  colors[ImGuiCol_ChildBg] = ImVec4(0.059f, 0.090f, 0.161f, 1.000f);   // Shade 2: #0F1B43
  colors[ImGuiCol_PopupBg] = ImVec4(0.078f, 0.114f, 0.196f, 1.000f);   // Shade 4: #19234A
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.027f, 0.051f, 0.176f, 1.000f); // Shade 1: #070D2D

  // -- Borders --
  colors[ImGuiCol_Border] = ImVec4(0.000f, 0.639f, 0.988f, 0.200f); // Notable Middle: #00A3F6
  colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);

  // -- Frame (inputs, checkboxes, etc.) --
  colors[ImGuiCol_FrameBg] = ImVec4(0.098f, 0.137f, 0.282f, 1.000f);        // Shade 5: #1B2652
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.118f, 0.165f, 0.341f, 1.000f); // Shade 6: #1E2A57
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.000f, 0.639f, 0.988f, 0.400f);  // Notable Middle tinted

  // -- Title --
  colors[ImGuiCol_TitleBg] = ImVec4(0.027f, 0.051f, 0.176f, 1.000f);       // Shade 1: #070D2D
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.000f, 0.404f, 0.710f, 1.000f); // Notable Dark: #0068B5
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.059f, 0.090f, 0.161f, 1.000f); // Shade 2: #0F1B43

  // -- Scrollbar --
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.027f, 0.051f, 0.176f, 1.000f);   // Shade 1: #070D2D
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.000f, 0.404f, 0.710f, 1.000f); // Notable Dark: #0068B5
  colors[ImGuiCol_ScrollbarGrabHovered] =
      ImVec4(0.000f, 0.639f, 0.988f, 1.000f); // Notable Middle: #00A3F6
  colors[ImGuiCol_ScrollbarGrabActive] =
      ImVec4(0.000f, 0.780f, 0.992f, 1.000f); // Notable Light: #00C7FD

  // -- Checkmark & Slider --
  colors[ImGuiCol_CheckMark] = ImVec4(0.000f, 0.780f, 0.992f, 1.000f);  // Notable Light: #00C7FD
  colors[ImGuiCol_SliderGrab] = ImVec4(0.000f, 0.639f, 0.988f, 1.000f); // Notable Middle: #00A3F6
  colors[ImGuiCol_SliderGrabActive] =
      ImVec4(0.000f, 0.780f, 0.992f, 1.000f); // Notable Light: #00C7FD

  // -- Buttons --
  colors[ImGuiCol_ButtonActive] = ImVec4(0.000f, 0.404f, 0.710f, 1.000f); // Notable Dark: #0068B5
  colors[ImGuiCol_ButtonHovered] =
      ImVec4(0.000f, 0.639f, 0.988f, 1.000f); // Notable Middle: #00A3F6
  colors[ImGuiCol_Button] =
      ImVec4(0.000f, 0.220f, 0.450f, 1.000f); // Much darker blue - text stays readable

  // -- Headers (CollapsingHeader, TreeNode, Selectable, MenuItem) --
  colors[ImGuiCol_Header] = ImVec4(0.000f, 0.404f, 0.710f, 0.500f);        // Notable Dark tinted
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.000f, 0.639f, 0.988f, 0.600f); // Notable Middle tinted
  colors[ImGuiCol_HeaderActive] = ImVec4(0.000f, 0.780f, 0.992f, 0.700f);  // Notable Light tinted

  // -- Separator --
  colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.639f, 0.988f, 0.300f); // Notable Middle subtle
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.000f, 0.639f, 0.988f, 0.800f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.000f, 0.780f, 0.992f, 1.000f);

  // -- Resize Grip --
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.000f, 0.404f, 0.710f, 0.400f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.000f, 0.639f, 0.988f, 0.700f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.000f, 0.780f, 0.992f, 1.000f);

  // -- Tabs --
  colors[ImGuiCol_Tab] = ImVec4(0.059f, 0.090f, 0.161f, 1.000f);                // Shade 2: #0F1B43
  colors[ImGuiCol_TabHovered] = ImVec4(0.000f, 0.639f, 0.988f, 0.800f);         // Notable Middle
  colors[ImGuiCol_TabActive] = ImVec4(0.000f, 0.404f, 0.710f, 1.000f);          // Notable Dark
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.027f, 0.051f, 0.176f, 1.000f);       // Shade 1
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.098f, 0.137f, 0.282f, 1.000f); // Shade 5

  // -- Plot --
  colors[ImGuiCol_PlotLines] =
      ImVec4(0.686f, 0.808f, 1.000f, 1.000f); // Less Notable Light: #C9E0FF
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.000f, 0.780f, 0.992f, 1.000f); // Notable Light
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.000f, 0.639f, 0.988f, 1.000f);    // Notable Middle
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.000f, 0.780f, 0.992f, 1.000f);

  // -- Text --
  colors[ImGuiCol_Text] = ImVec4(0.788f, 0.878f, 1.000f, 1.000f); // Less Notable Light: #C9E0FF
  colors[ImGuiCol_TextDisabled] =
      ImVec4(0.686f, 0.761f, 0.851f, 0.600f); // Less Notable Middle: #AFC2D9
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.000f, 0.639f, 0.988f, 0.350f); // Notable Middle tinted

  // -- Drag & Drop --
  colors[ImGuiCol_DragDropTarget] = ImVec4(0.000f, 0.780f, 0.992f, 0.900f);

  // -- Nav --
  colors[ImGuiCol_NavHighlight] = ImVec4(0.000f, 0.780f, 0.992f, 1.000f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.000f, 0.780f, 0.992f, 0.700f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.027f, 0.051f, 0.176f, 0.500f);

  // -- Modal --
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.027f, 0.051f, 0.176f, 0.700f);

  // -- Table --
  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.027f, 0.051f, 0.176f, 1.000f); // Shade 1
  colors[ImGuiCol_TableBorderStrong] = ImVec4(0.000f, 0.404f, 0.710f, 0.500f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.000f, 0.404f, 0.710f, 0.200f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.000f, 0.639f, 0.988f, 0.050f);
}

void Themes::ApplyLightTheme() {
  ImGui::StyleColorsLight();

  ApplyCommonThemeSettings();

  ImGuiStyle& style = ImGui::GetStyle();
  // -------------------------------------------------------
  // Colors
  // -------------------------------------------------------
  ImVec4* colors = style.Colors;

  // -- Backgrounds --
  colors[ImGuiCol_WindowBg] = ImVec4(0.957f, 0.973f, 1.000f, 1.000f);  // Shade 7: #F4F8FF
  colors[ImGuiCol_ChildBg] = ImVec4(0.933f, 0.957f, 1.000f, 1.000f);   // Shade 6: #EEF4FF
  colors[ImGuiCol_PopupBg] = ImVec4(0.957f, 0.973f, 1.000f, 1.000f);   // Shade 7: #F4F8FF
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.843f, 0.910f, 1.000f, 1.000f); // Shade 4: #D7E8FF

  // -- Borders --
  colors[ImGuiCol_Border] = ImVec4(0.000f, 0.404f, 0.710f, 0.200f); // Notable Dark subtle
  colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.030f);

  // -- Frame (inputs, checkboxes, etc.) --
  colors[ImGuiCol_FrameBg] =
      ImVec4(0.863f, 0.922f, 1.000f, 1.000f); // Shade 5: #DCE8FF - light but visible
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.784f, 0.878f, 1.000f, 1.000f); // Shade 4 tinted
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.706f, 0.835f, 1.000f, 1.000f);  // Shade 3: #B4D5FF

  // -- Title --
  colors[ImGuiCol_TitleBg] = ImVec4(0.843f, 0.910f, 1.000f, 1.000f); // Shade 4: #D7E8FF
  colors[ImGuiCol_TitleBgActive] =
      ImVec4(0.784f, 0.878f, 1.000f, 1.000f); // Shade 4 slightly deeper
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.933f, 0.957f, 1.000f, 1.000f); // Shade 6: #EEF4FF

  // -- Scrollbar --
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.933f, 0.957f, 1.000f, 1.000f); // Shade 6
  colors[ImGuiCol_ScrollbarGrab] =
      ImVec4(0.686f, 0.761f, 0.851f, 1.000f); // Less Notable Middle: #AFC2D9
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.000f, 0.639f, 0.988f, 0.600f); // Notable Middle
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.000f, 0.404f, 0.710f, 1.000f);  // Notable Dark

  // -- Checkmark & Slider --
  colors[ImGuiCol_CheckMark] = ImVec4(0.000f, 0.404f, 0.710f, 1.000f);  // Notable Dark: #0068B5
  colors[ImGuiCol_SliderGrab] = ImVec4(0.000f, 0.639f, 0.988f, 0.800f); // Notable Middle
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.000f, 0.404f, 0.710f, 1.000f); // Notable Dark

  // -- Buttons --
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.149f, 0.694f, 0.969f, 1.000f);
  colors[ImGuiCol_Button] = ImVec4(0.000f, 0.639f, 0.965f, 1.000f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.000f, 0.545f, 0.820f, 1.000f);

  // -- Headers (CollapsingHeader, TreeNode, Selectable, MenuItem) --
  colors[ImGuiCol_Header] = ImVec4(0.000f, 0.639f, 0.988f, 0.150f); // Very subtle
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.000f, 0.639f, 0.988f, 0.250f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.000f, 0.639f, 0.988f, 0.350f);

  // -- Separator --
  colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.404f, 0.710f, 0.200f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.000f, 0.404f, 0.710f, 0.400f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.000f, 0.404f, 0.710f, 0.800f);

  // -- Resize Grip --
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.000f, 0.404f, 0.710f, 0.150f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.000f, 0.404f, 0.710f, 0.350f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.000f, 0.404f, 0.710f, 0.700f);

  // -- Tabs --
  colors[ImGuiCol_Tab] = ImVec4(0.843f, 0.910f, 1.000f, 1.000f);        // Shade 4: #D7E8FF - light
  colors[ImGuiCol_TabHovered] = ImVec4(0.706f, 0.835f, 1.000f, 1.000f); // Shade 3: #B4D5FF
  colors[ImGuiCol_TabActive] =
      ImVec4(0.957f, 0.973f, 1.000f, 1.000f); // Shade 7 - active tab matches window bg
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.933f, 0.957f, 1.000f, 1.000f);       // Shade 6
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.843f, 0.910f, 1.000f, 1.000f); // Shade 4

  // -- Plot --
  colors[ImGuiCol_PlotLines] = ImVec4(0.000f, 0.404f, 0.710f, 1.000f);        // Notable Dark
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.000f, 0.639f, 0.988f, 1.000f); // Notable Middle
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.000f, 0.404f, 0.710f, 1.000f);    // Notable Dark
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.000f, 0.639f, 0.988f, 1.000f);

  // -- Text --
  colors[ImGuiCol_Text] =
      ImVec4(0.059f, 0.090f, 0.161f, 1.000f); // Shade 2: #0F1B43 - dark but not pure black
  colors[ImGuiCol_TextDisabled] =
      ImVec4(0.592f, 0.659f, 0.749f, 1.000f); // Less Notable Middle muted
  colors[ImGuiCol_TextSelectedBg] =
      ImVec4(0.000f, 0.639f, 0.988f, 0.200f); // Notable Middle very subtle

  // -- Drag & Drop --
  colors[ImGuiCol_DragDropTarget] = ImVec4(0.000f, 0.404f, 0.710f, 0.700f);

  // -- Nav --
  colors[ImGuiCol_NavHighlight] = ImVec4(0.000f, 0.639f, 0.988f, 0.800f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.000f, 0.639f, 0.988f, 0.600f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.843f, 0.910f, 1.000f, 0.400f);

  // -- Modal --
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.686f, 0.761f, 0.851f, 0.350f);

  // -- Table --
  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.843f, 0.910f, 1.000f, 1.000f); // Shade 4: #D7E8FF
  colors[ImGuiCol_TableBorderStrong] = ImVec4(0.000f, 0.404f, 0.710f, 0.300f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.000f, 0.404f, 0.710f, 0.150f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.000f, 0.404f, 0.710f, 0.040f);
}
} // namespace ImGuiHelper
} // namespace gits
