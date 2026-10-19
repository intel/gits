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
#include <cfloat>

#include "IntelOneMono-RegularAuto.h"
#include "log.h"

ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
  return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
  return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

ImVec2 operator*(const ImVec2& lhs, float rhs) {
  return ImVec2(lhs.x * rhs, lhs.y * rhs);
}

ImVec2 operator/(const ImVec2& lhs, float rhs) {
  return ImVec2(lhs.x / rhs, lhs.y / rhs);
}

ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs) {
  lhs.x += rhs.x;
  lhs.y += rhs.y;
  return lhs;
}

ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs) {
  lhs.x -= rhs.x;
  lhs.y -= rhs.y;
  return lhs;
}

namespace {
static constexpr const char* RELEASE_NOTES_WINDOW_TITLE = "Release Notes";
static constexpr const char* RELEASE_NOTES_BUTTON = "Show Release Notes";
static constexpr const char* RELEASE_NOTES_CLOSE_BUTTON = "Close";
} // namespace

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
  case Widgets::InputInt:
    return WidthOf(Widgets::Button, "+ ") + WidthOf(Widgets::Button, "- ") + padding * 2 +
           WidthOf(Widgets::Input, label);
  case Widgets::Combo:
    // TODO: Can this be handled better?
    // Right now the size is based on a "magic string"
    return ImGui::CalcTextSize("N/A   ").x + style.FramePadding.x * 3 + textWidth;
  case Widgets::Checkbox:
    // TODO: Can this be handled better?
    // Right now the size is based on a "magic checkbox width"
    return textWidth + padding + 48.f;
  default:
    return 0.0f; // Unknown widget type
  }
}

float LineHeight() {
  return ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;
}

void Seperator(bool sameLine) {
  ImGui::TextDisabled(" | ");
  if (sameLine) {
    ImGui::SameLine();
  }
}

std::string ToStr(const ImVec2& v) {
  return std::to_string(static_cast<int>(v.x)) + "x" + std::to_string(static_cast<int>(v.y));
}

ImFont* GetIntelFont(float uiScale) {
  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig font_cfg;
  font_cfg.FontDataOwnedByAtlas = false;
  return io.Fonts->AddFontFromMemoryTTF(gits::Font::font_data, gits::Font::font_data_size, 48.0f,
                                        &font_cfg);
}

float CurrentUIScale() {
  return currentUiScale;
}

bool UpdateUIScaling(float scale) {
  LOG_INFO << "Updating ui scale to: " << scale;

  ImGuiIO& io = ImGui::GetIO();

  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(scale);
  currentUiScale = scale;

  style.AntiAliasedLines = true;
  style.AntiAliasedLinesUseTex = true;

  io.Fonts->Clear();
  ImFont* newFont = GetIntelFont(scale);
  if (newFont == nullptr) {
    return false;
  }
  newFont->Scale = scale * 0.5;
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

void PushButtonStyle(const ButtonStyle style) {
  switch (style) {
  case ButtonStyle::BUTTON_STYLE_SUCCESS:
    ImGui::PushStyleColor(ImGuiCol_Button, ImGuiHelper::Colors::SUCCESS);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiHelper::Colors::SUCCESS_HOVERED);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGuiHelper::Colors::SUCCESS_ACTIVE);
    ImGui::PushStyleColor(
        ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // dark text - green is too bright for white
    break;
  case ButtonStyle::BUTTON_STYLE_WARNING:
    ImGui::PushStyleColor(ImGuiCol_Button, ImGuiHelper::Colors::WARNING);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiHelper::Colors::WARNING_HOVERED);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGuiHelper::Colors::WARNING_ACTIVE);
    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // dark text - yellow is too bright for white
    break;
  case ButtonStyle::BUTTON_STYLE_CRITICAL:
    ImGui::PushStyleColor(ImGuiCol_Button, ImGuiHelper::Colors::CRITICAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiHelper::Colors::CRITICAL_HOVERED);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGuiHelper::Colors::CRITICAL_ACTIVE);
    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // dark text - yellow is too bright for white
    break;
  case ButtonStyle::BUTTON_STYLE_INFO:
    ImGui::PushStyleColor(ImGuiCol_Button, ImGuiHelper::Colors::INFO);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiHelper::Colors::INFO_HOVERED);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGuiHelper::Colors::INFO_ACTIVE);
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // white text - orange is dark enough
    break;
  case ButtonStyle::BUTTON_STYLE_FAILURE:
    ImGui::PushStyleColor(ImGuiCol_Button, ImGuiHelper::Colors::FAILURE);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiHelper::Colors::FAILURE_HOVERED);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGuiHelper::Colors::FAILURE_ACTIVE);
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // white text - red is dark enough
    break;
  case ButtonStyle::BUTTON_STYLE_DEFAULT:
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImGui::GetStyleColorVec4(ImGuiCol_Text)); // keep current theme text color
    break;
  case ButtonStyle::BUTTON_STYLE_DISABLED:
    ImGui::PushStyleColor(ImGuiCol_Button, ImGuiHelper::Colors::DISABLED);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiHelper::Colors::DISABLED_HOVERED);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGuiHelper::Colors::DISABLED_ACTIVE);
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // white text - red is dark enough
    break;
  default:
    return; // no push, so no pop needed
  }
}

