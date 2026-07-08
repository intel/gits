// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "SubcaptureOptionsPanel.h"

#include "imgui.h"
#include "launcherActions.h"
#include "labels.h"
#include "eventBus.h"
#include "configOptions.h"
#include "configMetadataAuto.h"
#include "configOptionsGuiHelpers.h"
#include "contextHelper.h"

#include "nlohmann/json.hpp"

namespace gits::gui {

SubcaptureOptionsPanel::SubcaptureOptionsPanel() : BasePanel() {
  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&SubcaptureOptionsPanel::ContextCallback, this, std::placeholders::_1));
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&SubcaptureOptionsPanel::PathCallback, this, std::placeholders::_1));
  EventBus::GetInstance().subscribe<FileDropEvent>(
      std::bind(&SubcaptureOptionsPanel::FileDropCallback, this, std::placeholders::_1));
}

void SubcaptureOptionsPanel::Render() {
  auto& context = Context::GetInstance();
  static auto changed = true;

  // Common subcapture option (shared with playback)
  changed |= ImGui::Checkbox(Labels::EXECUTABLE_NAME_ENABLED,
                             &config_options::ExecutableNameOverrideEnabled(Mode::SUBCAPTURE));
  ImGui::SameLine();

  auto widthApplicationName =
      ImGui::GetContentRegionAvail().x -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::EXECUTABLE_NAME_RESET) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::HELP_BUTTON) - 32.0f;
  widthApplicationName *= 0.25f;
  ImGui::BeginDisabled(!config_options::ExecutableNameOverrideEnabled(Mode::SUBCAPTURE));
  changed |= ImGuiHelper::LabelInputString(
      "", "###ExecutableCustomName",
      config_options::ExecutableNameOverrideCustomName(Mode::SUBCAPTURE), 0, widthApplicationName);
  ImGui::SameLine();
  if (ImGui::Button(Labels::EXECUTABLE_NAME_RESET)) {
    try {
      auto recorderDiags =
          Context::GetInstance().ConfigurationForMode(Mode::SUBCAPTURE).MetaData.RecorderDiags;
      config_options::ExecutableNameOverrideCustomName(Mode::SUBCAPTURE) =
          recorderDiags["/original_app/name"_json_pointer].get<std::string>();
      changed = true;
    } catch (...) {
      LOG_ERROR << "Failed to reset executable name from metadata.";
    }
  }

  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::Common::Player::ExecutableNameOverride::GroupMetadata);
  ImGui::EndDisabled();

  ImGui::Separator();

  // Dedicated subcapture options
  const float widthLabel = ImGui::CalcTextSize(Labels::SUBCAPTURE_START_FRAME).x * 2.0f;
  ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::WARNING);
  ImGui::Text(Labels::NOTICE_2);
  ImGui::PopStyleColor();

  RowSubcapturePath();

  changed |= ImGui::Checkbox(Labels::SUBCAPTURE_OPTIMIZE, &config_options::SubcaptureOptimize());
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::DirectX::Features::Subcapture::optimize);

  changed |= ImGui::Checkbox(Labels::SUBCAPTURE_EXECUTION_SERIALIZATION,
                             &config_options::SubcaptureExecutionSerialization());
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::DirectX::Features::Subcapture::executionSerialization);

  // Mode Selector
  ImGui::Separator();

  enum class SubcaptureMode {
    Subcapture = 0,
    CommandListSubcapture = 1,
    CommandSubcapture = 2
  };
  static SubcaptureMode selectedMode = SubcaptureMode::Subcapture;

  auto modeChanged = false;
  modeChanged |= ImGui::RadioButton(Labels::SUBCAPTURE, reinterpret_cast<int*>(&selectedMode), 0);
  ImGui::SameLine();
  modeChanged |=
      ImGui::RadioButton(Labels::COMMAND_LIST_SUBCAPTURE, reinterpret_cast<int*>(&selectedMode), 1);
  //ImGui::SameLine();
  //modeChanged |= ImGui::RadioButton("Command Subcapture", reinterpret_cast<int*>(&selectedMode), 2);

  if (modeChanged) {
    if (selectedMode == SubcaptureMode::Subcapture) {
      config_options::CommandListExecutions() = "";
    }
    if (selectedMode == SubcaptureMode::CommandSubcapture) {
      // Not yet supported
    }
    changed = true;
  }

  ImGui::Separator();

  // Mode-specific Content
  if (selectedMode == SubcaptureMode::Subcapture) {
    ImGui::SetNextItemWidth(widthLabel / 4.0f);
    changed |= ImGui::InputInt(Labels::SUBCAPTURE_START_FRAME, &SubcaptureConfig.StartFrame, 1, 10);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(widthLabel / 4.0f);
    changed |= ImGui::InputInt(Labels::SUBCAPTURE_END_FRAME, &SubcaptureConfig.EndFrame, 1, 10);
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::Subcapture::frames);
  }

  if (selectedMode == SubcaptureMode::CommandListSubcapture) {
    const bool isSerialized =
        context.ConfigurationForMode(Mode::SUBCAPTURE).MetaData.IsASerializedSubcapture;
    if (!isSerialized) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::WARNING);
      ImGui::Text(Labels::COMMAND_LIST_SUBCAPTURE_NOTICE);
      ImGui::PopStyleColor();
    }
    ImGui::BeginDisabled(!isSerialized);
    ImGui::SetNextItemWidth(widthLabel / 4.0f);
    changed |= ImGui::InputInt(Labels::COMMAND_LIST_SUBCAPTURE_FRAME,
                               &SubcaptureConfig.CommandListSubcaptureFrame, 1, 10);
    ImGuiHelper::HelpButton(Labels::COMMAND_LIST_SUBCAPTURE_FRAME_HINT,
                            Labels::COMMAND_LIST_SUBCAPTURE_FRAME_HINT,
                            Labels::COMMAND_LIST_SUBCAPTURE_FRAME_HELP);
    ImGui::SetNextItemWidth(widthLabel / 4.0f);
    changed |= ImGui::InputInt(Labels::COMMAND_LIST_SUBCAPTURE_EXECUTIONS_START,
                               &SubcaptureConfig.CommandListExecutionsStart, 1, 10);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(widthLabel / 4.0f);
    changed |= ImGui::InputInt(Labels::COMMAND_LIST_SUBCAPTURE_EXECUTIONS_END,
                               &SubcaptureConfig.CommandListExecutionsEnd, 1, 10);
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::DirectX::Features::Subcapture::commandListExecutions);
    ImGui::EndDisabled();
  }

  if (selectedMode == SubcaptureMode::CommandSubcapture) {
    ImGui::Text("Not yet supported");
  }

  DroppedFilePath.reset();

  if (changed) {
    if (selectedMode == SubcaptureMode::Subcapture) {
      config_options::SubcaptureFrames() = SubcaptureConfig.FramesRange();
    }
    if (selectedMode == SubcaptureMode::CommandListSubcapture) {
      config_options::SubcaptureFrames() =
          std::to_string(SubcaptureConfig.CommandListSubcaptureFrame);
      config_options::CommandListExecutions() = SubcaptureConfig.CommandListExecutionsRange();
    }
    if (selectedMode == SubcaptureMode::CommandSubcapture) {
      // Not yet supported
    }
    context.UpdateInMemoryConfig(Mode::SUBCAPTURE);
    changed = false;
  }
}

