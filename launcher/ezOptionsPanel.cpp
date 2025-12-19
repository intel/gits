// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
  auto& context = getSharedContext<gui::Context>();
  const auto labels = {Labels::SCREENSHOTS_RANGES, Labels::SUBCAPTURE_OPTIMIZE,
                       Labels::SUBCAPTURE_OPTIMIZE_RAY, Labels::SUBCAPTURE_RANGE};

  auto width = std::ranges::max(labels | std::views::transform([](const auto& label) {
                                  return ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Text, label);
                                }));
  auto indent = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, "  ");
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
  if (ImGuiHelper::InputString("###ScreenshotsPath", context.ScreenshotPath, 0, allocatedWidth)) {
    UpdateCLICall(context);
  }

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_TARGET)) {
    ShowFileDialog(&context, FileDialogKeys::PICK_SCREENSHOTS_PATH);
  }
  ImGui::PopID();

  ImGui::Unindent();
  ImGui::EndDisabled();

  changed |= ImGui::Checkbox(Labels::TRACE_EXPORT, &TraceConfig.Enabled);

  ImGui::BeginDisabled(!TraceConfig.Enabled);
  ImGui::Indent(indent - 12.0f);

  labelSize = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::TRACE_PATH);
  ImGui::Text(Labels::TRACE_PATH);
  ImGui::SameLine();
  allocatedWidth = availableWidth - labelSize -
                   ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_TARGET) -
                   80.0f;

  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + width - indent - labelSize);
  if (ImGuiHelper::InputString("###TracePath", context.TracePath, 0, allocatedWidth)) {
    UpdateCLICall(context);
  }

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_TARGET)) {
    ShowFileDialog(&context, FileDialogKeys::PICK_TRACE_PATH);
  }
  ImGui::PopID();
  ImGui::Unindent();
  ImGui::EndDisabled();

  if (changed) {
    UpdateCLICall(getSharedContext<Context>());
  }
}

const std::string EzOptionsPanel::GetCLIArguments() const {
  auto& context = getSharedContext<gui::Context>();
  std::string args = "";
  if (HUDConfig.Enabled) {
    args += "--hud=DX ";
  }
  if (ScreenshotsConfig.Enabled) {
    args += "--DirectX.Features.Screenshots.Enabled ";
    args += "--DirectX.Features.Screenshots.Frames=\"" + ScreenshotsConfig.Range + "\" ";
    args += "--Common.Player.OutputDir=\"" + context.ScreenshotPath.string() + "\" ";
  }
  if (TraceConfig.Enabled) {
    args += "--DirectX.Features.Trace.Enabled ";
    args += "--Common.Player.OutputTracePath=\"" + context.TracePath.string() + "\" ";
  }
  return args;
}
} // namespace gits::gui
