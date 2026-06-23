// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "contentPanel.h"

#include "context.h"
#include "launcherActions.h"
#include "labels.h"
#include "captureActions.h"
#include "MainWindow.h"
#include "eventBus.h"

namespace gits::gui {

typedef Context::SideBarItem SideBarItem;

ContentPanel::ContentPanel()
    : m_OptionsPanel(), m_YAMLPanel(), m_MetaDataPanel(), m_ResourceDumpPanel() {
  EventBus::GetInstance().subscribe<ActionEvent>(
      std::bind(&ContentPanel::CaptureActionCallback, this, std::placeholders::_1),
      {ActionEvent::Type::Capture});
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
    case SideBarItem::YAML_CONFIG:
      ImGui::BeginChild("yamlPanel", ImVec2(area.x, area.y), true);
      m_YAMLPanel.Render();
      ImGui::EndChild();
      break;
    case SideBarItem::LOG:
      ImGui::BeginChild("gitsLogArea", ImVec2(area.x, area.y), true);
      context.GITSLogEditor->Render();
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
      ImGui::BeginChild("OptionsPanel", ImVec2(area.x, area.y), true);
      m_OptionsPanel.Render();
      ImGui::EndChild();
      break;
    case SideBarItem::RESOURCE_DUMP:
      ImGui::BeginChild("Resource dump", ImVec2(area.x, area.y), true);
      m_ResourceDumpPanel.Render();
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

    const auto& streamDirectory = capture_actions::GetStreamDirectoryFromLog(content);
    if (streamDirectory.has_value()) {
      auto streamPath = streamDirectory.value() / filesystem_names::GITS_STREAM;
      context.SetPath(streamPath, Path::INPUT_STREAM, Mode::PLAYBACK);
      context.SetPath(std::move(streamPath), Path::INPUT_STREAM, Mode::SUBCAPTURE);
    }

    capture_actions::CleanupRecorderFiles(context.SelectedApiForCapture,
                                          context.TheMainWindow->GetCleanupOptions());
    break;
  }
  default:
    break;
  }
}

} // namespace gits::gui
