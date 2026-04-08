// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playbackOptionsPanel.h"

#include <ranges>
#include <algorithm>

#include "imgui.h"
#include "imGuiHelper.h"
#include "launcherActions.h"
#include "labels.h"
#include "mainWindow.h"
#include "eventBus.h"
#include "contextHelper.h"
#include "nlohmann/json.hpp"

namespace {
static constexpr const char* SCREENSHOTS_SUB_PATH = "gitsScreenshots";

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

PlaybackOptionsPanel::PlaybackOptionsPanel() : BasePanel() {
  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&PlaybackOptionsPanel::ContextCallback, this, std::placeholders::_1));
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&PlaybackOptionsPanel::PathCallback, this, std::placeholders::_1));
}

void PlaybackOptionsPanel::Render() {
  auto& context = Context::GetInstance();
  auto indent = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, "  ");

  auto windowWidth = ImGui::GetContentRegionAvail().x;

  const auto labels = {Labels::SCREENSHOTS_RANGES, Labels::SCREENSHOTS_PATH, Labels::TRACE_PATH};
  auto labelWidth =
      std::ranges::max(labels | std::views::transform([](const auto& label) {
                         return ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Text, label);
                       })) +
      indent;
  auto screenshotRangeInputWidth =
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, "123456789012345678901234567890");
  auto screenshotRangeFieldInputWidth =
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, "12345678901234");

  auto labelSize = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::SCREENSHOTS_PATH);

  auto changed = false;

  changed |= ImGui::Checkbox(Labels::EXECUTABLE_NAME_ENABLED, &ExecutableNameConfig.Enabled);
  ImGui::SameLine();

  auto widthApplicationName =
      ImGui::GetContentRegionAvail().x -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::EXECUTABLE_NAME_RESET) - 16.0f;
  ImGui::BeginDisabled(!ExecutableNameConfig.Enabled);
  changed |= ImGuiHelper::LabelInputString(
      "", "###ExecutableCustomName", ExecutableNameConfig.CustomName, 0, widthApplicationName);
  ImGui::SameLine();
  if (ImGui::Button(Labels::EXECUTABLE_NAME_RESET)) {
    try {
      auto recorderDiags = Context::GetInstance().MetaData.RecorderDiags;
      ExecutableNameConfig.CustomName =
          recorderDiags["/diag/original_app/name"_json_pointer].get<std::string>();
      changed = true;
    } catch (...) {
      // nothing
    }
  }
  ImGui::EndDisabled();

  ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::WARNING);
  ImGui::Text(Labels::NOTICE_2);
  ImGui::PopStyleColor();

  changed |= ImGui::Checkbox(Labels::HUD_ENABLED, &HUDConfig.Enabled);

  changed |= ImGui::Checkbox(Labels::SCREENSHOTS, &ScreenshotsConfig.Enabled);

  ImGui::BeginDisabled(!ScreenshotsConfig.Enabled);
  ImGui::Indent(indent);
  auto inputPosition = ImGui::GetCursorPosX() + labelWidth - indent;

  changed |= ImGuiHelper::RangeControls(
      Labels::SCREENSHOTS_RANGES, labelWidth - indent, screenshotRangeInputWidth,
      screenshotRangeFieldInputWidth, "###ScreenshotsRANGES", "", ScreenshotsConfig.Range,
      ScreenshotsConfig.TmpStartFrame, ScreenshotsConfig.TmpEndFrame,
      ScreenshotsConfig.TmpStepFrame, Labels::SCREENSHOTS_ADD_RANGE, Labels::SCREENSHOTS_ADD_FRAME);

  ImGui::Text(Labels::SCREENSHOTS_PATH);
  ImGui::SameLine();

  ImGui::SetCursorPosX(inputPosition);
  auto screenshotsPathInputWidth =
      ImGui::GetContentRegionAvail().x -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::BASE_ON_STREAMPATH) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_TARGET) - 16.0f;

  context_helper::PathInput("###ScreenshotsPath", Path::SCREENSHOTS, Mode::PLAYBACK, 0,
                            screenshotsPathInputWidth);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::BASE_ON_STREAMPATH)) {
    SetScreenshotPathFromInputStream();
  }
  ImGui::PopID();
  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_TARGET)) {
    ShowFileDialog(FileDialogKey{Path::SCREENSHOTS, Mode::PLAYBACK});
  }
  ImGui::PopID();

  ImGui::Unindent(indent);
  ImGui::EndDisabled();

  changed |= ImGui::Checkbox(Labels::TRACE_EXPORT, &TraceConfig.Enabled);

  ImGui::BeginDisabled(!TraceConfig.Enabled);
  ImGui::Indent(indent);

  labelSize = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::TRACE_PATH);
  ImGui::Text(Labels::TRACE_PATH);
  ImGui::SameLine();

  ImGui::SetCursorPosX(inputPosition);
  auto tracePathInputWidth =
      ImGui::GetContentRegionAvail().x -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::BASE_ON_STREAMPATH) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_TARGET) - 16.0f;

  context_helper::PathInput("###TracePath", Path::TRACE, Mode::PLAYBACK, 0, tracePathInputWidth);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::BASE_ON_STREAMPATH)) {
    SetTracePathFromInputStream();
  }
  ImGui::PopID();
  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_TARGET)) {
    ShowFileDialog(FileDialogKey{Path::TRACE, Mode::PLAYBACK});
  }
  ImGui::PopID();
  ImGui::Unindent(indent);
  ImGui::EndDisabled();

  if (changed) {
    UpdateCLICall();
  }
}

