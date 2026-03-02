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
#include "guiController.h"
#include "labels.h"
#include "eventBus.h"

#include "mainWindow.h"
#include "contextHelper.h"

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
}

void PlaybackPanel::Render() {
  RowStreamPath();
  RowConfigPath();
  RowArguments();
}

void PlaybackPanel::RowStreamPath() {
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Text(Labels::STREAM);

  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto allocatedWidth = availableWidth - ImGui::GetCursorPosX() - WidthLastButton();

  context_helper::PathInput("###InputPath", Path::INPUT_STREAM, context.AppMode, 0, allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::STREAM_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_STREAM, ImVec2(WidthLastButton(), 0))) {
    ShowFileDialog(FileDialogKeys{Path::INPUT_STREAM, context.AppMode});
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_STREAM_HINT);
}

void PlaybackPanel::RowArguments() {
  auto& context = Context::GetInstance();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CUSTOM_ARGS);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX() - WidthLastButton();
  if (ImGuiHelper::InputString("###CustomArgumentsInput", context.CustomArguments, 0,
                               remainingWidth)) {
    UpdateCLICall();
  }
  ImGuiHelper::AddTooltip(Labels::CUSTOM_ARGS_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CLEAR_ARGUMENTS, ImVec2(WidthLastButton(), 0))) {
    context.CustomArguments.clear();
    UpdateCLICall();
  }
  ImGui::PopID();
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

  context_helper::PathInput("###ConfigPathInput", gui::Path::CONFIG, gui::Mode::PLAYBACK, 0,
                            allocatedWidth);
  ImGuiHelper::AddTooltip(Labels::CONFIG_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_CONFIG, ImVec2(WidthLastButton(), 0))) {
    ShowFileDialog(FileDialogKeys{Path::CONFIG, Mode::PLAYBACK});
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_CONFIG_HINT);
}

void PlaybackPanel::PathCallback(const Event& e) {
  const PathEvent& pathEvent = static_cast<const PathEvent&>(e);

  if (!pathEvent.Mode.has_value() || pathEvent.Mode.value() != Mode::PLAYBACK) {
    return;
  }

  if (pathEvent.EventType == PathEvent::Type::CONFIG) {
    LoadConfigFile();
  }

  UpdateCLICall();
}

} // namespace gits::gui
