// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "studioActions.h"

#include <filesystem>

#include "studioContext.h"
#include "guiActions.h"
#include "fileActions.h"
#include "baseCommon.h"

namespace gits::gui::studio_actions {
void OpenStream() {
  auto& context = StudioContext::GetInstance();
  auto streamPath = context.GetPathSafe(Path::INPUT_STREAM, GitsAction::PLAYBACK);
  if (streamPath.empty()) {
    return;
  }

  // load metadata
  context.ConfigurationForAction(GitsAction::PLAYBACK).MetaData = LoadStreamMetaData(streamPath);

  auto analysisPath = streamPath.parent_path() / "analysis_output";
  if (std::filesystem::exists(analysisPath)) {
    context.SetPath(analysisPath, Path::ANALYSIS, GitsAction::PLAYBACK);
  } else {
    context.SetPath("", Path::ANALYSIS, GitsAction::PLAYBACK);
  }

  const auto& metadata = context.ConfigurationForAction(GitsAction::PLAYBACK).MetaData;
  const auto& diags = metadata.RecorderDiags;
  if (diags.contains("gits") && diags["gits"].contains("captured_frames")) {
    context.PlayHeadState.TotalFrames = diags["gits"]["captured_frames"].get<uint64_t>();
  } else {
    context.PlayHeadState.TotalFrames = 0;
  }

  ProcessAnalysis(false);
}

void MakeConfigInteractive(Configuration& config) {
  config.common.player.interactive = true;
}

void AnalyzeStream(Configuration config, bool genScrenshots, bool genTrace) {
  auto& context = StudioContext::GetInstance();

  const auto streamPath = context.GetPathSafe(Path::INPUT_STREAM, GitsAction::PLAYBACK);
  if (streamPath.empty()) {
    LOG_ERROR << "No stream path set for analysis";
    return;
  }

  auto outputPath = context.GetPathSafe(gits::gui::Path::ANALYSIS);
  if (outputPath.empty()) {
    outputPath = streamPath.parent_path() / "analysis_output";
    if (outputPath.empty()) {
      LOG_ERROR << "No output path set for analysis";
      return;
    }
    if (!std::filesystem::exists(outputPath)) {
      if (!FileActions::CreateDirectorySafe(outputPath)) {
        LOG_ERROR << "Could not create output directory for analysis: " << outputPath.string();
        return;
      }
    }
  }
  context.SetPath(outputPath, gits::gui::Path::ANALYSIS);
  gui_actions::AnalyzeStreamAsync(streamPath, outputPath, config, genScrenshots, genTrace);
}

void ProcessAnalysis(bool logFail) {
  auto& context = StudioContext::GetInstance();

  auto& analysisState = context.Analysis;
  analysisState.Screenshots = false;
  analysisState.Trace = false;

  auto analysisPath = context.GetPathSafe(Path::ANALYSIS, GitsAction::PLAYBACK);
  if (analysisPath.empty()) {
    if (!logFail) {
      LOG_ERROR << "No analysis path set.";
    }
    EventBus::GetInstance().publish(ActionEvent::Type::StudioAnalysisFinished,
                                    ActionEvent::State::Ended, ActionEvent::Result::RESULT_FAILURE);
    return;
  }
  if (!std::filesystem::exists(analysisPath)) {
    if (!logFail) {
      LOG_ERROR << "Analysis path does not exist: " << analysisPath;
    }
    EventBus::GetInstance().publish(ActionEvent::Type::StudioAnalysisFinished,
                                    ActionEvent::State::Ended, ActionEvent::Result::RESULT_FAILURE);
    return;
  }

  // Create Panel with editor
  // Make panel subscribe to analysis done even & paths changed event - load when found.
  // Panel only visible when
  const auto tracePath = analysisPath / "trace" / "_tracefile.txt";
  if (!std::filesystem::exists(analysisPath)) {
    LOG_INFO << "No trace file found: " << tracePath;
  } else {
    context.traceFilePath = tracePath;
    analysisState.Trace = true;
  }

  const auto screenshotsPath = analysisPath / "screenshots";
  if (!std::filesystem::exists(screenshotsPath)) {
    if (!logFail) {
      LOG_ERROR << "Screenshots path does not exist: " << analysisPath;
    }
  } else {
    context.ScreenshotFiles.clear();
    for (const auto& entry : std::filesystem::directory_iterator(screenshotsPath)) {
      if (entry.is_regular_file() && entry.path().extension() == ".jpg" &&
          entry.path().filename().string().find("frame") == 0) {
        context.ScreenshotFiles.push_back(entry.path());
      }
    }
    LOG_INFO << "Found " << context.ScreenshotFiles.size() << " screenshots in " << screenshotsPath;
    if (context.PlayHeadState.TotalFrames == 0) {
      context.PlayHeadState.Init(context.ScreenshotFiles.size());
    }
    analysisState.Screenshots = true;
  }

  EventBus::GetInstance().publish(ActionEvent::Type::StudioAnalysisFinished,
                                  ActionEvent::State::Ended, ActionEvent::Result::RESULT_SUCCESS);
}
} // namespace gits::gui::studio_actions
