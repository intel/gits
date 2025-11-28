// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imGuiHelper.h"

#include <algorithm>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>

#include "IntelOneMono-RegularAuto.h"
#include "log.h"

namespace gits {
namespace ImGuiHelper {

static float currentUiScale = 1.0f;

bool SetStyleFromFile(const std::filesystem::path filePath) {
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
    PLOG_WARNING << "Error loading style from file '" << filePath.string() << "': " << e.what();
    return false;
  }
}

bool SaveStyleToFile(const std::filesystem::path filePath) {
  const auto& style = ImGui::GetStyle();

  PLOG_DEBUG << "SaveStyleToFile received path: '" << filePath.string();
  PLOG_DEBUG << "Path is empty: " << (filePath.empty() ? "YES" : "NO");
  PLOG_DEBUG << "Has filename: " << (filePath.has_filename() ? "YES" : "NO");
  PLOG_DEBUG << "Filename: '" << filePath.filename().string();
  PLOG_DEBUG << "Parent path: '" << filePath.parent_path().string();

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
      PLOG_WARNING << "Failed to open file: " << filePath;
      return false;
    }

    // Use YAML::Emitter to write the node
    YAML::Emitter emitter;
    emitter << root;

    // Write to file
    file << emitter.c_str();

    return true;
  } catch (const std::exception& e) {
    PLOG_WARNING << "Error saving YAML to " << filePath.string() << ": " << e.what();
    return false;
  }
}

float WidthOf(Widgets widget, const std::string& label) {
  const auto& style = ImGui::GetStyle();
  auto frameHeight = ImGui::GetFrameHeight();
  auto textWidth = ImGui::CalcTextSize(label.c_str()).x;
  auto padding = style.FramePadding.x * 2;
  switch (widget) {
  case Widgets::Button:
    return textWidth + padding;
  case Widgets::Input:
  case Widgets::Label:
    return textWidth + padding;
  case Widgets::RadioButton:
    return textWidth + frameHeight + style.FramePadding.x * 3;
  case Widgets::ArrowButton:
    return WidthOf(Widgets::Button, "-.-");
  case Widgets::Text:
    return textWidth + padding;
  default:
    return 0.0f; // Unknown widget type
  }
}

float LineHeight() {
  return ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;
}

std::string ToStr(const ImVec2& v) {
  return std::to_string(static_cast<int>(v.x)) + "x" + std::to_string(static_cast<int>(v.y));
}

ImFont* GetIntelFont(float uiScale) {
  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig font_cfg;
  font_cfg.FontDataOwnedByAtlas = false;
  return io.Fonts->AddFontFromMemoryTTF(gits::Font::font_data, gits::Font::font_data_size,
                                        13.0f * uiScale, &font_cfg);
}

bool UpdateUIScaling(float scale) {
  ImGuiIO& io = ImGui::GetIO();

  ImGuiStyle& style = ImGui::GetStyle();
  ImGuiStyle styleold = style;

  style = ImGuiStyle();
  style.WindowBorderSize = 1.0f;
  style.ChildBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
  style.TabBorderSize = 1.0f;
  style.WindowRounding = 0.0f;
  style.ChildRounding = 2.0f;
  style.PopupRounding = 2.0f;
  style.FrameRounding = 2.0f;
  style.ScrollbarRounding = 2.0f;
  style.GrabRounding = 2.0f;
  style.TabRounding = 2.0f;
  style.ItemSpacing.y = 8.0f;

  style.ScaleAllSizes(scale);
  currentUiScale = scale;

  std::copy(std::begin(styleold.Colors), std::end(styleold.Colors), std::begin(style.Colors));

  style.AntiAliasedLines = true;
  style.AntiAliasedLinesUseTex = true;

  ImFont* newFont = GetIntelFont(scale);
  if (newFont == nullptr) {
    return false;
  }

  io.FontDefault = newFont;
  io.Fonts->Build();

  return true;
}

void AddTooltip(const std::string& message) {
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::Text(message.c_str());
    ImGui::EndTooltip();
  }
}

void RenderTableContent(const std::vector<std::array<std::string, 2>>& dataTable) {
  int maxCol2Width = 0;
  for (const auto& row : dataTable) {
    int width = ImGui::CalcTextSize(row[1].c_str()).x;
    maxCol2Width = std::max(maxCol2Width, width);
  }

  for (const auto& row : dataTable) {
    AddTableRow(row[0], row[1], maxCol2Width);
  }
}

void AddTableRow(const std::string& left, const std::string& right, const int& col2Width) {
  ImGui::TableNextRow();

  ImGui::TableSetColumnIndex(0);
  ImGui::Text(left.c_str());

  ImGui::TableSetColumnIndex(1);
  ImVec2 textSize = ImGui::CalcTextSize(right.c_str());
  float cursorPosX = ImGui::GetCursorPosX() + col2Width - textSize.x;
  ImGui::SetCursorPosX(cursorPosX);
  ImGui::Text(right.c_str());
}

bool InputString(const char* labelID,
                 std::string& str,
                 ImGuiInputTextFlags flags,
                 const float widthInput) {
  if (widthInput > 0) {
    ImGui::SetNextItemWidth(widthInput);
  }
  str.resize(1024);

  bool changed = ImGui::InputText(labelID, str.data(), str.size(), flags);

  str.resize(strlen(str.c_str()));

  return changed;
}

bool InputString(const char* labelID,
                 std::filesystem::path& path,
                 ImGuiInputTextFlags flags,
                 const float widthInput) {
  std::string str = path.string();

  const auto changed = InputString(labelID, str, flags, widthInput);
  if (changed) {
    path = std::filesystem::path(str);
  }
  return changed;
}

bool LabelInputString(const std::string& label,
                      const char* labelID,
                      std::string& str,
                      const float widthLabel,
                      const float widthInput,
                      ImGuiInputTextFlags flags) {
  if (widthLabel > 0) {
    ImGui::SetNextItemWidth(static_cast<float>(widthLabel));
  }

  ImGui::Text(label.c_str());
  ImGui::SameLine();

  // let the input field take the remaining width
  if (widthLabel > 0) {
    ImGui::SetCursorPosX(widthLabel);
  }
  return InputString(labelID, str, flags, widthInput);
}

} // namespace ImGuiHelper
} // namespace gits
