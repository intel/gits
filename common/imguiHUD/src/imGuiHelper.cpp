// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imGuiHelper.h"

#include <algorithm>

#include "IntelOneMono-RegularAuto.h"
#include "enumsAuto.h"

namespace gits {
namespace ImGuiHelper {
std::string ToStr(const ImVec2& v) {
  return std::to_string(static_cast<int>(v.x)) + "x" + std::to_string(static_cast<int>(v.y));
}

ImFont* GetIntelFont(float uiScale) {
  ImGuiIO& io = ImGui::GetIO();
  return io.Fonts->AddFontFromMemoryTTF(gits::Font::font_data, gits::Font::font_data_size,
                                        13.0f * uiScale);
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

  style.ScaleAllSizes(scale);

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

void PositionWindow(gits::HUDAnchor anchor, const ImVec2& windowSize, const ImVec2& padding) {
  const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
  float x = 0;
  if (anchor == gits::HUDAnchor::TOP_LEFT || anchor == gits::HUDAnchor::CENTER_LEFT ||
      anchor == gits::HUDAnchor::BOTTOM_LEFT) {
    x = mainViewport->WorkPos.x + padding.x;
  } else if (anchor == gits::HUDAnchor::TOP_CENTER || anchor == gits::HUDAnchor::CENTER ||
             anchor == gits::HUDAnchor::BOTTOM_CENTER) {
    x = mainViewport->WorkPos.x + (mainViewport->Size.x - windowSize.x) / 2.0f;
  } else { // <X>_RIGHT
    x = mainViewport->WorkPos.x + mainViewport->Size.x - windowSize.x - padding.x;
  }

  float y = 0;
  if (anchor == gits::HUDAnchor::BOTTOM_LEFT || anchor == gits::HUDAnchor::BOTTOM_CENTER ||
      anchor == gits::HUDAnchor::BOTTOM_RIGHT) {
    y = mainViewport->WorkPos.y + mainViewport->Size.y - windowSize.y - padding.y;
  } else if (anchor == gits::HUDAnchor::CENTER_LEFT || anchor == gits::HUDAnchor::CENTER ||
             anchor == gits::HUDAnchor::CENTER_RIGHT) {
    y = mainViewport->WorkPos.y + (mainViewport->Size.y - windowSize.y) / 2.0f;
  } else { // TOP_<X>
    y = mainViewport->WorkPos.y + padding.y;
  }
  ImGui::SetWindowPos(ImVec2(x, y), ImGuiCond_Always);
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

} // namespace ImGuiHelper
} // namespace gits
