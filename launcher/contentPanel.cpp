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
#include "eventBus.h"

namespace gits::gui {

typedef Context::SideBarItem SideBarItem;

ContentPanel::ContentPanel() : m_PluginsPanel(), CLIEditor("CLIEditor"), m_MetaDataPanel() {
  CLIEditor.SetConfig(TextEditorWidget::Config{.ShowToolbar = false});
  CLIEditor.GetEditor().SetReadOnly(true);
  CLIEditor.GetEditor().SetShowWhitespaces(false);
  CLIEditor.GetEditor().SetTabSize(4);
  CLIEditor.UpdatePalette();

  EventBus::GetInstance().subscribe<AppEvent>(
      std::bind(&ContentPanel::ThemeChangedCallback, this, std::placeholders::_1),
      {AppEvent::Type::ThemeChanged});
  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&ContentPanel::CliUpdatedCallback, this, std::placeholders::_1),
      {ContextEvent::Type::CLIUpdated});
  EventBus::GetInstance().subscribe<ActionEvent>(
      std::bind(&ContentPanel::CaptureActionCallback, this, std::placeholders::_1),
      {ActionEvent::Type::Capture});
  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&ContentPanel::PluginsUpdatedCallback, this, std::placeholders::_1),
      {ContextEvent::Type::PluginsUpdated});
}

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
  auto& context = Context::GetInstance();
  if (ImGui::BeginTable("MainLayoutTable", 2, ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, WidthColumn1() - 12.0f);
    ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    if (context.BtnsSideBar->Render(true)) {
      // nothing
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
        SetImGuiStyle(selectedItem);
      }
    }
    ImGui::TableSetColumnIndex(1);
    auto area = ImGui::GetContentRegionAvail();
    switch (context.BtnsSideBar->Selected()) {
    case SideBarItem::CONFIG:
      ChildWindowConfig();
      break;
    case SideBarItem::LOG:
      ImGui::BeginChild("gitsLogArea", ImVec2(area.x, area.y), true);
      context.GITSLogEditor->Render();
      ImGui::EndChild();
      break;
    case SideBarItem::CLI:
      ImGui::BeginChild("CLIArea", ImVec2(area.x, area.y), true);
      CLIEditor.Render();
      ImGui::EndChild();
      break;
    case SideBarItem::APP_LOG:
      ImGui::BeginChild("LauncherLogArea", ImVec2(area.x, area.y), true);
      context.LogEditor->Render();
      ImGui::EndChild();
      break;
    case SideBarItem::STATS:
      ImGui::BeginChild("StatsArea", ImVec2(area.x, area.y), true);
      m_MetaDataPanel.Render();
      ImGui::EndChild();
      break;
    case SideBarItem::OPTIONS:
      ImGui::BeginChild("StatsArea", ImVec2(area.x, area.y), true);
      context.PlaybackOptionsPanel->Render();
      ImGui::EndChild();
      break;
    case SideBarItem::PLUGINS:
      ImGui::BeginChild("Plugins", ImVec2(area.x, area.y), true);
      m_PluginsPanel.Render();
      ImGui::EndChild();
      break;
    case SideBarItem::RESOURCE_DUMP:
      ImGui::BeginChild("Resource dump", ImVec2(area.x, area.y), true);
      context.ResourceDumpPanel->Render();
      ImGui::EndChild();
      break;
    default:
      ImGui::BeginChild("ContentArea", ImVec2(area.x, area.y), true);
      ImGui::Text("Not implemented yet.");
      ImGui::EndChild();
      break;
    }
    ImGui::EndTable();
  }
}

void ContentPanel::ChildWindowConfig() {
  auto& context = Context::GetInstance();
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

void ContentPanel::ThemeChangedCallback(const Event& event) {
  CLIEditor.UpdatePalette();
}

void ContentPanel::CliUpdatedCallback(const Event& event) {
  auto& context = Context::GetInstance();
  const auto gitsExecutable = context.GetGITSPlayerPath();

  switch (context.AppMode) {
  case Mode::PLAYBACK:
  // [[fallthrough]]
  case Mode::SUBCAPTURE: {
    std::string cliEditorText =
        "# This buffer is write protected, it shows the current command line\n\n";

    auto executableSpecified = false;
    if (gitsExecutable.empty()) {
      cliEditorText += "Error: no GITS-Player specified!\n\n";
    } else if (!std::filesystem::exists(gitsExecutable)) {
      cliEditorText += "Error: GITS-Player does not exist\n> " + gitsExecutable.string() + "\n\n";
    } else {
      cliEditorText += gitsExecutable.string() + "\n";
      executableSpecified = true;
    }
    if (!executableSpecified) {
      cliEditorText += "Arguments for GITS-Player:\n";
    }
    for (const auto& argument : context.CLIArguments) {
      cliEditorText += "  " + argument + "\n";
    }

    CLIEditor.SetText(cliEditorText);

    break;
  }
  case Mode::CAPTURE: {
    const auto targetExecutable = context.GetPathSafe(Path::CAPTURE_TARGET);
    std::string cliEditorText =
        "# This buffer is write protected, it shows the current command line\n\n";

    cliEditorText += targetExecutable.string() + "\n";
    if (!std::filesystem::exists(targetExecutable)) {
      cliEditorText += "!!  [Warning: Target executable path does not exist]\n";
    }

    for (const auto& argument : context.CLIArguments) {
      cliEditorText += "  " + argument + "\n";
    }
    CLIEditor.SetText(cliEditorText);

    break;
  }
  default:
    break;
  }
}

void ContentPanel::CaptureActionCallback(const Event& e) {
  const ActionEvent& actionEvent = static_cast<const ActionEvent&>(e);

  auto& context = Context::GetInstance();

  switch (actionEvent.ActionState) {
  case ActionEvent::State::Started: {
    context.GITSLogEditor->SetText("");
    break;
  }
  case ActionEvent::State::Ended: {
    auto logPath = capture_actions::FindLatestRecorderLog(
        context.GetPathSafe(Path::CAPTURE_TARGET).parent_path());

    if (logPath.empty()) {
      context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);
      return;
    }

    std::ifstream file(logPath);
    if (!file.is_open()) {
      context.BtnsSideBar->SelectEntry(Context::SideBarItem::APP_LOG);
      return;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    context.GITSLogEditor->AppendText(content);

    context.BtnsSideBar->SelectEntry(Context::SideBarItem::LOG);

    capture_actions::CleanupRecorderFiles(context.SelectedApiForCapture,
                                          context.TheMainWindow->GetCleanupOptions());
    break;
  }
  default:
    break;
  }
}

void ContentPanel::PluginsUpdatedCallback(const Event& e) {
  LoadConfigFile();
}

} // namespace gits::gui
