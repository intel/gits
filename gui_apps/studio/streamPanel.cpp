// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "streamPanel.h"

#include "imGuiHelper.h"

#include "resource.h"
#include "studioContext.h"
#include "studioActions.h"
#include "fileActions.h"
#include "studioLabels.h"
#include "baseEventBus.h"
#include "guiActions.h"

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
StreamPanel::StreamPanel() : BasePanel() {
  m_NavBar = NavBar(false);

  EventBus::GetInstance().subscribe<ActionEvent>(
      std::bind(&StreamPanel::ActionCallback, this, std::placeholders::_1));

  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&StreamPanel::PathCallback, this, std::placeholders::_1));

  EventBus::GetInstance().subscribe<FileDropEvent>(
      std::bind(&StreamPanel::FileDropCallback, this, std::placeholders::_1));

  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&StreamPanel::ContextCallback, this, std::placeholders::_1));
}

void StreamPanel::Render() {
  auto& context = StudioContext::GetInstance();
  static float cached_width = -1.0;
  if (cached_width < 0) {
    float maxWidth = ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, Labels::STREAM);
    cached_width = maxWidth + 8 + 24;
  }
  WidthLeftColumn = context.TheMainWindow->WidthLeftColumn;

  RowStreamPath();

  m_NavBar.Render();

  DroppedFilePath.reset();
}

void StreamPanel::RowStreamPath() {
  auto& context = StudioContext::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Text(Labels::STREAM);

  ImGui::SameLine();
  ImGui::SetCursorPosX(WidthLeftColumn);

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

  ImGui::SetCursorPosX(WidthLeftColumn);
  auto apiText = "Stream API: " + Labels::ApiToString(context.GetStreamAPI());
  ImGui::Text(apiText.c_str());
}

void StreamPanel::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);

  if (!pathEvent.Mode.has_value() || pathEvent.Mode.value() != GitsAction::PLAYBACK) {
    return;
  }

  if (pathEvent.EventType == PathEvent::Type::CONFIG) {
    gui_actions::LoadConfigFile(GitsAction::PLAYBACK);
  }

  if (pathEvent.EventType == PathEvent::Type::INPUT_STREAM) {
    studio_actions::OpenStream();
  }
}

void StreamPanel::FileDropCallback(const Event& e) {
  auto& baseContext = BaseContext::GetBaseInstance();
  const FileDropEvent& fileDropEvent = static_cast<const FileDropEvent&>(e);
  auto gitsAction = baseContext.SelectedAction;
  if (gitsAction == GitsAction::PLAYBACK || gitsAction == GitsAction::SUBCAPTURE) {
    DroppedFilePath = fileDropEvent.FilePath;
  }
}

void StreamPanel::ContextCallback(const Event& e) {
  const ContextEvent& contextEvent = static_cast<const ContextEvent&>(e);

  auto& context = StudioContext::GetInstance();
  auto& configurationForMode = context.ConfigurationForAction(GitsAction::PLAYBACK);

  if (contextEvent.EventType == ContextEvent::Type::InMemoryConfigurationChanged) {
    configurationForMode.ConfigurationDirty = true;
  }
}

void StreamPanel::ActionCallback(const Event& e) {
  const ActionEvent& actionEvent = static_cast<const ActionEvent&>(e);
  auto& context = StudioContext::GetInstance();
  if (actionEvent.EventType == ActionEvent::Type::StudioAnalysisExecuted) {
    studio_actions::ProcessAnalysis(true);
  }
}
} // namespace gits::gui
