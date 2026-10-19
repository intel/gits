// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureOptionsPanel.h"

#include "imgui.h"
#include "launcherActions.h"
#include "labels.h"
#include "baseEventBus.h"
#include "configOptions.h"
#include "configMetadataAuto.h"
#include "configOptionsGuiHelpers.h"
#include "contextHelper.h"
#include "guiActions.h"
#include "mainWindow.h"

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
  changed |=
      ImGui::Checkbox(Labels::EXECUTABLE_NAME_ENABLED,
                      &config_options::ExecutableNameOverrideEnabled(GitsAction::SUBCAPTURE));
  ImGui::SameLine();

  auto widthApplicationName =
      ImGui::GetContentRegionAvail().x -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::EXECUTABLE_NAME_RESET) -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::HELP_BUTTON) - 32.0f;
  widthApplicationName *= 0.25f;
  ImGui::BeginDisabled(!config_options::ExecutableNameOverrideEnabled(GitsAction::SUBCAPTURE));
  changed |= ImGuiHelper::LabelInputString(
      "", "###ExecutableCustomName",
      config_options::ExecutableNameOverrideCustomName(GitsAction::SUBCAPTURE), 0,
      widthApplicationName);
  ImGui::SameLine();
  if (ImGui::Button(Labels::EXECUTABLE_NAME_RESET)) {
    try {
      auto recorderDiags =
          context.ConfigurationForAction(GitsAction::SUBCAPTURE).MetaData.RecorderDiags;
      static const nlohmann::json::json_pointer originalAppNamePtr("/original_app/name");
      if (recorderDiags.contains(originalAppNamePtr)) {
        config_options::ExecutableNameOverrideCustomName(GitsAction::SUBCAPTURE) =
            recorderDiags.at(originalAppNamePtr).get<std::string>();
        changed = true;
      } else {
        LOG_ERROR << "Failed to reset executable name from metadata.";
      }
    } catch (...) {
      LOG_ERROR << "Failed to reset executable name from metadata.";
    }
  }

  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::Common::Player::ExecutableNameOverride::GroupMetadata);
  ImGui::EndDisabled();

  auto api = context.GetStreamAPI();

  if (api == Api::DIRECTX || api == Api::VULKAN) {
    const auto hudApi = api == Api::DIRECTX ? gits::ApiBool::DX : gits::ApiBool::VK;
    auto& hudApis = config_options::HudEnabled(GitsAction::SUBCAPTURE);
    auto hudEnabled = std::find(hudApis.begin(), hudApis.end(), hudApi) != hudApis.end();
    if (ImGui::Checkbox(Labels::HUD_ENABLED, &hudEnabled)) {
      changed = true;
      auto it = std::find(hudApis.begin(), hudApis.end(), hudApi);
      if (hudEnabled && it == hudApis.end()) {
        hudApis.push_back(hudApi);
      } else if (!hudEnabled && it != hudApis.end()) {
        hudApis.erase(it);
      }
    }
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::Common::Shared::HUD::GroupMetadata);
  }

  ImGui::Separator();

  // Dedicated subcapture options
  const float widthLabel = ImGui::CalcTextSize(Labels::SUBCAPTURE_START_FRAME).x * 2.0f;
  if (api == Api::DIRECTX) {
    RowSubcapturePath();

    changed |= ImGui::Checkbox(Labels::SUBCAPTURE_OPTIMIZE, &config_options::SubcaptureOptimize());
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::Common::Player::Subcapture::optimize);

    changed |= ImGui::Checkbox(Labels::SUBCAPTURE_EXECUTION_SERIALIZATION,
                               &config_options::SubcaptureExecutionSerialization());
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::Common::Player::Subcapture::DirectX::executionSerialization);

    // GitsAction Selector
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
    modeChanged |= ImGui::RadioButton(Labels::COMMAND_LIST_SUBCAPTURE,
                                      reinterpret_cast<int*>(&selectedMode), 1);
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

    // GitsAction-specific Content
    if (selectedMode == SubcaptureMode::Subcapture) {
      ImGui::SetNextItemWidth(widthLabel / 4.0f);
      changed |=
          ImGui::InputInt(Labels::SUBCAPTURE_START_FRAME, &SubcaptureConfig.StartFrame, 1, 10);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(widthLabel / 4.0f);
      changed |= ImGui::InputInt(Labels::SUBCAPTURE_END_FRAME, &SubcaptureConfig.EndFrame, 1, 10);
      config_options_gui_helpers::ConfigOptionHelpButton(
          ConfigMetadata::Common::Player::Subcapture::frames);
    }

    if (selectedMode == SubcaptureMode::CommandListSubcapture) {
      const bool isSerialized =
          context.ConfigurationForAction(GitsAction::SUBCAPTURE).MetaData.IsASerializedSubcapture;
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
          ConfigMetadata::Common::Player::Subcapture::DirectX::commandListExecutions);
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
      context.UpdateInMemoryConfig(GitsAction::SUBCAPTURE);
      changed = false;
    }
  }

  if (api == Api::VULKAN) {
    RowSubcapturePath();

    changed |= ImGui::Checkbox(Labels::SUBCAPTURE_OPTIMIZE, &config_options::SubcaptureOptimize());
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::Common::Player::Subcapture::optimize);

    ImGui::Separator();

    ImGui::SetNextItemWidth(widthLabel / 4.0f);
    changed |= ImGui::InputInt(Labels::SUBCAPTURE_START_FRAME, &SubcaptureConfig.StartFrame, 1, 10);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(widthLabel / 4.0f);
    changed |= ImGui::InputInt(Labels::SUBCAPTURE_END_FRAME, &SubcaptureConfig.EndFrame, 1, 10);
    config_options_gui_helpers::ConfigOptionHelpButton(
        ConfigMetadata::Common::Player::Subcapture::frames);

    DroppedFilePath.reset();

    if (changed) {
      config_options::SubcaptureFrames() = SubcaptureConfig.FramesRange();
      context.UpdateInMemoryConfig(GitsAction::SUBCAPTURE);
      changed = false;
    }
  }
}

