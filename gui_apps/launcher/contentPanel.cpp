// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "contentPanel.h"

#include "context.h"
#include "baseContext.h"
#include "launcherActions.h"
#include "labels.h"
#include "captureActions.h"
#include "mainWindow.h"
#include "baseEventBus.h"
#include "contextHelper.h"

namespace gits::gui {

typedef Context::SideBarItem SideBarItem;

ContentPanel::ContentPanel()
    : m_OptionsPanel(), m_YAMLPanel(), m_MetaDataPanel(), m_ResourceDumpPanel() {

  EventBus::GetInstance().subscribe<ActionEvent>(
      std::bind(&ContentPanel::CaptureActionCallback, this, std::placeholders::_1),
      {ActionEvent::Type::Capture});
}

ContentPanel::~ContentPanel() {
  // nothing
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

    context.BtnsSideBar->Render(true);

    ImGui::TableSetColumnIndex(1);
    bool renderDummy = false;
    auto area = ImGui::GetContentRegionAvail();
    if (context.BtnsSideBar->HasSelection()) {
      switch (context.BtnsSideBar->Selected()) {
      case SideBarItem::YAML_CONFIG:
        ImGui::BeginChild("yamlPanel", ImVec2(area.x, area.y), true);
        m_YAMLPanel.Render();
        ImGui::EndChild();
        break;
      case SideBarItem::LOG:
        ImGui::BeginChild("gitsLogArea", ImVec2(area.x, area.y), true);
        context.RenderGITSLogEditor();
        ImGui::EndChild();
        break;
      case SideBarItem::APP_LOG:
        ImGui::BeginChild("LauncherLogArea", ImVec2(area.x, area.y), true);
        context.RenderLogEditor();
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
        renderDummy = true;
        break;
      }
    }
    if (renderDummy) {
      ImGui::BeginChild("ContentArea", ImVec2(area.x, area.y), true);
      ImGui::Text("Not implemented yet.");
      ImGui::EndChild();
    }
    ImGui::EndTable();
  }
}

void ContentPanel::RenderSimple() {
  auto& context = Context::GetInstance();

  m_OptionsPanel.RenderSimple();

  if (!context.HasGITSLogToRender()) {
    return;
  }

  ImVec2 logAreaSize = ImGui::GetContentRegionAvail();
  ImGui::BeginChild("ExecutionLogSimple", logAreaSize, true);
  context.RenderGITSLogEditor();
  ImGui::EndChild();
}

void ContentPanel::CaptureActionCallback(const Event& e) {
  const ActionEvent& actionEvent = static_cast<const ActionEvent&>(e);

  auto& context = Context::GetInstance();
  auto& baseContext = BaseContext::GetBaseInstance();

  switch (actionEvent.ActionState) {
  case ActionEvent::State::Started: {
    baseContext.SetGITSLogText("");
    break;
  }
  case ActionEvent::State::Ended: {
    std::filesystem::path logDirectory =
        baseContext.GetPathSafe(Path::CAPTURE_TARGET).parent_path();
#ifndef _WIN32
    const auto recorderDirectoryName =
        capture_actions::GetRecorderDirectoryNameForApi(baseContext.SelectedApiForCapture);
    const auto recorderDirectory = baseContext.GetPathSafe(Path::GITS_BASE) /
                                   filesystem_names::RECORDER_DIRECTORY_NAME /
                                   recorderDirectoryName;
    if (!recorderDirectoryName.empty() && std::filesystem::exists(recorderDirectory)) {
      logDirectory = recorderDirectory;
    }
#endif
    auto logPath = capture_actions::FindLatestRecorderLog(logDirectory);

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
    baseContext.AddGITSLogText(content);
    context.BtnsSideBar->SelectEntry(Context::SideBarItem::LOG);

    const auto& streamDirectory = capture_actions::GetStreamDirectoryFromLog(content);
    if (streamDirectory.has_value()) {
      auto streamPath = streamDirectory.value() / filesystem_names::GITS_STREAM;
      baseContext.SetPath(streamPath, Path::INPUT_STREAM, GitsAction::PLAYBACK);
      baseContext.SetPath(std::move(streamPath), Path::INPUT_STREAM, GitsAction::SUBCAPTURE);
    }

    capture_actions::CleanupRecorderFiles(baseContext.SelectedApiForCapture,
                                          context.TheMainWindow->GetCleanupOptions());
    break;
  }
  default:
    break;
  }
}

} // namespace gits::gui
