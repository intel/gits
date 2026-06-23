// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "metaDataPanel.h"
#include "labels.h"
#include "context.h"
#include "fileActions.h"
#include "eventBus.h"

#include <filesystem>
#include "imgui.h"
#include <mutex>
#include <string>

namespace gits::gui {
MetaDataPanel::MetaDataPanel()
    : m_MetaDataEditor("MetaDataEditor"),
      m_MetaDataStatsEditor("MetaDataStatsEditor"),
      m_YamlTreeViewer() {
  m_YamlTreeViewer.SetTextEditor(&m_MetaDataEditor.GetEditor());
  m_MetaDataEditor.GetEditor().SetShowWhitespaces(false);
  m_MetaDataEditor.GetEditor().SetTabSize(4);
  m_MetaDataEditor.GetEditor().SetLanguageDefinition(TextEditorWidget::GetYamlLanguageDefinition());
  m_MetaDataEditor.SetConfig(TextEditorWidget::CONFIG_NO_TOOLBAR);

  m_MetaDataStatsEditor.GetEditor().SetShowWhitespaces(false);
  m_MetaDataStatsEditor.GetEditor().SetTabSize(4);
  m_MetaDataStatsEditor.GetEditor().SetLanguageDefinition(
      TextEditorWidget::GetYamlLanguageDefinition());
  m_MetaDataStatsEditor.SetConfig(TextEditorWidget::CONFIG_NO_TOOLBAR);

  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&MetaDataPanel::StreamPathCallback, this, std::placeholders::_1),
      {PathEvent::Type::INPUT_STREAM});
  EventBus::GetInstance().subscribe<AppEvent>(
      std::bind(&MetaDataPanel::ThemeChangedCallback, this, std::placeholders::_1),
      {AppEvent::Type::ThemeChanged});
}

