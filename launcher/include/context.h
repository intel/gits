// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
#include <yaml-cpp/yaml.h>

#include <windows.h>

#include "fileActions.h"
#include "imGuiHelper.h"
#include "buttonGroup.h"
#include "textEditorWidget.h"

// Add plog appender includes
#include <plog/Appenders/IAppender.h>
#include <plog/Record.h>
#include <plog/Util.h>
#include <plog/Formatters/TxtFormatter.h>

namespace gits::gui {

typedef gits::ImGuiHelper::TextEditorWidget TextEditorWidget;

class TextEditorAppender;

struct Context {
  enum class Mode {
    PLAYBACK,
    CAPTURE,
  };

  enum class MainAction {
    PLAYBACK = 0,
    STATISTICS,
    COUNT
  };

  enum class Paths {
    GITS_PLAYER,
    STREAM,
    CONFIG,
    COUNT
  };

  enum class SideBarItems {
    CONFIG = 0,
    CLI,
    STATS,
    LOG,
    APP_LOG,
    OPTIONS,
    INFO,
    API_TRACE,
    COUNT
  };

  enum class ConfigSectionItems {
    COMMON = 0,
    DIRECTX,
    OPENGL,
    VULKAN,
    OPENCL,
    LEVELZERO,
    OVERRIDES,
    COUNT
  };

  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> ConfigEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> CLIEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> LogEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> GITSLogEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> TraceInfoEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> TraceStatsEditor;
  std::unique_ptr<TextEditorAppender> LogAppender;

  ImGuiHelper::ButtonGroup<SideBarItems>* BtnsSideBar;
  ImGuiHelper::ButtonGroup<ConfigSectionItems>* BtnsAPI;

  Mode AppMode = Mode::PLAYBACK;
  MainAction CurrentMainAction = MainAction::PLAYBACK;

  std::filesystem::path GITSPlayerPath = ".";
  std::filesystem::path StreamPath = "";
  std::filesystem::path ConfigPath = ".";
  std::string FixedLauncherArguments = ""; // Arguments created by the launcher
  std::string CustomArguments = "";        // Custom arguments added by user
  std::vector<std::string> CLIArguments;

  std::map<ConfigSectionItems, int> ConfigSectionLines = {};

  bool MainActionAllowed() {
    return false;
  }

  void GITSLog(const std::string& msg) {
    GITSLogEditor->AppendText(msg);
  }

  void TraceStats(const std::string& msg) {
    if (msg.empty()) {
      return;
    }
    TraceStatsEditor->AppendText(msg, false);
  }

  std::filesystem::path GetPath(Paths path) const {
    switch (path) {
    case Paths::STREAM:
      return StreamPath;
    case Paths::GITS_PLAYER:
      return GITSPlayerPath;
    case Paths::CONFIG:
      return ConfigPath;
    default:
      return std::filesystem::path();
    }
  }

  bool IsPlayback() {
    return AppMode == Mode::PLAYBACK;
  }
};

class TextEditorAppender : public plog::IAppender {
public:
  TextEditorAppender(TextEditorWidget* editor) : m_Editor(editor) {}

  virtual void write(const plog::Record& record) override {
    if (!m_Editor) {
      return;
    }

    plog::util::nstring str = plog::TxtFormatter::format(record);
    std::string logLine = plog::util::toNarrow(str, 0);
    if (!logLine.empty() && logLine.back() == '\n') {
      logLine.pop_back();
    }
    std::string currentText = m_Editor->GetText();
    currentText += logLine;
    m_Editor->SetText(currentText);
  }

private:
  TextEditorWidget* m_Editor;
};
} // namespace gits::gui
