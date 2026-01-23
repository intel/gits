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

#include "mainWindow.h"

namespace {
float WidthLastButton() {
  auto labels = {gits::gui::Labels::CHOOSE_STREAM, gits::gui::Labels::CLEAR_ARGUMENTS,
                 gits::gui::Labels::CHOOSE_CONFIG};

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
typedef gits::gui::Context::SideBarItems SideBarItems;

void PlaybackPanel::Render() {
  RowStreamPath();
  RowConfigPath();
  RowArguments();
}

void PlaybackPanel::RowStreamPath() {
  auto& context = getSharedContext<gui::Context>();
  auto availableWidth = ImGui::GetContentRegionAvail().x;

  ImGui::Text(Labels::STREAM);

  ImGui::SameLine();
  ImGui::SetCursorPosX(context.TheMainWindow->WidthLeftColumn);

  auto allocatedWidth = availableWidth - ImGui::GetCursorPosX() - WidthLastButton();

  if (ImGuiHelper::InputString("###InputPath", context.StreamPath, 0, allocatedWidth)) {
    UpdateCLICall(context);
    // TODO: This should be acted upon a message (once we have them)
    context.MetaDataPanel->InvalidateMetaData();
  }
  ImGuiHelper::AddTooltip(Labels::STREAM_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_STREAM, ImVec2(WidthLastButton(), 0))) {
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

  auto remainingWidth = availableWidth - ImGui::GetCursorPosX() - WidthLastButton();
  if (ImGuiHelper::InputString("###CustomArgumentsInput", context.CustomArguments, 0,
                               remainingWidth)) {
    UpdateCLICall(context);
  }
  ImGuiHelper::AddTooltip(Labels::CUSTOM_ARGS_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CLEAR_ARGUMENTS, ImVec2(WidthLastButton(), 0))) {
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
  auto allocatedWidth = remainingWidth - WidthLastButton();

  if (ImGuiHelper::InputString("###ConfigPathInput", context.ConfigPath, 0, allocatedWidth)) {
    UpdateCLICall(context);
    LoadConfigFile(&context);
  }
  ImGuiHelper::AddTooltip(Labels::CONFIG_INPUT_HINT);

  ImGui::SameLine();
  ImGui::PushID(++context.ImguiIDs);
  if (ImGui::Button(Labels::CHOOSE_CONFIG, ImVec2(WidthLastButton(), 0))) {
    ShowFileDialog(&context, FileDialogKeys::PICK_CONFIG_PATH);
  }
  ImGui::PopID();
  ImGuiHelper::AddTooltip(Labels::CHOOSE_CONFIG_HINT);
}

} // namespace gits::gui
