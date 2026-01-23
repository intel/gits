// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "contentPanel.h"

#include "imGuiHelper.h"

#include "resource.h"
#include "context.h"
#include "launcherActions.h"
#include "labels.h"
#include "captureActions.h"
#include "MainWindow.h"

namespace gits::gui {

typedef gits::gui::Context::SideBarItems SideBarItems;

float ContentPanel::WidthColumn1(bool resetSize) {
  static float cached_width = -1.0;
  if (cached_width < 0 || resetSize) {
    const std::vector<std::string> row_labels = {Labels::STREAM,
                                                 Labels::TARGET,
                                                 Labels::CLEANUP,
                                                 Labels::PLAYER_PATH,
                                                 Labels::BASE_PATH,
                                                 Labels::CONFIG,
                                                 Labels::CAPTURE_CONFIG,
                                                 Labels::CUSTOM_ARGS,
                                                 Labels::CAPTURE_CUSTOM_ARGS,
                                                 Labels::CAPTURE_OUTPUT_PATH};
    float maxWidth = Labels::MaxLength(row_labels);
    maxWidth = std::max(maxWidth, Labels::MaxLength(Labels::SIDE_BAR_VEC()));
    cached_width = maxWidth + 8 + 24;
  }
  return cached_width;
}

void ContentPanel::Render() {
  auto& context = getSharedContext<Context>();
  if (ImGui::BeginTable("MainLayoutTable", 2, ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, WidthColumn1() - 12.0f);
    ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    if (context.BtnsSideBar->Render(true)) {
      context.CurrentMainAction =
          context.IsPlayback() ? gui::Context::MainAction::PLAYBACK
                               : (context.IsCapture() ? gui::Context::MainAction::CAPTURE
                                                      : gui::Context::MainAction::SUBCAPTURE);
    }
    float width = ImGui::GetContentRegionAvail().x;
    float remaining = ImGui::GetContentRegionAvail().y;
    bool showDropDown = (remaining > 2.0 * ImGui::GetTextLineHeight());
    float horizontalSkip = ImGui::GetContentRegionAvail().y - 2.5 * ImGui::GetTextLineHeight();

    if (showDropDown) {
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + horizontalSkip);

      static int selectedItem = context.LauncherConfiguration.Theme.CurrentThemeIdx;
      ImVec2 buttonSize = ImVec2((width - 8.0f) * 0.5f, 0.0f);
      if (ImGui::Button("UI-", buttonSize)) {
        context.UIScaleDelta = -0.1f;
      }
      ImGui::SameLine();
      if (ImGui::Button("UI+", buttonSize)) {
        context.UIScaleDelta = +0.1f;
      }

      ImGui::SetNextItemWidth(width);
      if (ImGui::Combo("##ThemeSelector", &selectedItem,
                       context.LauncherConfiguration.Theme.ThemeLabels.data(),
                       context.LauncherConfiguration.Theme.ThemeLabels.size())) {
        SetImGuiStyle(&context, selectedItem);
      }
    }
    ImGui::TableSetColumnIndex(1);
    auto area = ImGui::GetContentRegionAvail();
    switch (context.BtnsSideBar->Selected()) {
    case SideBarItems::CONFIG:
      ChildWindowConfig();
      break;
    case SideBarItems::LOG:
      ImGui::BeginChild("gitsLogArea", ImVec2(area.x, area.y), true);
      context.GITSLogEditor->Render();
      ImGui::EndChild();
      break;
    case SideBarItems::CLI:
      ImGui::BeginChild("CLIArea", ImVec2(area.x, area.y), true);
      context.CLIEditor->Render();
      ImGui::EndChild();
      break;
    case SideBarItems::APP_LOG:
      ImGui::BeginChild("LauncherLogArea", ImVec2(area.x, area.y), true);
      context.LogEditor->Render();
      ImGui::EndChild();
      break;
    case SideBarItems::STATS:
      ImGui::BeginChild("StatsArea", ImVec2(area.x, area.y), true);
      context.MetaDataPanel->Render();
      ImGui::EndChild();
      break;
    case SideBarItems::OPTIONS:
      ImGui::BeginChild("StatsArea", ImVec2(area.x, area.y), true);
      context.EasyOptionsPanel->Render();
      ImGui::EndChild();
      break;
    default:
      ImGui::BeginChild("ContentArea", ImVec2(area.x, area.y), true);
      ImGui::Text("Not implemented yet.");
      ImGui::EndChild();
      return;
    }
    ImGui::EndTable();
  }

  if (context.IsCapture() && context.RecordingProcessingPending) {
    // TODO: Either move this or replace with a thread safe callback in the capture function?
    context.RecordingProcessingPending = false;
    auto logPath = gui::capture_actions::FindLatestRecorderLog(
        context.GetPath(gui::Context::Paths::TARGET).parent_path());

    if (logPath.empty()) {
      context.BtnsSideBar->SelectEntry(gui::Context::SideBarItems::APP_LOG);

      return;
    }

    std::ifstream file(logPath);
    if (!file.is_open()) {
      context.BtnsSideBar->SelectEntry(gui::Context::SideBarItems::APP_LOG);

      return;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    context.GITSLogEditor->AppendText(content);

    context.BtnsSideBar->SelectEntry(gui::Context::SideBarItems::LOG);
    gui::capture_actions::CleanupRecorderFiles(context, context.SelectedApiForCapture,
                                               context.TheMainWindow->GetCleanupOptions());
  }
}

void ContentPanel::ChildWindowConfig() {
  auto& context = getSharedContext<gui::Context>();
  ImVec2 available = ImGui::GetContentRegionAvail();
  if (ImGui::BeginChild("ContentArea", ImVec2(available.x, available.y), true)) {

    if (context.BtnsAPI->Render()) {
      auto lineIdx = context.ConfigSectionLines[context.BtnsAPI->Selected()];
      if (lineIdx > -1) {
        auto pos = TextEditor::Coordinates(lineIdx, 0);
        context.ConfigEditor->GetEditor().SetCursorPosition(pos);
      } else {
        LOG_INFO << "Config section line index is invalid or not found. Selected index: "
                 << context.BtnsAPI->SelectedIndex()
                 << ", item:" << context.BtnsAPI->SelectedItem().label;
      }
    }
    context.ConfigEditor->Render(available);
  }
  ImGui::EndChild();
}
} // namespace gits::gui
