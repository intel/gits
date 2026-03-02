// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "ezOptionsPanel.h"

#include <ranges>
#include <algorithm>

#include "imgui.h"
#include "imGuiHelper.h"
#include "launcherActions.h"
#include "labels.h"
#include "mainWindow.h"
#include "contextHelper.h"

namespace {
std::optional<std::filesystem::path> GetPath(bool flag, std::string path) {
  if (flag) {
    if (!path.empty()) {
      if (std::filesystem::exists(path)) {
        return path;
      }
    }
  }
  return std::nullopt;
}
} // namespace

namespace gits::gui {

void EzOptionsPanel::Render() {
  auto& context = Context::GetInstance();
  const auto labels = {Labels::SCREENSHOTS_RANGES, Labels::SCREENSHOTS_PATH, Labels::TRACE_PATH};
  auto indent = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, "  ");

  auto width = std::ranges::max(labels | std::views::transform([](const auto& label) {
                                  return ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Text, label);
                                })) +
               indent;
  auto widthLabel =
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, "123456789012345678901234567890");
  widthLabel = std::min(widthLabel, ImGui::GetContentRegionAvail().x - indent - width);

  ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::WARNING);
  ImGui::Text(Labels::NOTICE_2);
  ImGui::PopStyleColor();

  auto changed = false;
  changed |= ImGui::Checkbox(Labels::HUD_ENABLED, &HUDConfig.Enabled);

  changed |= ImGui::Checkbox(Labels::SCREENSHOTS, &ScreenshotsConfig.Enabled);
  ImGui::BeginDisabled(!ScreenshotsConfig.Enabled);
  ImGui::Indent(indent);
  changed |= ImGuiHelper::LabelInputString(Labels::SCREENSHOTS_RANGES, "###ScreenshotsRANGES",
                                           ScreenshotsConfig.Range, width, widthLabel);
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + width - indent);
  ImGui::SetNextItemWidth(widthLabel / 2.0f);
  changed |=
      ImGui::InputInt(Labels::SCREENSHOTS_START_FRAME, &ScreenshotsConfig.TmpStartFrame, 1, 50);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(widthLabel / 2.0f);
  changed |= ImGui::InputInt(Labels::SCREENSHOTS_END_FRAME, &ScreenshotsConfig.TmpEndFrame, 1, 50);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(widthLabel / 2.0f);
  changed |= ImGui::InputInt(Labels::SUBCAPTURE_STEP_FRAME, &ScreenshotsConfig.TmpStepFrame, 1, 10);
  ScreenshotsConfig.TmpStepFrame = std::max(ScreenshotsConfig.TmpStepFrame, 1);

  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + width - indent);
  if (ImGui::Button(Labels::SCREENSHOTS_ADD_RANGE)) {
    changed = ScreenshotsConfig.AddTmpRange();
  }
  ImGui::SameLine();
  if (ImGui::Button(Labels::SCREENSHOTS_ADD_FRAME)) {
    changed = ScreenshotsConfig.AddTmpFrame();
  }

  auto labelSize = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::SCREENSHOTS_PATH);
  ImGui::Text(Labels::SCREENSHOTS_PATH);
  ImGui::SameLine();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  auto allocatedWidth = availableWidth - labelSize -
                        ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_TARGET) -
                        20.0f;

  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + width - indent - labelSize);
  context_helper::PathInput("###ScreenshotsPath", Path::SCREENSHOTS, Mode::PLAYBACK, 0,
                            allocatedWidth);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_TARGET)) {
    ShowFileDialog(FileDialogKeys{Path::SCREENSHOTS, Mode::PLAYBACK});
  }
  ImGui::PopID();

  ImGui::Unindent();
  ImGui::EndDisabled();

  changed |= ImGui::Checkbox(Labels::TRACE_EXPORT, &TraceConfig.Enabled);

  ImGui::BeginDisabled(!TraceConfig.Enabled);
  ImGui::Indent(indent);

  labelSize = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::TRACE_PATH);
  ImGui::Text(Labels::TRACE_PATH);
  ImGui::SameLine();
  allocatedWidth = availableWidth - labelSize -
                   ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_TARGET);

  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + width - indent - labelSize);
  context_helper::PathInput("###TracePath", Path::TRACE, Mode::PLAYBACK, 0, allocatedWidth);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_TARGET)) {
    ShowFileDialog(FileDialogKeys{Path::TRACE, Mode::PLAYBACK});
  }
  ImGui::PopID();
  ImGui::Unindent();
  ImGui::EndDisabled();

  if (changed) {
    UpdateCLICall();
  }
}

const std::string EzOptionsPanel::GetCLIArguments() const {
  auto& context = Context::GetInstance();
  std::string args = "";
  if (HUDConfig.Enabled) {
    args += "--hud=DX ";
  }
  if (ScreenshotsConfig.Enabled) {
    auto screenshotPath = Context::GetInstance().GetPath(Path::SCREENSHOTS, Mode::PLAYBACK);
    if (screenshotPath.has_value()) {
      args += "--DirectX.Features.Screenshots.Enabled ";
      args += "--DirectX.Features.Screenshots.Frames=\"" + ScreenshotsConfig.Range + "\" ";
      args += "--Common.Player.OutputDir=\"" + screenshotPath.value().string() + "\" ";
    }
  }
  if (TraceConfig.Enabled) {
    auto tracePath = Context::GetInstance().GetPath(Path::TRACE, Mode::PLAYBACK);
    if (tracePath.has_value()) {
      args += "--DirectX.Features.Trace.Enabled ";
      args += "--Common.Player.OutputTracePath=\"" + tracePath.value().string() + "\" ";
    }
  }
  return args;
}
} // namespace gits::gui
