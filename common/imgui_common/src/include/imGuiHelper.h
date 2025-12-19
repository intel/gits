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

struct Colors {
  static constexpr const ImVec4 SUCCESS = ImVec4(0.1f, 0.7f, 0.2f, 1.0f);
  static constexpr const ImVec4 SUCCESS_HOVERED = ImVec4(0.25f, 0.9f, 0.35f, 1.0f);
  static constexpr const ImVec4 SUCCESS_ACTIVE = ImVec4(0.15f, 0.7f, 0.25f, 1.0f);
  static constexpr const ImVec4 WARNING = ImVec4(0.8f, 0.7f, 0.1f, 1.0f);
  static constexpr const ImVec4 WARNING_HOVERED = ImVec4(1.0f, 0.9f, 0.25f, 1.0f);
  static constexpr const ImVec4 WARNING_ACTIVE = ImVec4(0.7f, 0.6f, 0.15f, 1.0f);
  static constexpr const ImVec4 FAILURE = ImVec4(0.7f, 0.1f, 0.2f, 1.0f);
  static constexpr const ImVec4 FAILURE_HOVERED = ImVec4(0.9f, 0.25f, 0.35f, 1.0f);
  static constexpr const ImVec4 FAILURE_ACTIVE = ImVec4(0.7f, 0.15f, 0.25f, 1.0f);
};

enum class Widgets {
  Button,
  Input,
  Label,
  RadioButton,
  ArrowButton,
  Text,
  Combo,
  Unknown
};

float WidthOf(Widgets widget, const std::string& label = "");
float LineHeight();

std::string ToStr(const ImVec2& v);

ImFont* GetIntelFont(float uiScale);

float CurrentUIScale();

bool UpdateUIScaling(float scale);

void AddTooltip(const std::string& message);

void RenderTableContent(const std::vector<std::array<std::string, 2>>& dataTable);

void AddTableRow(const std::string& left, const std::string& right, const int& col2Width);

bool InputString(const char* label,
                 std::string& str,
                 ImGuiInputTextFlags flags = 0,
                 const float widthInput = -1.0f);

bool InputString(const char* labelID,
                 std::filesystem::path& path,
                 ImGuiInputTextFlags flags = 0,
                 const float widthInput = -1.0f);

bool LabelInputString(const std::string& label,
                      const char* labelID,
                      std::string& str,
                      const float widthLabel = 0.0f,
                      const float widthInput = -1.0f,
                      ImGuiInputTextFlags flags = 0);

}; // namespace ImGuiHelper
}; // namespace gits
