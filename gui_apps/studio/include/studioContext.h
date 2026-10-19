// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "imgui.h"
#include <filesystem>
#include <functional>
#include <fstream>
#include <utility>
#include <optional>
#include <iostream>
#include <atomic>
#include <yaml-cpp/yaml.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "common.h"
#include "basePanel.h"
#include "imGuiHelper.h"
#include "buttonGroup.h"
#include "tabGroup.h"
#include "textEditorWidget.h"

#include "baseContext.h"
#include "studioConfig.h"
#include "playbackSession.h"
#include "log.h"

#include <plog/Appenders/IAppender.h>
#include <plog/Record.h>

namespace gits::gui {

typedef gits::ImGuiHelper::TextEditorWidget TextEditorWidget;

class MainWindow;

class StudioContext : public BaseContext {
public:
  static StudioContext& GetInstance() {
    static StudioContext context;
    return context;
  }

  BaseConfig* GetBaseConfiguration() override;

  void UpdatePalette();

  std::shared_ptr<MainWindow> TheMainWindow;

  void ToogleLiveStreamMode();
  bool IsLiveMode() const;

  StudioConfig* StudioConfiguration;
  AnalysisState Analysis;

  PlayHead PlayHeadState;
  PlaybackSession Playback;
  std::string LiveWorkloadArguments = ""; // Custom arguments added by user
  std::vector<std::filesystem::path> ScreenshotFiles;
  std::filesystem::path traceFilePath;

private:
  StudioContext();
  bool m_LiveMode;
};

} // namespace gits::gui