const std::string PlaybackOptionsPanel::GetCLIArguments() const {
  auto& context = Context::GetInstance();
  std::string args = "";
  if (ExecutableNameConfig.Enabled) {
    args += "--Common.Player.ExecutableNameOverride.Enabled ";
    if (!ExecutableNameConfig.CustomName.empty()) {
      args += "--Common.Player.ExecutableNameOverride.CustomName=\"" +
              ExecutableNameConfig.CustomName + "\" ";
    }
  }
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

void PlaybackOptionsPanel::SetScreenshotPathFromInputStream() {
  auto& context = Context::GetInstance();
  auto streamPath = context.GetPathSafe(Path::INPUT_STREAM, Mode::PLAYBACK);
  const auto screenshotPath = streamPath.parent_path() / SCREENSHOTS_SUB_PATH;
  context.SetPath(screenshotPath, Path::SCREENSHOTS, Mode::PLAYBACK);
}

void PlaybackOptionsPanel::SetTracePathFromInputStream() {
  auto& context = Context::GetInstance();
  auto streamPath = context.GetPathSafe(Path::INPUT_STREAM, Mode::PLAYBACK);
  context.SetPath(streamPath.parent_path(), Path::TRACE, Mode::PLAYBACK);
}

void PlaybackOptionsPanel::ContextCallback(const Event& e) {
  const ContextEvent& contextEvent = static_cast<const ContextEvent&>(e);

  if (contextEvent.EventType == ContextEvent::Type::MetadataLoaded) {
    try {
      auto recorderDiags = Context::GetInstance().MetaData.RecorderDiags;
      ExecutableNameConfig.CustomName =
          recorderDiags["/diag/original_app/name"_json_pointer].get<std::string>();
    } catch (...) {
      // nothing
    }
  }
}

void PlaybackOptionsPanel::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);
  if (pathEvent.EventType == PathEvent::Type::INPUT_STREAM) {
    auto screenshotPath = Context::GetInstance().GetPathSafe(Path::SCREENSHOTS, Mode::PLAYBACK);
    if (screenshotPath.empty()) {
      SetScreenshotPathFromInputStream();
    }
    auto tracePath = Context::GetInstance().GetPathSafe(Path::TRACE, Mode::PLAYBACK);
    if (tracePath.empty()) {
      SetTracePathFromInputStream();
    }
  }
}
} // namespace gits::gui
