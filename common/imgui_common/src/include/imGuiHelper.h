// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs);
ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs);
ImVec2 operator*(const ImVec2& lhs, float rhs);
ImVec2 operator/(const ImVec2& lhs, float rhs);
ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs);
ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs);

namespace gits {

enum class HUDAnchor;

namespace ImGuiHelper {

struct Colors {
  // Notable Blue
  static constexpr ImVec4 NOTICE = ImVec4(0.000f, 0.639f, 0.988f, 1.000f);         // #00A3F6
  static constexpr ImVec4 NOTICE_HOVERED = ImVec4(0.000f, 0.780f, 0.992f, 1.000f); // #00C7FD
  static constexpr ImVec4 NOTICE_ACTIVE = ImVec4(0.000f, 0.404f, 0.710f, 1.000f);  // #0068B5
  // Success Green
  static constexpr ImVec4 SUCCESS = ImVec4(0.384f, 0.808f, 0.345f, 1.000f); // #62CE58
  static constexpr ImVec4 SUCCESS_HOVERED = ImVec4(0.500f, 0.920f, 0.460f, 1.000f);
  static constexpr ImVec4 SUCCESS_ACTIVE = ImVec4(0.300f, 0.700f, 0.270f, 1.000f);
  // Warning Orange
  static constexpr ImVec4 WARNING = ImVec4(0.953f, 0.678f, 0.149f, 1.000f); // #F3AD26
  static constexpr ImVec4 WARNING_HOVERED = ImVec4(1.000f, 0.800f, 0.300f, 1.000f);
  static constexpr ImVec4 WARNING_ACTIVE = ImVec4(0.850f, 0.580f, 0.100f, 1.000f);
  // Critical Orange (between warning and failure)
  static constexpr ImVec4 CRITICAL = ImVec4(0.953f, 0.450f, 0.100f, 1.000f);
  static constexpr ImVec4 CRITICAL_HOVERED = ImVec4(1.000f, 0.560f, 0.200f, 1.000f);
  static constexpr ImVec4 CRITICAL_ACTIVE = ImVec4(0.800f, 0.350f, 0.080f, 1.000f);
  // Failure Red
  static constexpr ImVec4 FAILURE = ImVec4(0.894f, 0.133f, 0.133f, 1.000f); // #E42222
  static constexpr ImVec4 FAILURE_HOVERED = ImVec4(1.000f, 0.280f, 0.280f, 1.000f);
  static constexpr ImVec4 FAILURE_ACTIVE = ImVec4(0.750f, 0.100f, 0.100f, 1.000f);
  // Info Purple
  static constexpr ImVec4 INFO = ImVec4(0.769f, 0.259f, 0.812f, 1.000f); // #C442CF
  static constexpr ImVec4 INFO_HOVERED = ImVec4(0.880f, 0.380f, 0.920f, 1.000f);
  static constexpr ImVec4 INFO_ACTIVE = ImVec4(0.650f, 0.180f, 0.700f, 1.000f);
  // Disabled Grey
  static constexpr ImVec4 DISABLED = ImVec4(0.670f, 0.670f, 0.670f, 1.000f); // #C442CF
  static constexpr ImVec4 DISABLED_HOVERED = ImVec4(0.780f, 0.780f, 0.780f, 1.000f);
  static constexpr ImVec4 DISABLED_ACTIVE = ImVec4(0.680f, 0.680f, 0.670f, 1.000f);
};

enum class Widgets {
  Button,
  Input,
  InputInt,
  Label,
  RadioButton,
  ArrowButton,
  Text,
  Combo,
  Checkbox,
  Unknown
};

enum class ButtonStyle {
  BUTTON_STYLE_SUCCESS,
  BUTTON_STYLE_WARNING,
  BUTTON_STYLE_CRITICAL,
  BUTTON_STYLE_FAILURE,
  BUTTON_STYLE_INFO,
  BUTTON_STYLE_DISABLED,
  BUTTON_STYLE_DEFAULT
};

float WidthOf(Widgets widget, const std::string& label = "");
float LineHeight();

void Seperator(bool samelLine = true);

std::string ToStr(const ImVec2& v);

ImFont* GetIntelFont(float uiScale);

float CurrentUIScale();

bool UpdateUIScaling(float scale);

void AddTooltip(const std::string& message);

void RenderTableContent(const std::vector<std::array<std::string, 2>>& dataTable);

void AddTableRow(const std::string& left, const std::string& right, const int& col2Width);

void PushButtonStyle(const ButtonStyle style);

void PopButtonStyle();

void RenderUIViewWidget(float width);

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

bool LabelInputStringTooltip(const std::string& label,
                             const char* labelID,
                             const char* tooltip,
                             std::string& str,
                             const float widthLabel = 0.0f,
                             const float widthInput = -1.0f,
                             ImGuiInputTextFlags flags = 0);

bool RangeControls(const char* label,
                   float labelWidth,
                   float inputWidth,
                   float rangeFieldInputWidth,
                   const char* rangeId,
                   const char* tooltip,
                   std::string& rangeString,
                   int& tmpStart,
                   int& tmpEnd,
                   int& tmpStep,
                   const char* addRangeLabel,
                   const char* addSingleLabel);

void HelpButton(const std::string& title,
                const std::string& tooltip,
                const std::string& description);

bool ShowReleaseNotesModal(const char* releaseNotes);
}; // namespace ImGuiHelper
}; // namespace gits
