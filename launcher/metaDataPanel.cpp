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

namespace gits::gui {
MetaDataPanel::MetaDataPanel() {
  EventBus::GetInstance().subscribe<PathEvent>(
      std::bind(&MetaDataPanel::StreamPathCallback, this, std::placeholders::_1),
      {PathEvent::Type::INPUT_STREAM});
  EventBus::GetInstance().subscribe<ContextEvent>(
      std::bind(&MetaDataPanel::MetaDataCallback, this, std::placeholders::_1),
      {ContextEvent::Type::MetadataLoaded});
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

  ImVec2 available = ImGui::GetContentRegionAvail();
  if (ImGui::BeginChild("MetaDataPanel", ImVec2(available.x, available.y), true)) {
    context.BtnsMetaData->Render();
    switch (context.BtnsMetaData->Selected()) {
    case Context::MetaDataItem::DIAGS:
      ImGui::NewLine();
      context.DiagsEditor->Render(available);
      break;
    case Context::MetaDataItem::CONFIG:
      ImGui::NewLine();
      context.TraceConfigEditor->Render(available);
      break;
    case Context::MetaDataItem::STATS:
      ImGui::NewLine();
      if (StatsGatheringInProgress) {
        ImGui::TextColored(ImVec4(0.8f, 0.15f, 0.15f, 1.0f),
                           Labels::STATS_GATHERING_IN_PROGRESS_MESSAGE);
      } else if (!StatsGatheringInProgress && !Stats.empty()) {
        context.TraceStatsEditor->Render(available);
      } else {
        ImGui::Text(Labels::STATS_GATHERING_PROMPT_MESSAGE);
        if (ImGui::Button(Labels::STATS_GATHERING_BUTTON_LABEL)) {
          context.TraceStatsEditor->SetText("");
          std::string statistics;
          const auto& gitsPlayerPath = context.GetGITSPlayerPath();

          StatsGatheringInProgress = true;
          FileActions::LaunchExecutableThreadCallbackOnExit(
              gitsPlayerPath, {"--stats", streamPathValue.string()}, gitsPlayerPath.parent_path(),
              [this](const std::string& stats) { this->AppendStats(stats); },
              [this, &context]() {
                this->FillStatsEditor();
                this->StatsGatheringInProgress = false;
              });
        }
      }
      break;
    default:
      ImGui::Text(Labels::UNKNOWN_METADATA_TAB_MESSAGE);
      break;
    }
  }
  ImGui::EndChild();
}

void MetaDataPanel::LoadMetaData(std::filesystem::path streamPath) {
  auto& context = Context::GetInstance();
  auto MetaData = context.MetaData;
  context.DiagsEditor->SetText(MetaData.RecorderDiags.empty() ? Labels::EMPTY_RECORDER_DIAGS_MESSAGE
                                                              : MetaData.RecorderDiags.dump(2));
  context.TraceConfigEditor->SetText(MetaData.RecorderConfig.empty()
                                         ? Labels::EMPTY_RECORDER_CONFIG_MESSAGE
                                         : MetaData.RecorderConfig);
}

void MetaDataPanel::ClearStats() {
  std::lock_guard guard{StatsMutex};
  Stats.clear();
}

void MetaDataPanel::AppendStats(std::string str) {
  std::lock_guard guard{StatsMutex};
  Stats.append(str);
}

void MetaDataPanel::FillStatsEditor() {
  std::lock_guard guard{StatsMutex};
  auto& context = Context::GetInstance();
  // Remove everything before the "Statistics" line (which appears exactly once)
  size_t pos = Stats.find("Statistics");
  if (pos != std::string::npos) {
    Stats = Stats.substr(pos);
  }

  context.TraceStatsEditor->SetText(Stats);
}

void MetaDataPanel::MetaDataCallback(const Event& e) {
  auto& context = Context::GetInstance();
  auto MetaData = context.MetaData;
  context.DiagsEditor->SetText(MetaData.RecorderDiags.empty() ? Labels::EMPTY_RECORDER_DIAGS_MESSAGE
                                                              : MetaData.RecorderDiags.dump(2));
  context.TraceConfigEditor->SetText(MetaData.RecorderConfig.empty()
                                         ? Labels::EMPTY_RECORDER_CONFIG_MESSAGE
                                         : MetaData.RecorderConfig);
}

void MetaDataPanel::StreamPathCallback(const Event& e) {
  ClearStats();
}
} // namespace gits::gui