void MetaDataPanel::Render() {
  auto& context = Context::GetInstance();
  const auto& streamPath = context.GetPath(Path::INPUT_STREAM);

  if (!streamPath.has_value()) {
    ImGui::Text(Labels::NULLOPT_STREAM_PATH_MESSAGE);
    return;
  }

  const auto& streamPathValue = streamPath.value();

  if (streamPathValue.empty()) {
    ImGui::Text(Labels::EMPTY_STREAM_PATH_MESSAGE);
    return;
  }

  if (!std::filesystem::exists(streamPathValue)) {
    ImGui::Text(Labels::STREAM_PATH_DOESNT_EXIST_MESSAGE);
    return;
  }

  auto currentSubPanel = m_ActiveSubPanel;
  ImVec2 available = ImGui::GetContentRegionAvail();
  if (ImGui::BeginChild("MetaDataPanel", ImVec2(available.x, available.y), true)) {
    if (ImGui::BeginTabBar("MetaDataTabs")) {
      if (ImGui::BeginTabItem("Configuration")) {
        currentSubPanel = SubPanel::Config;
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Diagnostics")) {
        currentSubPanel = SubPanel::Diags;
        ImGui::EndTabItem();
      }
      // Stats are currently broken
      //if (ImGui::BeginTabItem("Stats")) {
      //  currentSubPanel = SubPanel::Stats;
      //  ImGui::EndTabItem();
      //}
      ImGui::EndTabBar();
    }

    if (currentSubPanel != m_ActiveSubPanel) {
      m_ActiveSubPanel = currentSubPanel;
      UpdateContent();
    }

    const auto col1Ratio = std::min(256.0f / available.x, 0.25f);
    if (m_ActiveSubPanel == SubPanel::Stats) {
      std::lock_guard guard{m_StatsMutex};
      ImGui::NewLine();
      if (m_StatsGatheringInProgress) {
        ImGui::TextColored(ImVec4(0.8f, 0.15f, 0.15f, 1.0f),
                           Labels::STATS_GATHERING_IN_PROGRESS_MESSAGE);
      } else if (!m_StatsGatheringInProgress && !m_StatsStr.empty()) {
        m_MetaDataStatsEditor.Render(available);
      } else {
        ImGui::Text(Labels::STATS_GATHERING_PROMPT_MESSAGE);
        if (ImGui::Button(Labels::STATS_GATHERING_BUTTON_LABEL)) {
          m_MetaDataStatsEditor.SetText("");
          std::string statistics;
          const auto& gitsPlayerPath = context.GetGITSPlayerPath();

          m_StatsGatheringInProgress = true;
          FileActions::LaunchExecutableThreadCallbackOnExit(
              gitsPlayerPath, {"--stats", streamPathValue.string()}, gitsPlayerPath.parent_path(),
              [this](const std::string& stats) { this->AppendStats(stats); },
              [this, &context]() {
                this->FillStatsEditor();
                this->m_StatsGatheringInProgress = false;
              });
        }
      }
    } else {
      const auto& MetaData = context.ConfigurationForMode(Mode::PLAYBACK).MetaData;
      if (ImGui::BeginTable("MetaDataTable", 2,
                            ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("TreeViewColumn", ImGuiTableColumnFlags_WidthStretch, col1Ratio);
        ImGui::TableSetupColumn("EditorColumn", ImGuiTableColumnFlags_WidthStretch,
                                1.0f - col1Ratio);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        m_YamlTreeViewer.Render();
        ImGui::TableSetColumnIndex(1);
        m_MetaDataEditor.Render(available);
        ImGui::EndTable();
      }
    }
  }
  ImGui::EndChild();
}

void MetaDataPanel::ClearStats() {
  std::lock_guard guard{m_StatsMutex};
  m_StatsStr.clear();
}

void MetaDataPanel::AppendStats(std::string str) {
  std::lock_guard guard{m_StatsMutex};
  m_StatsStr.append(str);
}

void MetaDataPanel::FillStatsEditor() {
  std::lock_guard guard{m_StatsMutex};
  auto& context = Context::GetInstance();
  // Remove everything before the "Statistics" line (which appears exactly once)
  size_t pos = m_StatsStr.find("Statistics");
  if (pos != std::string::npos) {
    m_StatsStr = m_StatsStr.substr(pos);
  }

  m_MetaDataStatsEditor.SetText(m_StatsStr);
}

void MetaDataPanel::UpdateContent() {
  auto& context = Context::GetInstance();
  const auto& MetaData = context.ConfigurationForMode(Mode::PLAYBACK).MetaData;
  std::string yamlText = "";
  std::string yamlNodeText = "";
  if (m_ActiveSubPanel == SubPanel::Diags) {
    if (MetaData.RecorderDiags.empty()) {
      yamlText = Labels::EMPTY_RECORDER_DIAGS_MESSAGE;
    } else {
      yamlText = MetaData.RecorderDiags.dump(2);
      try {
        yamlText = YAML::Dump(MetaData.RecorderDiagsYAML);
        yamlNodeText = yamlText;
      } catch (const YAML::Exception& e) {
        LOG_ERROR << "MetaData:Diagnostics: YAML conversion error: " << e.what();
      }
    }
  } else if (m_ActiveSubPanel == SubPanel::Config) {
    if (MetaData.RecorderConfig.empty()) {
      yamlText = Labels::EMPTY_RECORDER_CONFIG_MESSAGE;
    } else {
      yamlText = MetaData.RecorderConfig;
      yamlNodeText = MetaData.RecorderConfig;
    }
  }
  m_YamlTreeViewer.SetYAMLText(yamlNodeText);
  m_MetaDataEditor.SetText(yamlText);

  // nothing
}

void MetaDataPanel::StreamPathCallback(const Event& e) {
  ClearStats();
  m_ActiveSubPanel = SubPanel::Unset;
}

void MetaDataPanel::ThemeChangedCallback(const Event& e) {
  m_MetaDataStatsEditor.UpdatePalette();
  m_MetaDataEditor.UpdatePalette();
}

} // namespace gits::gui
