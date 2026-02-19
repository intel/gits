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

#include "basePanel.h"
#include "imGuiHelper.h"
#include "buttonGroup.h"
#include "tabGroup.h"
#include "textEditorWidget.h"

#include "fileActions.h"
#include "launcherConfig.h"
#include "ezOptionsPanel.h"
#include "log.h"
#include "metaDataPanel.h"

#include <plog/Appenders/IAppender.h>
#include <plog/Record.h>

namespace gits::gui {

typedef gits::ImGuiHelper::TextEditorWidget TextEditorWidget;

class TextEditorAppender;
class MainWindow;

class Context : public ISharedContext {
public:
  enum class Mode {
    PLAYBACK,
    CAPTURE,
    SUBCAPTURE
  };

  enum class MainAction {
    PLAYBACK = 0,
    CAPTURE,
    SUBCAPTURE,
    COUNT
  };

  enum class Paths {
    GITS_PLAYER,
    GITS_BASE,
    STREAM,
    TARGET,
    CONFIG,
    CAPTURE_CONFIG,
    CAPTURE_OUTPUT,
    COUNT
  };

  enum class SideBarItems {
    OPTIONS = 0,
    CONFIG,
    CLI,
    STATS,
    LOG,
    APP_LOG,
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

  enum class Api {
    UNKNOWN = 0,
    DIRECTX,
    OPENGL,
    VULKAN,
    OPENCL,
    LEVELZERO,
    COUNT
  };

  enum class MetaDataItems {
    CONFIG = 0,
    STATS,
    DIAGS
  };

  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> ConfigEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> CLIEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> LogEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> GITSLogEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> TraceInfoEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> DiagsEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> TraceConfigEditor;
  std::unique_ptr<gits::ImGuiHelper::TextEditorWidget> TraceStatsEditor;

  std::unique_ptr<gits::gui::EzOptionsPanel> EasyOptionsPanel;
  std::unique_ptr<gits::gui::MetaDataPanel> MetaDataPanel;

  std::shared_ptr<MainWindow> TheMainWindow;

  std::unique_ptr<TextEditorAppender> LogAppender;

  ImGuiHelper::TabGroup<SideBarItems>* BtnsSideBar;
  ImGuiHelper::TabGroup<ConfigSectionItems>* BtnsAPI;
  ImGuiHelper::TabGroup<MetaDataItems>* BtnsMetaData;

  Mode AppMode = Mode::CAPTURE;
  MainAction CurrentMainAction = MainAction::CAPTURE;
  Api SelectedApiForCapture = Api::UNKNOWN;
  std::atomic<bool> CaptureInProgress = false;
  std::atomic<bool> RecordingProcessingPending = false;
  std::atomic<bool> SubcaptureInProgress = false;

  size_t ImguiIDs = 0;

  std::filesystem::path GITSPlayerPath = ".";
  std::filesystem::path GITSBasePath = "";
  bool UseCustomGITSPlayer = false;
  std::filesystem::path StreamPath = "";
  std::filesystem::path TargetPath = "";
  std::filesystem::path ConfigPath = ".";
  std::filesystem::path CaptureConfigPath = "";
  std::filesystem::path SubcapturePath = "";
  std::filesystem::path TracePath = "";
  std::filesystem::path ScreenshotPath = "";
  std::string FixedLauncherArguments = ""; // Arguments created by the launcher
  std::string CustomArguments = "";        // Custom arguments added by user
  std::vector<std::string> CLIArguments;
  std::string CaptureCustomArguments = "";
  std::vector<std::string> CaptureCLIArguments;
  std::filesystem::path CaptureOutputPath = "";

  std::map<ConfigSectionItems, int> ConfigSectionLines = {};

  LauncherConfig LauncherConfiguration;
  std::optional<float> UIScaleDelta = std::nullopt;

  bool ShouldQuit = false;

  void UpdateFixedLauncherArguments();

  bool MainActionAllowed();

  void GITSLog(const std::string& msg);

  std::filesystem::path GetPath(Paths path) const;

  bool IsPlayback();
  bool IsCapture();
  void ChangeMode(Mode mode);
  void UpdatePalette();
};

class TextEditorAppender : public plog::IAppender {
public:
  TextEditorAppender(TextEditorWidget* editor) : m_Editor(editor) {}

  virtual void write(const plog::Record& record) override;

private:
  TextEditorWidget* m_Editor;
};
} // namespace gits::gui