void SubcaptureOptionsPanel::RowSubcapturePath() {
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Text(Labels::SUBCAPTURE_PATH);

  ImGui::SameLine();

  // TODO: Fix the sizing of this path input (right now it's based on a magic number)
  auto allocatedWidth =
      availableWidth - ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::SUBCAPTURE_PATH) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_TARGET) - 64.0f;

  context_helper::PathInput("###SubcapturePath", Path::OUTPUT_STREAM, Mode::SUBCAPTURE, 0,
                            allocatedWidth);

  if (DroppedFilePath.has_value() &&
      ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
    context.SetPath(DroppedFilePath.value(), Path::OUTPUT_STREAM, Mode::SUBCAPTURE);
    DroppedFilePath.reset();
  }

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_TARGET)) {
    ShowFileDialog(FileDialogKey{Path::OUTPUT_STREAM, Mode::SUBCAPTURE});
  }
  ImGui::PopID();
}

void SubcaptureOptionsPanel::ContextCallback(const Event& e) {
  const ContextEvent& contextEvent = static_cast<const ContextEvent&>(e);

  auto& context = Context::GetInstance();

  if (contextEvent.Mode == Mode::SUBCAPTURE &&
      (contextEvent.EventType == ContextEvent::Type::MetadataLoaded ||
       contextEvent.EventType == ContextEvent::Type::ConfigFileLoaded)) {
    try {
      auto& recorderDiags = context.ConfigurationForMode(Mode::SUBCAPTURE).MetaData.RecorderDiags;
      config_options::ExecutableNameOverrideCustomName(Mode::SUBCAPTURE) =
          recorderDiags["/original_app/name"_json_pointer].get<std::string>();
      context.UpdateInMemoryConfig(Mode::SUBCAPTURE);
    } catch (...) {
      LOG_ERROR << "Failed to load executable name from metadata.";
    }
  }

  auto& configurationForMode = context.ConfigurationForMode(Mode::SUBCAPTURE);

  if (contextEvent.EventType == ContextEvent::Type::InMemoryConfigurationChanged) {
    configurationForMode.ConfigurationDirty = true;
  }
}
void SubcaptureOptionsPanel::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);

  if (!pathEvent.Mode.has_value() || pathEvent.Mode.value() != Mode::SUBCAPTURE) {
    return;
  }

  if (pathEvent.EventType == PathEvent::Type::CONFIG) {
    LoadConfigFile(Mode::SUBCAPTURE);
  }

  if (pathEvent.EventType == PathEvent::Type::INPUT_STREAM) {
    auto streamPath = Context::GetInstance().GetPathSafe(Path::OUTPUT_STREAM, Mode::SUBCAPTURE);
    if (streamPath.empty()) {
      const auto& parentPath = Context::GetInstance()
                                   .GetPathSafe(Path::INPUT_STREAM, Mode::SUBCAPTURE)
                                   .parent_path()
                                   .parent_path();
      Context::GetInstance().SetPath(parentPath, Path::OUTPUT_STREAM, Mode::SUBCAPTURE);
    }
  }
}

void SubcaptureOptionsPanel::FileDropCallback(const Event& e) {
  const FileDropEvent& fileDropEvent = static_cast<const FileDropEvent&>(e);
  if (Context::GetInstance().AppMode == Mode::SUBCAPTURE) {
    DroppedFilePath = fileDropEvent.FilePath;
  }
}

} // namespace gits::gui
