// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

#include "mainWindow.h"

namespace gits::gui {
typedef gits::gui::Context::SideBarItems SideBarItems;

void PlaybackPanel::Render() {
  RowStreamPath();
  RowConfigPath();
  RowArguments();
}

void PlaybackPanel::RowStreamPath() {
  auto& context = getSharedContext<gui::Context>();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  switch (context.AppMode) {
  case gui::Context::Mode::CAPTURE:
    ImGui::Text(Labels::TARGET);
    break;
  case gui::Context::Mode::PLAYBACK:
    ImGui::Text(Labels::STREAM);
    break;
  default:
    break;
  }

  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto allocatedWidth =
      availableWidth - ImGui::GetCursorPosX() -
      ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_GITSPLAYER);

  if (ImGuiHelper::InputString("###InputPath", context.StreamPath, 0, allocatedWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::STREAM_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_STREAM)) {
    ShowFileDialog(&context, FileDialogKeys::PICK_STREAM_PATH);
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_STREAM_HINT);
}

void PlaybackPanel::RowArguments() {
  auto& context = getSharedContext<gui::Context>();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CUSTOM_ARGS);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX() -
                        ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CLEAR_ARGUMENTS);
  if (ImGuiHelper::InputString("###CustomArgumentsInput", context.CustomArguments, 0,
                               remainingWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::CUSTOM_ARGS_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CLEAR_ARGUMENTS)) {
    context.CustomArguments.clear();
    UpdateCLICall(context);
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CLEAR_ARGUMENTS_HINT);
}

void PlaybackPanel::RowConfigPath() {
  auto& context = getSharedContext<gui::Context>();
  auto availableWidth = ImGui::GetContentRegionAvail().x;
  ImGui::Text(Labels::CONFIG);
  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX();
  auto allocatedWidth =
      remainingWidth - ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Button, Labels::CHOOSE_CONFIG);

  if (ImGuiHelper::InputString("###ConfigPathInput", context.ConfigPath, 0, allocatedWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::CONFIG_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_CONFIG)) {
    ShowFileDialog(&context, FileDialogKeys::PICK_CONFIG_PATH);
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_CONFIG_HINT);
}

} // namespace gits::gui
