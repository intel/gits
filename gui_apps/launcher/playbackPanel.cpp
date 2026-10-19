// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playbackPanel.h"

#include "imGuiHelper.h"

#include "resource.h"
#include "context.h"
#include "launcherActions.h"
#include "fileActions.h"
#include "labels.h"
#include "baseEventBus.h"
#include "guiActions.h"
#include "configOptions.h"
#include "configOptionsGuiHelpers.h"
#include "configMetadataAuto.h"

#include "mainWindow.h"
#include "contextHelper.h"

#include "configurationYAMLAuto.h"

namespace {
using namespace gits::gui;

float WidthLastButton() {
  auto labels = {Labels::CHOOSE_STREAM, Labels::CLEAR_ARGUMENTS, Labels::CHOOSE_CONFIG};

  float result = 0;
  for (const auto& label : labels) {
    auto width = gits::ImGuiHelper::WidthOf(gits::ImGuiHelper::Widgets::Button, label);
    if (width > result) {
      result = width;
    }
  }
  return result;
}

} // namespace

namespace gits::gui {
typedef gits::gui::Context::SideBarItem SideBarItem;

PlaybackPanel::PlaybackPanel() : BasePanel() {
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&PlaybackPanel::PathCallback, this, std::placeholders::_1));

  EventBus::GetInstance().subscribe<FileDropEvent>(
      std::bind(&PlaybackPanel::FileDropCallback, this, std::placeholders::_1));

  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&PlaybackPanel::ContextCallback, this, std::placeholders::_1));
}

void PlaybackPanel::Render() {
  RowStreamPath();
  RowConfigPath();
  RowArguments();
  DroppedFilePath.reset();
}

void PlaybackPanel::RenderSimple() {
  RowStreamPath();
}

void PlaybackPanel::RowStreamPath() {
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Text(Labels::STREAM);

  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto allocatedWidth = availableWidth - ImGui::GetCursorPosX() - WidthLastButton();

  context_helper::PathInput("###InputPath", Path::INPUT_STREAM, context.SelectedAction, 0,
                            allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::STREAM_INPUT_HINT);

  if (DroppedFilePath.has_value() &&
      ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
    context.SetPath(DroppedFilePath.value(), Path::INPUT_STREAM, context.SelectedAction);
    DroppedFilePath.reset();
  }

  ImGui::SameLine();
  if (ImGui::Button(Labels::CHOOSE_STREAM, ImVec2(WidthLastButton(), 0))) {
    gui_actions::ShowFileDialog(FileDialogKey{Path::INPUT_STREAM, context.SelectedAction});
  }
  ImGuiHelper::AddTooltip(Labels::CHOOSE_STREAM_HINT);

  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);
  auto apiText = "Stream API: " + Labels::ApiToString(context.GetStreamAPI());
  ImGui::Text(apiText.c_str());
}

void PlaybackPanel::RowArguments() {
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CUSTOM_ARGS);
  ImGui::SameLine();
  ImGuiHelper::HelpButton(Labels::CUSTOM_ARGS_PLAYBACK_HELP_TITLE,
                          Labels::CUSTOM_ARGS_PLAYBACK_HELP_HINT,
                          Labels::CUSTOM_ARGS_PLAYBACK_HELP_TEXT);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX() - WidthLastButton();
  if (ImGuiHelper::InputString("###CustomArgumentsInput", context.CustomArguments, 0,
                               remainingWidth)) {
    // nothing
  }
  ImGuiHelper::AddTooltip(Labels::CUSTOM_ARGS_PLAYBACK_HINT);

  ImGui::SameLine();
  if (ImGui::Button(Labels::CLEAR_ARGUMENTS, ImVec2(WidthLastButton(), 0))) {
    context.CustomArguments.clear();
  }
  ImGuiHelper::AddTooltip(Labels::CLEAR_ARGUMENTS_HINT);
}

void PlaybackPanel::RowConfigPath() {
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CONFIG);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX();
  auto allocatedWidth = remainingWidth - WidthLastButton();

  context_helper::PathInput("###ConfigPathInput", gui::Path::CONFIG, context.SelectedAction, 0,
                            allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::CONFIG_INPUT_HINT);

  if (DroppedFilePath.has_value() &&
      ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
    context.SetPath(DroppedFilePath.value(), Path::CONFIG, context.SelectedAction);
    DroppedFilePath.reset();
  }

  ImGui::SameLine();
  if (ImGui::Button(Labels::CHOOSE_CONFIG, ImVec2(WidthLastButton(), 0))) {
    gui_actions::ShowFileDialog(FileDialogKey{Path::CONFIG, context.SelectedAction});
  }
  ImGuiHelper::AddTooltip(Labels::CHOOSE_CONFIG_HINT);
}

void PlaybackPanel::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);

  if (!pathEvent.Mode.has_value() || pathEvent.Mode.value() != GitsAction::PLAYBACK) {
    return;
  }

  if (pathEvent.EventType == PathEvent::Type::CONFIG) {
    gui_actions::LoadConfigFile(GitsAction::PLAYBACK);
  }
}

void PlaybackPanel::FileDropCallback(const Event& e) {
  auto& baseContext = BaseContext::GetBaseInstance();
  const FileDropEvent& fileDropEvent = static_cast<const FileDropEvent&>(e);
  auto gitsAction = baseContext.SelectedAction;
  if (gitsAction == GitsAction::PLAYBACK || gitsAction == GitsAction::SUBCAPTURE) {
    DroppedFilePath = fileDropEvent.FilePath;
  }
}

void PlaybackPanel::ContextCallback(const Event& e) {
  const ContextEvent& contextEvent = static_cast<const ContextEvent&>(e);

  auto& context = Context::GetInstance();
  auto& configurationForMode = context.ConfigurationForAction(GitsAction::PLAYBACK);

  if (contextEvent.EventType == ContextEvent::Type::InMemoryConfigurationChanged) {
    configurationForMode.ConfigurationDirty = true;
  }
}

} // namespace gits::gui