void PopButtonStyle() {
  ImGui::PopStyleColor(4);
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
  const auto indent = ImGui::GetCursorPosX();
  if (widthLabel > 0) {
    ImGui::SetNextItemWidth(static_cast<float>(widthLabel));
  }

  ImGui::Text(label.c_str());
  ImGui::SameLine();

  // let the input field take the remaining width
  if (widthLabel > 0) {
    ImGui::SetCursorPosX(indent + widthLabel);
  }
  return InputString(labelID, str, flags, widthInput);
}

bool LabelInputStringTooltip(const std::string& label,
                             const char* labelID,
                             const char* tooltip,
                             std::string& str,
                             const float widthLabel,
                             const float widthInput,
                             ImGuiInputTextFlags flags) {
  const auto indent = ImGui::GetCursorPosX();
  if (widthLabel > 0) {
    ImGui::SetNextItemWidth(static_cast<float>(widthLabel));
  }

  ImGui::Text(label.c_str());
  if (ImGui::IsItemHovered() && tooltip) {
    ImGui::SetTooltip(tooltip);
  }
  ImGui::SameLine();

  // let the input field take the remaining width
  if (widthLabel > 0) {
    ImGui::SetCursorPosX(indent + widthLabel);
  }
  return InputString(labelID, str, flags, widthInput);
}

