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
#include "configOptions.h"
#include "configOptionsGuiHelpers.h"
#include "configMetadataAuto.h"

#include "nlohmann/json.hpp"

#include "configurationYAMLAuto.h"

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

  changed |= ImGui::Checkbox(Labels::EXECUTABLE_NAME_ENABLED,
                             &config_options::ExecutableNameOverrideEnabled(Mode::PLAYBACK));
  ImGui::SameLine();

  auto widthApplicationName =
      ImGui::GetContentRegionAvail().x -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::EXECUTABLE_NAME_RESET) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::HELP_BUTTON) - 32.0f;
  widthApplicationName *= 0.25f;

  // Common subcapture option (shared with playback)
  ImGui::BeginDisabled(!config_options::ExecutableNameOverrideEnabled(Mode::PLAYBACK));
  changed |= ImGuiHelper::LabelInputString(
      "", "###ExecutableCustomName",
      config_options::ExecutableNameOverrideCustomName(Mode::PLAYBACK), 0, widthApplicationName);
  ImGui::SameLine();
  if (ImGui::Button(Labels::EXECUTABLE_NAME_RESET)) {
    try {
      auto recorderDiags =
          Context::GetInstance().ConfigurationForMode(Mode::PLAYBACK).MetaData.RecorderDiags;
      config_options::ExecutableNameOverrideCustomName(Mode::PLAYBACK) =
          recorderDiags["/original_app/name"_json_pointer].get<std::string>();
      changed = true;
    } catch (...) {
      LOG_ERROR << "Failed to reset executable name override, original application name not found "
                   "in metadata.";
    }
  }
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::Common::Player::ExecutableNameOverride::GroupMetadata);
  ImGui::EndDisabled();

  ImGui::Separator();

  // Dedicated playback options
  ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::WARNING);
  ImGui::Text(Labels::NOTICE_2);
  ImGui::PopStyleColor();

  auto& hudApis = config_options::HudEnabled(Mode::PLAYBACK);
  auto dxHudEnabled = std::find(hudApis.begin(), hudApis.end(), gits::ApiBool::DX) != hudApis.end();
  changed |= ImGui::Checkbox(Labels::HUD_ENABLED, &dxHudEnabled);
  if (changed) {
    auto it = std::find(hudApis.begin(), hudApis.end(), gits::ApiBool::DX);
    if (dxHudEnabled && it == hudApis.end()) {
      hudApis.push_back(gits::ApiBool::DX);
    } else if (!dxHudEnabled && it != hudApis.end()) {
      hudApis.erase(it);
    }
  }
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::Common::Shared::HUD::GroupMetadata);

  changed |=
      ImGui::Checkbox(Labels::SCREENSHOTS, &config_options::ScreenshotsEnabled(Mode::PLAYBACK));
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::Common::Shared::Screenshots::enabled);

  ImGui::BeginDisabled(!config_options::ScreenshotsEnabled(Mode::PLAYBACK));
  ImGui::Indent(indent);
  auto inputPosition = ImGui::GetCursorPosX() + labelWidth - indent;

  changed |= ImGuiHelper::RangeControls(
      Labels::SCREENSHOTS_RANGES, labelWidth - indent, screenshotRangeInputWidth,
      screenshotRangeFieldInputWidth, "###ScreenshotsRANGES", nullptr,
      config_options::ScreenshotsFrames(Mode::PLAYBACK), ScreenshotsConfig.TmpStartFrame,
      ScreenshotsConfig.TmpEndFrame, ScreenshotsConfig.TmpStepFrame, Labels::SCREENSHOTS_ADD_RANGE,
      Labels::SCREENSHOTS_ADD_FRAME);
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::Common::Shared::Screenshots::frames);

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

  changed |= config_options_gui_helpers::Trace(Mode::PLAYBACK, indent);

  if (changed) {
    context.UpdateInMemoryConfig(Mode::PLAYBACK);
  }
}

void PlaybackOptionsPanel::SetScreenshotPathFromInputStream() {
  auto& context = Context::GetInstance();
  auto streamPath = context.GetPathSafe(Path::INPUT_STREAM, Mode::PLAYBACK);
  const auto screenshotPath = streamPath.parent_path() / SCREENSHOTS_SUB_PATH;
  context.SetPath(screenshotPath, Path::SCREENSHOTS, Mode::PLAYBACK);
}

void PlaybackOptionsPanel::ContextCallback(const Event& e) {
  const ContextEvent& contextEvent = static_cast<const ContextEvent&>(e);

  auto& context = Context::GetInstance();

  if (contextEvent.Mode == Mode::PLAYBACK &&
      (contextEvent.EventType == ContextEvent::Type::MetadataLoaded ||
       contextEvent.EventType == ContextEvent::Type::ConfigFileLoaded)) {
    try {
      auto& recorderDiags = context.ConfigurationForMode(Mode::PLAYBACK).MetaData.RecorderDiags;
      config_options::ExecutableNameOverrideCustomName(Mode::PLAYBACK) =
          recorderDiags["/original_app/name"_json_pointer].get<std::string>();
      context.UpdateInMemoryConfig(Mode::PLAYBACK);
    } catch (...) {
      LOG_ERROR << "Failed to set executable name override, original application name not found in "
                   "metadata.";
    }
  }

  if (contextEvent.Mode == Mode::PLAYBACK &&
      contextEvent.EventType == ContextEvent::Type::ConfigFileLoaded) {
    auto screenshotPath = context.GetPathSafe(Path::SCREENSHOTS, Mode::PLAYBACK);
    if (screenshotPath.empty()) {
      SetScreenshotPathFromInputStream();
    }
    auto tracePath = context.GetPathSafe(Path::TRACE, Mode::PLAYBACK);
    if (tracePath.empty()) {
      SetTracePathFromInputStream();
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