void SubcaptureOptionsPanel::RenderSimple() {
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  RowSubcapturePath();

  ImGui::Spacing();
  ImGui::Text(Labels::SUBCAPTURE_RANGE);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  const float widthLabel = ImGui::CalcTextSize(Labels::SUBCAPTURE_START_FRAME).x * 2.0f;
  bool changed = false;

  ImGui::SetNextItemWidth(widthLabel / 4.0f);
  changed |= ImGui::InputInt(Labels::SUBCAPTURE_START_FRAME, &SubcaptureConfig.StartFrame, 1, 10);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(widthLabel / 4.0f);
  changed |= ImGui::InputInt(Labels::SUBCAPTURE_END_FRAME, &SubcaptureConfig.EndFrame, 1, 10);
  config_options_gui_helpers::ConfigOptionHelpButton(
      ConfigMetadata::Common::Player::Subcapture::frames);

  if (SubcaptureConfig.StartFrame < 1) {
    SubcaptureConfig.StartFrame = 1;
  }
  if (SubcaptureConfig.EndFrame < SubcaptureConfig.StartFrame) {
    SubcaptureConfig.EndFrame = SubcaptureConfig.StartFrame;
  }

  if (changed) {
    config_options::SubcaptureFrames(GitsAction::SUBCAPTURE) = SubcaptureConfig.FramesRange();
    context.UpdateInMemoryConfig(GitsAction::SUBCAPTURE);
  }
}

void SubcaptureOptionsPanel::RowSubcapturePath() {
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Text(Labels::SUBCAPTURE_PATH);

  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto buttonWidth = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_TARGET);
  auto allocatedWidth = availableWidth - ImGui::GetCursorPosX() - buttonWidth;

  context_helper::PathInput("###SubcapturePath", Path::OUTPUT_STREAM, GitsAction::SUBCAPTURE, 0,
                            allocatedWidth);

  if (DroppedFilePath.has_value() &&
      ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
    context.SetPath(DroppedFilePath.value(), Path::OUTPUT_STREAM, GitsAction::SUBCAPTURE);
    DroppedFilePath.reset();
  }

  ImGui::SameLine();
  if (ImGui::Button(Labels::CHOOSE_TARGET, ImVec2(buttonWidth, 0))) {
    gui_actions::ShowFileDialog(FileDialogKey{Path::OUTPUT_STREAM, GitsAction::SUBCAPTURE});
  }
}

void SubcaptureOptionsPanel::ContextCallback(const Event& e) {
  const ContextEvent& contextEvent = static_cast<const ContextEvent&>(e);

  auto& context = Context::GetInstance();

  if (contextEvent.Mode == GitsAction::SUBCAPTURE &&
      (contextEvent.EventType == ContextEvent::Type::MetadataLoaded ||
       contextEvent.EventType == ContextEvent::Type::ConfigFileLoaded)) {
    try {
      const auto& recorderDiags =
          context.ConfigurationForAction(GitsAction::SUBCAPTURE).MetaData.RecorderDiags;
      static const nlohmann::json::json_pointer originalAppNamePtr("/original_app/name");
      if (recorderDiags.contains(originalAppNamePtr)) {
        config_options::ExecutableNameOverrideCustomName(GitsAction::SUBCAPTURE) =
            recorderDiags.at(originalAppNamePtr).get<std::string>();
        context.UpdateInMemoryConfig(GitsAction::SUBCAPTURE);
      } else {
        LOG_ERROR << "Failed to load executable name from metadata.";
      }
    } catch (...) {
      LOG_ERROR << "Failed to load executable name from metadata.";
    }
  }

  auto& configurationForMode = context.ConfigurationForAction(GitsAction::SUBCAPTURE);

  if (contextEvent.EventType == ContextEvent::Type::InMemoryConfigurationChanged) {
    configurationForMode.ConfigurationDirty = true;
  }
}
void SubcaptureOptionsPanel::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);
  auto& baseContext = BaseContext::GetBaseInstance();

  if (!pathEvent.Mode.has_value() || pathEvent.Mode.value() != GitsAction::SUBCAPTURE) {
    return;
  }

  if (pathEvent.EventType == PathEvent::Type::CONFIG) {
    gui_actions::LoadConfigFile(GitsAction::SUBCAPTURE);
  }

  if (pathEvent.EventType == PathEvent::Type::INPUT_STREAM) {
    auto streamPath = baseContext.GetPathSafe(Path::OUTPUT_STREAM, GitsAction::SUBCAPTURE);
    if (streamPath.empty()) {
      const auto& parentPath = baseContext.GetPathSafe(Path::INPUT_STREAM, GitsAction::SUBCAPTURE)
                                   .parent_path()
                                   .parent_path();
      baseContext.SetPath(parentPath, Path::OUTPUT_STREAM, GitsAction::SUBCAPTURE);
    }
  }
}

void SubcaptureOptionsPanel::FileDropCallback(const Event& e) {
  auto& baseContext = BaseContext::GetBaseInstance();
  const FileDropEvent& fileDropEvent = static_cast<const FileDropEvent&>(e);
  if (baseContext.SelectedAction == GitsAction::SUBCAPTURE) {
    DroppedFilePath = fileDropEvent.FilePath;
  }
}

} // namespace gits::gui