std::string GetLabelWithRangeID(const char* label, const char* id) {
  return std::string(label) + "###" + id + std::string(label);
};

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
                   const char* addSingleLabel) {
  bool localChanged = false;

  // Main range input string
  localChanged |= ImGuiHelper::LabelInputStringTooltip(label, rangeId, tooltip, rangeString,
                                                       labelWidth, inputWidth);

  // Individual controls for start/end/step - use consistent inputPosition
  auto inputPosition = ImGui::GetCursorPosX() + labelWidth;
  ImGui::SetCursorPosX(inputPosition);

  std::string startLabel = GetLabelWithRangeID("Start", rangeId);
  ImGui::SetNextItemWidth(rangeFieldInputWidth);
  ImGui::InputInt(startLabel.c_str(), &tmpStart, 1, 50);
  ImGui::SameLine();

  std::string endLabel = GetLabelWithRangeID("End", rangeId);
  ImGui::SetNextItemWidth(rangeFieldInputWidth);
  ImGui::InputInt(endLabel.c_str(), &tmpEnd, 1, 50);
  ImGui::SameLine();

  std::string stepLabel = GetLabelWithRangeID("Step", rangeId);
  ImGui::SetNextItemWidth(rangeFieldInputWidth);
  ImGui::InputInt(stepLabel.c_str(), &tmpStep, 1, 10);
  tmpStep = std::max(tmpStep, 1);

  ImGui::SetCursorPosX(inputPosition);
  std::string addRangeBtn = GetLabelWithRangeID(addRangeLabel, rangeId);
  if (ImGui::Button(addRangeBtn.c_str())) {
    // Add range to string (format: "start-end:step")
    std::string newRange;
    if (tmpStep == 1) {
      newRange = std::to_string(tmpStart) + "-" + std::to_string(tmpEnd);
    } else {
      newRange =
          std::to_string(tmpStart) + "-" + std::to_string(tmpEnd) + ":" + std::to_string(tmpStep);
    }

    if (!rangeString.empty()) {
      rangeString += ",";
    }
    rangeString += newRange;
    localChanged = true;
  }
  ImGui::SameLine();

  std::string addSingleBtn = GetLabelWithRangeID(addSingleLabel, rangeId);
  if (ImGui::Button(addSingleBtn.c_str())) {
    // Add single value to string
    std::string newValue = std::to_string(tmpStart);
    if (!rangeString.empty()) {
      rangeString += ",";
    }
    rangeString += newValue;
    localChanged = true;
  }

  return localChanged;
}

void HelpButton(const std::string& title,
                const std::string& tooltip,
                const std::string& description) {
  std::string buttonId = "?##HelpBtn" + title;
  std::string popupId = title + "##HelpPopup";

  float buttonSize = ImGui::GetFrameHeight();
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, buttonSize * 0.5f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                      ImVec2(10.0f, 0.0f)); // Manual tweak for centering

  ImGui::SameLine();
  if (ImGui::Button(buttonId.c_str(), ImVec2(buttonSize, buttonSize))) {
    ImGui::OpenPopup(popupId.c_str());
  }
  ImGui::PopStyleVar(2);

  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tooltip.c_str());
  }

  ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
  float minWidth = viewportSize.x * 0.4f;
  ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, 0), ImVec2(FLT_MAX, FLT_MAX));

  if (ImGui::BeginPopupModal(popupId.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextWrapped("%s", description.c_str());
    if (ImGui::Button("Close")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}
bool ShowReleaseNotesModal(const char* releaseNotes) {
  bool showReleaseNotes = true;
  ImGui::OpenPopup(::RELEASE_NOTES_WINDOW_TITLE);
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImVec2 viewportSize = viewport->Size;

  ImVec2 modalSize = ImVec2(viewportSize.x * 0.8f, viewportSize.y * 0.8f);

  ImVec2 center = viewport->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(modalSize, ImGuiCond_Always);

  if (ImGui::BeginPopupModal(::RELEASE_NOTES_WINDOW_TITLE, &showReleaseNotes,
                             ImGuiWindowFlags_NoResize)) {
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();

    float buttonWidth = 120.0f;
    float buttonHeight = ImGui::GetFrameHeight();
    float padding = ImGui::GetStyle().WindowPadding.y;

    float contentHeight = contentRegion.y - buttonHeight - padding * 2 - 1; // 1 for separator

    if (ImGui::BeginChild("ReleaseNotesContent", ImVec2(0, contentHeight),
                          true, // border
                          ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
      ImGui::TextWrapped("%s", releaseNotes);
    }
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::SetCursorPosX(windowSize.x - buttonWidth - ImGui::GetStyle().WindowPadding.x);
    ImGui::SetCursorPosY(windowSize.y - buttonHeight - ImGui::GetStyle().WindowPadding.y);

    if (ImGui::Button(::RELEASE_NOTES_CLOSE_BUTTON, ImVec2(buttonWidth, 0))) {
      showReleaseNotes = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  return showReleaseNotes;
}

} // namespace ImGuiHelper
} // namespace gits
