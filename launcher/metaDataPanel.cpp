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

#include <filesystem>

namespace {
// Message and label text constants used by the metadata panel
static constexpr const char* EMPTY_STREAM_PATH_MESSAGE =
    "Can't get stream metadata, no stream path was selected";
static constexpr const char* STREAM_PATH_DOESNT_EXIST_MESSAGE =
    "Can't get stream metadata, selected stream path doesn't exist";
static constexpr const char* STATS_GATHERING_IN_PROGRESS_MESSAGE = "Gathering stream stats...";
static constexpr const char* STATS_GATHERING_PROMPT_MESSAGE =
    "Click the button below to gather stream stats (this launches the gits player).";
static constexpr const char* STATS_GATHERING_BUTTON_LABEL = "Gather stats";
static constexpr const char* UNKNOWN_METADATA_TAB_MESSAGE = "Couldn't get stream meta data";
static constexpr const char* EMPTY_RECORDER_DIAGS_MESSAGE =
    "Couldn't get recorder diagnostic information for given trace.";
static constexpr const char* EMPTY_RECORDER_CONFIG_MESSAGE =
    "Couldn't get recorder config for given trace.";
} // namespace

namespace gits::gui {
MetaDataPanel::MetaDataPanel(ISharedContext& sharedContext) : BasePanel(sharedContext) {}

void MetaDataPanel::InvalidateMetaData() {
  MetaDataNeedsUpdating = true;
  ClearStats();
}

void MetaDataPanel::Render() {
  auto& context = getSharedContext<gui::Context>();
  const auto& streamPath = context.GetPath(Context::Paths::STREAM);

  if (streamPath.empty()) {
    ImGui::Text(EMPTY_STREAM_PATH_MESSAGE);
    return;
  }

  if (!std::filesystem::exists(streamPath)) {
    ImGui::Text(STREAM_PATH_DOESNT_EXIST_MESSAGE);
    return;
  }

  if (MetaDataNeedsUpdating) {
    LoadMetaData(streamPath);
  }

  ImVec2 available = ImGui::GetContentRegionAvail();
  if (ImGui::BeginChild("MetaDataPanel", ImVec2(available.x, available.y), true)) {
    context.BtnsMetaData->Render();
    switch (context.BtnsMetaData->Selected()) {
    case Context::MetaDataItems::DIAGS:
      ImGui::NewLine();
      context.DiagsEditor->Render(available);
      break;
    case Context::MetaDataItems::CONFIG:
      ImGui::NewLine();
      context.TraceConfigEditor->Render(available);
      break;
    case Context::MetaDataItems::STATS:
      ImGui::NewLine();
      if (StatsGatheringInProgress) {
        ImGui::TextColored(ImVec4(0.8f, 0.15f, 0.15f, 1.0f), STATS_GATHERING_IN_PROGRESS_MESSAGE);
      } else if (!StatsGatheringInProgress && !Stats.empty()) {
        context.TraceStatsEditor->Render(available);
      } else {
        ImGui::Text(STATS_GATHERING_PROMPT_MESSAGE);
        if (ImGui::Button(STATS_GATHERING_BUTTON_LABEL)) {
          context.TraceStatsEditor->SetText("");
          std::string statistics;
          const auto& gitsPlayerPath = context.GetPath(Context::Paths::GITS_PLAYER);

          StatsGatheringInProgress = true;
          FileActions::LaunchExecutableThreadCallbackOnExit(
              gitsPlayerPath, {"--stats", streamPath.string()}, gitsPlayerPath.parent_path(),
              [this](const std::string& stats) { this->AppendStats(stats); },
              [this, &context]() {
                this->FillStatsEditor();
                this->StatsGatheringInProgress = false;
              });
        }
      }
      break;
    default:
      ImGui::Text(UNKNOWN_METADATA_TAB_MESSAGE);
      break;
    }
  }
  ImGui::EndChild();
}
void MetaDataPanel::LoadMetaData(std::filesystem::path streamPath) {
  MetaDataNeedsUpdating = false;
  auto& context = getSharedContext<gui::Context>();
  MetaData = GetStreamMetaData(streamPath);
  context.DiagsEditor->SetText(MetaData.RecorderDiags.empty() ? EMPTY_RECORDER_DIAGS_MESSAGE
                                                              : MetaData.RecorderDiags.dump(2));
  context.TraceConfigEditor->SetText(MetaData.RecorderConfig.empty() ? EMPTY_RECORDER_CONFIG_MESSAGE
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
  auto& context = getSharedContext<gui::Context>();
  // Remove everything before the "Statistics" line (which appears exactly once)
  size_t pos = Stats.find("Statistics");
  if (pos != std::string::npos) {
    Stats = Stats.substr(pos);
  }

  context.TraceStatsEditor->SetText(Stats);
}
} // namespace gits::gui
