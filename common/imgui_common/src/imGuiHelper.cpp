// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

float WidthOf(Widgets widget, const std::string& label) {
  // remove everything after '###' from label to account for ImGui ID tags
  std::string cleanLabel = label;
  size_t idTagPos = cleanLabel.find("###");
  if (idTagPos != std::string::npos) {
    cleanLabel = cleanLabel.substr(0, idTagPos);
  }
  const auto& style = ImGui::GetStyle();
  auto frameHeight = ImGui::GetFrameHeight();
  auto textWidth = ImGui::CalcTextSize(cleanLabel.c_str()).x;
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
  case Widgets::Combo:
    // TODO: Can this be handled better?
    // Right now the size is based on a "magic string"
    return ImGui::CalcTextSize("N/A   ").x + style.FramePadding.x * 3 + textWidth;
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
  return io.Fonts->AddFontFromMemoryTTF(gits::Font::font_data, gits::Font::font_data_size, 24.0f,
                                        &font_cfg);
}

float CurrentUIScale() {
  return currentUiScale;
}

bool UpdateUIScaling(float scale) {
  LOG_INFO << "Updating ui scale to: " << scale;

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

  io.Fonts->Clear();
  ImFont* newFont = GetIntelFont(scale);
  if (newFont == nullptr) {
    return false;
  }
  newFont->Scale = scale;
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
